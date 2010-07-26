/*
  Copyright (C) 2007-2010 Sebastian Trueg <trueg@kde.org>
  Copyright (C) 2010 Vishesh Handa <handa.vish@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this library; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "nepomukindexwriter.h"
#include "nepomukindexfeeder.h"
#include "util.h"
#include "nfo.h"
#include "nie.h"

#include <Soprano/Vocabulary/RDF>
#include <Soprano/LiteralValue>
#include <Soprano/Node>
#include <Soprano/QueryResultIterator>

#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QVariant>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QUrl>
#include <QtCore/QDateTime>
#include <QtCore/QByteArray>
#include <QtCore/QStack>

#include <KUrl>
#include <KDebug>

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <map>
#include <sstream>
#include <algorithm>

#include <Nepomuk/Types/Property>
#include <Nepomuk/Types/Class>
#include <Nepomuk/Types/Literal>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Resource>


// IMPORTANT: strings in Strigi are apparently UTF8! Except for file names. Those are in local encoding.

using namespace Soprano;
using namespace Strigi;


uint qHash( const std::string& s )
{
    return qHash( s.c_str() );
}

namespace {
    QString findArchivePath( const QString& path ) {
        QString p( path );
        int i = 0;
        while ( ( i = p.lastIndexOf( '/' ) ) > 0 ) {
            p.truncate( i );
            if ( QFileInfo( p ).isFile() ) {
                return p;
            }
        }
        return QString();
    }

    QUrl createFileUrl( const Strigi::AnalysisResult* idx ) {
        // HACK: Strigi includes analysers that recurse into tar or zip archives and index
        // the files therein. In KDE these files could perfectly be handled through kio slaves
        // such as tar:/ or zip:/
        // Here we try to use KDE-compatible URIs for these indexed files the best we can
        // everything else defaults to file:/
        QUrl uri;
        QString path = QFile::decodeName( idx->path().c_str() );
        if ( KUrl::isRelativeUrl( path ) )
            uri = QUrl::fromLocalFile( QFileInfo( path ).absoluteFilePath() );
        else
            uri = KUrl( path ); // try to support http and other URLs

        if ( idx->depth() > 0 ) {
            QString archivePath = findArchivePath( path );
            if ( QFile::exists( archivePath ) ) {
                if ( archivePath.endsWith( QLatin1String( ".tar" ) ) ||
                     archivePath.endsWith( QLatin1String( ".tar.gz" ) ) ||
                     archivePath.endsWith( QLatin1String( ".tar.bz2" ) ) ||
                     archivePath.endsWith( QLatin1String( ".tar.lzma" ) ) ||
                     archivePath.endsWith( QLatin1String( ".tar.xz" ) ) ) {
                    uri.setScheme( "tar" );
                }
                else if ( archivePath.endsWith( QLatin1String( ".zip" ) ) ) {
                    uri.setScheme( "zip" );
                }
            }
        }

        // fallback for all
        if ( uri.scheme().isEmpty() ) {
            uri.setScheme( "file" );
        }

        return uri;
    }


    /**
     * Creates a Blank or Resource Node based on the contents of the string provided.
     * If the string is of the form ':identifier', a Blank node is created.
     * Otherwise a Resource Node is returned.
     */
    Soprano::Node createBlankOrResourceNode( const std::string & str ) {
        QString identifier = QString::fromUtf8( str.c_str() );

        if( !identifier.isEmpty() && identifier[0] == ':' ) {
            identifier.remove( 0, 1 );
            return Soprano::Node::createBlankNode( identifier );
        }

        //Not a blank node
        return Soprano::Node( QUrl(identifier) );
    }


    /**
     * A simple cache for properties that are used in Strigi.
     * This avoids matching them again and again.
     *
     * Also the class provides easy conversion methods for
     * values provided by Strigi to values that Nepomuk understands.
     */
    class RegisteredFieldData
    {
    public:
        RegisteredFieldData( const Nepomuk::Types::Property& prop, QVariant::Type t )
            : m_property( prop ),
              m_dataType( t ),
              m_isRdfType( prop == Soprano::Vocabulary::RDF::type() ) {
        }

        const Nepomuk::Types::Property& property() const { return m_property; }
        bool isRdfType() const { return m_isRdfType; }

        Soprano::Node createObject( const std::string& value );

    private:
        Soprano::LiteralValue createLiteralValue( const std::string& value );

        /// The actual property URI
        Nepomuk::Types::Property m_property;

        /// the literal range of the property (if applicable)
        QVariant::Type m_dataType;

        /// caching QUrl comparison
        bool m_isRdfType;
    };


    /**
     * Data objects that are used to store information relative to one
     * indexing run.
     */
    class FileMetaData
    {
    public:
        FileMetaData( const Strigi::AnalysisResult* idx );

        /// stores basic data including the nie:url and the nrl:GraphMetadata in \p model
        void storeBasicData( Nepomuk::IndexFeeder* feeder );

        /// map a blank node to a resource
        QUrl mapNode( const std::string& s );

        /// The resource URI
        QUrl resourceUri;

        /// The file URL (nie:url)
        KUrl fileUrl;

        /// The file info - saved to prevent multiple stats
        QFileInfo fileInfo;

        /// a buffer for all plain-text content generated by strigi
        std::string content;

    private:
        /// The Strigi result
        const Strigi::AnalysisResult* m_analysisResult;
    };


    Soprano::Node RegisteredFieldData::createObject( const std::string& value )
    {
        //
        // Strigi uses anonymeous nodes prefixed with ':'. However, it is possible that literals
        // start with a ':'. Thus, we also check the range of the property
        //
        if ( value[0] == ':' &&
             m_property.range().isValid() ) {
            return createBlankOrResourceNode( value );
        }
        else {
            return createLiteralValue( value );
        }
    }


    Soprano::LiteralValue RegisteredFieldData::createLiteralValue( const std::string& value )
    {
        QString s = QString::fromUtf8( ( const char* )value.c_str(), value.length() ).trimmed();
        if( s.isEmpty() ) 
            return Soprano::LiteralValue();

        // This is a workaround for a Strigi bug which sometimes stores datatime values as strings
        // but the format is not supported by Soprano::LiteralValue
        if ( m_dataType == QVariant::DateTime ) {
            // dateTime is stored as integer (time_t) in strigi
            bool ok = false;
            uint t = s.toUInt( &ok );
            if ( ok )
                return LiteralValue( QDateTime::fromTime_t( t ) );

            // workaround for at least nie:contentCreated which is encoded like this: "2005:06:03 17:13:33"
            QDateTime dt = QDateTime::fromString( s, QLatin1String( "yyyy:MM:dd hh:mm:ss" ) );
            if ( dt.isValid() )
                return LiteralValue( dt );
        }

        // this is a workaround for EXIF values stored as "1/10" and the like which need to
        // be converted to double values.
        else if ( m_dataType == QVariant::Double ) {
            bool ok = false;
            double d = s.toDouble( &ok );
            if ( ok )
                return LiteralValue( d );

            int x = 0;
            int y = 0;
            if ( sscanf( s.toLatin1().data(), "%d/%d", &x, &y ) == 2 ) {
                return LiteralValue( double( x )/double( y ) );
            }
        }

        if ( m_dataType != QVariant::Invalid ) {
            return LiteralValue::fromString( s, m_dataType );
        }
        else {
            // we default to string
            return LiteralValue( s );
        }
    }


    FileMetaData::FileMetaData( const Strigi::AnalysisResult* idx )
        : m_analysisResult( idx )
    {
        fileUrl = createFileUrl( idx );
        fileInfo = QFileInfo( fileUrl.toLocalFile() );

        // determine the resource URI by using Nepomuk::Resource's power
        // this will automatically find previous uses of the file in question
        // with backwards compatibility
        resourceUri = Nepomuk::Resource( fileUrl ).resourceUri();
    }

    void FileMetaData::storeBasicData( Nepomuk::IndexFeeder * feeder )
    {
        feeder->addStatement( resourceUri, Nepomuk::Vocabulary::NIE::url(), fileUrl );

        // Strigi only indexes files and extractors mostly (if at all) store the nie:DataObject type (i.e. the contents)
        // Thus, here we go the easy way and mark each indexed file as a nfo:FileDataObject.
        feeder->addStatement( resourceUri,
                              Vocabulary::RDF::type(),
                              Nepomuk::Vocabulary::NFO::FileDataObject() );
        if ( fileInfo.isDir() ) {
            feeder->addStatement( resourceUri,
                                  Vocabulary::RDF::type(),
                                  Nepomuk::Vocabulary::NFO::Folder() );
        }
    }

    FileMetaData* fileDataForResult( const Strigi::AnalysisResult* idx )
    {
        return static_cast<FileMetaData*>( idx->writerData() );
    }
}


class Nepomuk::StrigiIndexWriter::Private
{
public:
    //
    // The Strigi API does not provide context information in addTriplet, i.e. the AnalysisResult.
    // However, we only use one thread, only one AnalysisResult at the time.
    // Thus, we can just remember that and use it in addTriplet.
    //
    QMutex resultStackMutex;
    QStack<const Strigi::AnalysisResult*> currentResultStack;

    Nepomuk::IndexFeeder* feeder;
};


Nepomuk::StrigiIndexWriter::StrigiIndexWriter( IndexFeeder* feeder )
    : Strigi::IndexWriter(),
    d( new Private() )
{
    d->feeder = feeder;
}


Nepomuk::StrigiIndexWriter::~StrigiIndexWriter()
{
    kDebug();
    delete d;
}


// unused
void Nepomuk::StrigiIndexWriter::commit()
{
    // do nothing
}


// delete all indexed data for the files listed in entries
void Nepomuk::StrigiIndexWriter::deleteEntries( const std::vector<std::string>& entries )
{
    for ( unsigned int i = 0; i < entries.size(); ++i ) {
        QString path = QString::fromUtf8( entries[i].c_str() );
        IndexFeeder::removeIndexedDataForUrl( KUrl( path ) );
    }
}


// unused
void Nepomuk::StrigiIndexWriter::deleteAllEntries()
{
    // do nothing
}


// called for each indexed file
void Nepomuk::StrigiIndexWriter::startAnalysis( const AnalysisResult* idx )
{
    // we need to remember the AnalysisResult since addTriplet does not provide it
    d->currentResultStack.push(idx);

    // for now we ignore embedded files -> too many false positives and useless query results
    if ( idx->depth() > 0 ) {
        return;
    }

    // create the file data used during the analysis
    FileMetaData* data = new FileMetaData( idx );

    // remove previously indexed data
    IndexFeeder::removeIndexedDataForResourceUri( data->resourceUri );

    // It is important to keep the resource URI between updates (especially for sharing of files)
    // However, when updating data from pre-KDE 4.4 times we want to get rid of old file:/ resource
    // URIs. However, we can only do that if we were the only ones to write info about that file
    // Thus, we need to use Nepomuk::Resource again
    if ( data->resourceUri.scheme() == QLatin1String( "file" ) ) {
        // we need to clear the ResourceManager cache. Otherwise the bug/shortcoming in libnepomuk will
        // always get us back to the cached file:/ URI
        Nepomuk::ResourceManager::instance()->clearCache();
        data->resourceUri = Nepomuk::Resource( data->fileUrl ).resourceUri();
    }

    // create a new resource URI for non-existing file resources
    if ( data->resourceUri.isEmpty() )
        data->resourceUri = Nepomuk::ResourceManager::instance()->generateUniqueUri( QString() );

    // Initialize the feeder to accept statements
    d->feeder->begin( data->resourceUri );

    // store initial data to make sure newly created URIs are reused directly by libnepomuk
    data->storeBasicData( d->feeder );

    // remember the file data
    idx->setWriterData( data );
}


void Nepomuk::StrigiIndexWriter::addText( const AnalysisResult* idx, const char* text, int32_t length )
{
    if ( idx->depth() > 0 ) {
        return;
    }

    FileMetaData* md = fileDataForResult( idx );
    md->content.append( text, length );
}


void Nepomuk::StrigiIndexWriter::addValue( const AnalysisResult* idx,
                                           const RegisteredField* field,
                                           const std::string& value )
{
    if ( idx->depth() > 0 ) {
        return;
    }

    if ( value.length() > 0 ) {
        FileMetaData* md = fileDataForResult( idx );
        RegisteredFieldData* rfd = reinterpret_cast<RegisteredFieldData*>( field->writerData() );

        // the statement we will create, we will determine the object below
        Soprano::Statement statement( md->resourceUri, rfd->property().uri(), Soprano::Node() );

        //
        // Strigi uses rdf:type improperly since it stores the value as a string. We have to
        // make sure it is a resource.
        // only we handle the basic File/Folder typing ourselves
        //
        if ( rfd->isRdfType() ) {
            const QUrl type = QUrl::fromEncoded( value.c_str(), QUrl::StrictMode );
            if ( statement.object().uri() != Nepomuk::Vocabulary::NFO::FileDataObject() ) {
                statement.setObject( type );
            }
        }

        //
        // parent location is a special case as we need the URI of the corresponding resource instead of the path
        //
        else if ( field->key() == FieldRegister::parentLocationFieldName ) {
            QUrl folderUri = determineFolderResourceUri( QUrl::fromLocalFile( QFile::decodeName( QByteArray::fromRawData( value.c_str(), value.length() ) ) ) );
            if ( !folderUri.isEmpty() )
                statement.setObject( folderUri );
        }

        //
        // ignore the path as we handle that ourselves in startAnalysis
        //
        else if ( field->key() != FieldRegister::pathFieldName ) {
            statement.setObject( rfd->createObject( value ) );
        }

        if ( !statement.object().isEmpty() ) {
            d->feeder->addStatement( statement );
        }
    }
}


// the main addValue method
void Nepomuk::StrigiIndexWriter::addValue( const AnalysisResult* idx,
                                           const RegisteredField* field,
                                           const unsigned char* data,
                                           uint32_t size )
{
    addValue( idx, field, std::string( ( const char* )data, size ) );
}


void Nepomuk::StrigiIndexWriter::addValue( const AnalysisResult*, const RegisteredField*,
                                           const std::string&, const std::string& )
{
    // we do not support map types
}


void Nepomuk::StrigiIndexWriter::addValue( const AnalysisResult* idx,
                                           const RegisteredField* field,
                                           uint32_t value )
{
    if ( idx->depth() > 0 ) {
        return;
    }

    FileMetaData* md = fileDataForResult( idx );
    RegisteredFieldData* rfd = reinterpret_cast<RegisteredFieldData*>( field->writerData() );

    LiteralValue val( value );
    if ( field->type() == FieldRegister::datetimeType ) {
        val = QDateTime::fromTime_t( value );
    }

    d->feeder->addStatement( md->resourceUri, rfd->property().uri(), val);
}


void Nepomuk::StrigiIndexWriter::addValue( const AnalysisResult* idx,
                                           const RegisteredField* field,
                                           int32_t value )
{
    if ( idx->depth() > 0 ) {
        return;
    }

    FileMetaData* md = fileDataForResult( idx );
    RegisteredFieldData* rfd = reinterpret_cast<RegisteredFieldData*>( field->writerData() );

    d->feeder->addStatement( md->resourceUri, rfd->property().uri(), LiteralValue( value ) );
}


void Nepomuk::StrigiIndexWriter::addValue( const AnalysisResult* idx,
                                           const RegisteredField* field,
                                           double value )
{
    if ( idx->depth() > 0 ) {
        return;
    }

    FileMetaData* md = fileDataForResult( idx );
    RegisteredFieldData* rfd = reinterpret_cast<RegisteredFieldData*>( field->writerData() );

    d->feeder->addStatement( md->resourceUri, rfd->property().uri(), LiteralValue( value ) );
}


void Nepomuk::StrigiIndexWriter::addTriplet( const std::string& s,
                                             const std::string& p,
                                             const std::string& o )
{
    if ( d->currentResultStack.top()->depth() > 0 ) {
        return;
    }

    //FileMetaData* md = fileDataForResult( d->currentResultStack.top() );

    Soprano::Node subject( createBlankOrResourceNode( s ) );
    Nepomuk::Types::Property property( QUrl( QString::fromUtf8(p.c_str()) ) ); // Was mapped earlier
    Soprano::Node object;
    if ( property.range().isValid() )
        object = Soprano::Node( createBlankOrResourceNode( o ) );
    else
        object = Soprano::LiteralValue::fromString( QString::fromUtf8( o.c_str() ), property.literalRangeType().dataTypeUri() );

    d->feeder->addStatement( subject, property.uri(), object );
}


// called after each indexed file
void Nepomuk::StrigiIndexWriter::finishAnalysis( const AnalysisResult* idx )
{
    d->currentResultStack.pop();

    if ( idx->depth() > 0 ) {
        return;
    }

    FileMetaData* md = fileDataForResult( idx );

    // store the full text of the file
    if ( md->content.length() > 0 ) {
        d->feeder->addStatement( md->resourceUri,
                                 Nepomuk::Vocabulary::NIE::plainTextContent(),
                                 LiteralValue( QString::fromUtf8( md->content.c_str() ) ) );
    }

    d->feeder->end( md->fileInfo.isDir() );

    // cleanup
    delete md;
    idx->setWriterData( 0 );
}


void Nepomuk::StrigiIndexWriter::initWriterData( const Strigi::FieldRegister& f )
{
    // build a temp hash for built-in strigi types
    QHash<std::string, QVariant::Type> literalTypes;
    literalTypes[FieldRegister::stringType] = QVariant::String;
    literalTypes[FieldRegister::floatType] = QVariant::Double;
    literalTypes[FieldRegister::integerType] = QVariant::Int;
    literalTypes[FieldRegister::binaryType] = QVariant::ByteArray;
    literalTypes[FieldRegister::datetimeType] = QVariant::DateTime; // Strigi encodes datetime as unsigned integer, i.e. addValue( ..., uint )

    // cache type conversion for all strigi fields
    std::map<std::string, RegisteredField*>::const_iterator i;
    std::map<std::string, RegisteredField*>::const_iterator end = f.fields().end();
    for (i = f.fields().begin(); i != end; ++i) {
        Nepomuk::Types::Property prop = Strigi::Util::fieldUri( i->second->key() );

        QVariant::Type type( QVariant::Invalid );

        QHash<std::string, QVariant::Type>::const_iterator it = literalTypes.constFind( i->second->properties().typeUri() );
        if ( it != literalTypes.constEnd() ) {
            type = *it;
        }
        else if ( prop.literalRangeType().isValid() ) {
            type = LiteralValue::typeFromDataTypeUri( prop.literalRangeType().dataTypeUri() );
        }

        kDebug() << prop << type;

        i->second->setWriterData( new RegisteredFieldData( prop, type ) );
    }
}


void Nepomuk::StrigiIndexWriter::releaseWriterData( const Strigi::FieldRegister& f )
{
    std::map<std::string, RegisteredField*>::const_iterator i;
    std::map<std::string, RegisteredField*>::const_iterator end = f.fields().end();
    for (i = f.fields().begin(); i != end; ++i) {
        delete static_cast<RegisteredFieldData*>( i->second->writerData() );
        i->second->setWriterData( 0 );
    }
}


QUrl Nepomuk::StrigiIndexWriter::determineFolderResourceUri( const KUrl& fileUrl )
{
    Nepomuk::Resource res( fileUrl );
    if ( res.exists() ) {
        return res.resourceUri();
    }
    else {
        kDebug() << "Could not find resource URI for folder (this is not an error)" << fileUrl;
        return QUrl();
    }
}
