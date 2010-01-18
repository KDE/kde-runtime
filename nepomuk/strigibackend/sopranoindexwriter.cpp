/*
   Copyright (C) 2007-2010 Sebastian Trueg <trueg@kde.org>

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

#include "sopranoindexwriter.h"
#include "util.h"
#include "nfo.h"
#include "nie.h"

#include <Soprano/Soprano>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/Xesam>
#include <Soprano/LiteralValue>
#include <Soprano/Node>
#include <Soprano/QueryResultIterator>

#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QVariant>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QUrl>
#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QDateTime>
#include <QtCore/QByteArray>
#include <QtCore/QUuid>
#include <QtCore/QStack>

#include <KUrl>

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <map>
#include <sstream>
#include <algorithm>

#include <Nepomuk/Types/Property>
#include <Nepomuk/Types/Class>
#include <Nepomuk/Types/Literal>

// IMPORTANT: strings in Strigi are apparently UTF8! Except for file names. Those are in local encoding.

using namespace Soprano;
using namespace std;


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

    QUrl createResourceUri( const Strigi::AnalysisResult* idx ) {
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

    class FileMetaData
    {
    public:
        // caching URIs for little speed improvement
        QUrl fileUri;
        QUrl context;
        std::string content;

        // mapping from blank nodes used in addTriplet to our urns
        QMap<std::string, QUrl> blankNodeMap;
    };

    class RegisteredFieldData
    {
    public:
        RegisteredFieldData( const QUrl& prop, QVariant::Type t )
            : property( prop ),
              dataType( t ),
              isRdfType( prop == Vocabulary::RDF::type() ) {
        }

        QUrl property;
        QVariant::Type dataType;
        bool isRdfType;
    };
}


class Strigi::Soprano::IndexWriter::Private
{
public:
    Private()
        : indexTransactionID( 0 ) {
        literalTypes[FieldRegister::stringType] = QVariant::String;
        literalTypes[FieldRegister::floatType] = QVariant::Double;
        literalTypes[FieldRegister::integerType] = QVariant::Int;
        literalTypes[FieldRegister::binaryType] = QVariant::ByteArray;
        literalTypes[FieldRegister::datetimeType] = QVariant::DateTime; // Strigi encodes datetime as unsigned integer, i.e. addValue( ..., uint )
    }

    QVariant::Type literalType( const Strigi::FieldProperties& strigiType ) {
        // it looks as if the typeUri can contain arbitrary values, URIs or stuff like "string"
        QHash<std::string, QVariant::Type>::const_iterator it = literalTypes.constFind( strigiType.typeUri() );
        if ( it == literalTypes.constEnd() ) {
            return LiteralValue::typeFromDataTypeUri( QUrl::fromEncoded( strigiType.typeUri().c_str() ) );
        }
        else {
            return *it;
        }
    }

    LiteralValue createLiteralValue( QVariant::Type type,
                                     const unsigned char* data,
                                     uint32_t size ) {
        QString value = QString::fromUtf8( ( const char* )data, size );
        if ( type == QVariant::DateTime ) { // dataTime is stored as integer in strigi!
            return LiteralValue( QDateTime::fromTime_t( value.toUInt() ) );
        }
        else if ( type != QVariant::Invalid ) {
            return LiteralValue::fromString( value, type );
        }
        else {
            // we default to string
            return LiteralValue( value );
        }
    }

    QUrl createUrn() {
        QUrl urn;
        do {
            urn = QUrl( "urn:nepomuk:local:" + QUuid::createUuid().toString().remove( QRegExp( "[\\{\\}]" ) ) );
        } while ( repository->executeQuery( QString("ask where { "
                                                    "{ %1 ?p1 ?o1 . } "
                                                    "UNION "
                                                    "{ ?r2 %1 ?o2 . } "
                                                    "UNION "
                                                    "{ ?r3 ?p3 %1 . } "
                                                    "}")
                                            .arg( ::Soprano::Node::resourceToN3( urn ) ),
                                            ::Soprano::Query::QueryLanguageSparql ).boolValue() );
        return urn;
    }

    QUrl mapNode( FileMetaData* fmd, const std::string& s ) {
        if ( s[0] == ':' ) {
            if( fmd->blankNodeMap.contains( s ) ) {
                return fmd->blankNodeMap[s];
            }
            else {
                QUrl urn = createUrn();
                fmd->blankNodeMap.insert( s, urn );
                return urn;
            }
        }
        else {
            return QUrl::fromEncoded( s.c_str() );
        }
    }

    ::Soprano::Model* repository;
    int indexTransactionID;

    //
    // The Strigi API does not provide context information in addTriplet, i.e. the AnalysisResult.
    // However, we only use one thread, only one AnalysisResult at the time.
    // Thus, we can just remember that and use it in addTriplet.
    //
    QStack<const Strigi::AnalysisResult*> currentResultStack;

private:
    QHash<std::string, QVariant::Type> literalTypes;
};


Strigi::Soprano::IndexWriter::IndexWriter( ::Soprano::Model* model )
    : Strigi::IndexWriter()
{
    d = new Private;
    d->repository = model;
    Util::storeStrigiMiniOntology( d->repository );
}


Strigi::Soprano::IndexWriter::~IndexWriter()
{
    delete d;
}


void Strigi::Soprano::IndexWriter::commit()
{
}


// delete all indexed data for the files listed in entries
void Strigi::Soprano::IndexWriter::deleteEntries( const std::vector<std::string>& entries )
{
    for ( unsigned int i = 0; i < entries.size(); ++i ) {
        //
        // We are compatible with old Xesam data where the url was encoded as a string instead of a url,
        // thus the weird query
        //
        QString path = QString::fromUtf8( entries[i].c_str() );
        QString query = QString( "select ?g ?mg where { "
                                 "{ ?r %3 %1 . } "
                                 "UNION "
                                 "{ ?r %3 %2 . } "
                                 "UNION "
                                 "{ ?r %4 %2 . } . "
                                 "?g %5 ?r . "
                                 "OPTIONAL { ?mg %6 ?g . } }" )
                        .arg( Node::literalToN3( path ),
                              Node::resourceToN3( QUrl::fromLocalFile( path ) ),
                              Node::resourceToN3( Vocabulary::Xesam::url() ),
                              Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                              Node::resourceToN3( Strigi::Ontology::indexGraphFor() ),
                              Node::resourceToN3( Vocabulary::NRL::coreGraphMetadataFor() ) );

//        qDebug() << "deleteEntries query:" << query;

        QueryResultIterator result = d->repository->executeQuery( query, ::Soprano::Query::QueryLanguageSparql );
        if ( result.next() ) {
            Node indexGraph = result.binding( "g" );
            Node metaDataGraph = result.binding( "mg" );

            result.close();

            // delete the indexed data
            d->repository->removeContext( indexGraph );

            // delete the metadata (backwards compatible)
            if ( metaDataGraph.isValid() )
                d->repository->removeContext( metaDataGraph );
            else
                d->repository->removeAllStatements( Statement( indexGraph, Node(), Node() ) );
        }
    }
}


void Strigi::Soprano::IndexWriter::deleteAllEntries()
{
    // query all index graphs (FIXME: would a type derived from nrl:Graph be better than only the predicate?)
    QString query = QString( "select ?g where { ?g %1 ?r . }" ).arg( Node::resourceToN3( Strigi::Ontology::indexGraphFor() ) );

    qDebug() << "deleteAllEntries query:" << query;

    QueryResultIterator result = d->repository->executeQuery( query, ::Soprano::Query::QUERY_LANGUAGE_SPARQL );
    QList<Node> allIndexGraphs = result.iterateBindings( "g" ).allNodes();
    for ( QList<Node>::const_iterator it = allIndexGraphs.constBegin(); it != allIndexGraphs.constEnd(); ++it ) {
        Node indexGraph = *it;

        qDebug() << "Found indexGraph to delete:" << indexGraph;

        // delete the indexed data
        d->repository->removeContext( indexGraph );

        // delete the metadata
        d->repository->removeAllStatements( Statement( indexGraph, Node(), Node() ) );
    }
}


// called for each indexed file
void Strigi::Soprano::IndexWriter::startAnalysis( const AnalysisResult* idx )
{
    // we need to remember the AnalysisResult since addTriplet does not provide it
    d->currentResultStack.push(idx);

    if ( idx->depth() > 0 ) {
        return;
    }

    FileMetaData* data = new FileMetaData();
    data->fileUri = createResourceUri( idx );

    // let's check if we already have data on the file
    StatementIterator it = d->repository->listStatements( Node(),
                                                          Strigi::Ontology::indexGraphFor(),
                                                          data->fileUri );
    if ( it.next() ) {
        data->context = it.current().subject().uri();
    }
    else {
        data->context = d->createUrn();
    }

    idx->setWriterData( data );
}


void Strigi::Soprano::IndexWriter::addText( const AnalysisResult* idx, const char* text, int32_t length )
{
    if ( idx->depth() > 0 ) {
        return;
    }

    FileMetaData* md = reinterpret_cast<FileMetaData*>( idx->writerData() );
    md->content.append( text, length );
}


void Strigi::Soprano::IndexWriter::addValue( const AnalysisResult* idx,
                                             const RegisteredField* field,
                                             const std::string& value )
{
    if ( idx->depth() > 0 ) {
        return;
    }

    if ( value.length() > 0 ) {
        FileMetaData* md = reinterpret_cast<FileMetaData*>( idx->writerData() );
        RegisteredFieldData* rfd = reinterpret_cast<RegisteredFieldData*>( field->writerData() );

        // the statement we will create, we will determine the object below
        ::Soprano::Statement statement( md->fileUri, rfd->property, ::Soprano::Node(), md->context );

        //
        // Strigi uses rdf:type improperly since it stores the value as a string. We have to
        // make sure it is a resource.
        //
        if ( rfd->isRdfType ) {
            statement.setPredicate( ::Soprano::Vocabulary::RDF::type() );
            statement.setObject( QUrl::fromEncoded( value.c_str(), QUrl::StrictMode ) );
        }

        else {
            //
            // we bend the plain strigi properties into something nicer, also because we
            // do not want paths to be indexed, way too many false positives
            // in standard desktop searches
            //
            if ( field->key() == FieldRegister::pathFieldName ||
                 field->key() == FieldRegister::parentLocationFieldName ) {
                // TODO: this is where relative file URLs are to be generated and our new fancy file system
                // class should provide us with the file system URI
                statement.setObject( QUrl::fromLocalFile( QFile::decodeName( QByteArray::fromRawData( value.c_str(), value.length() ) ) ) );
            }
            else {
                statement.setObject( d->createLiteralValue( rfd->dataType, ( unsigned char* )value.c_str(), value.length() ) );
            }

            //
            // Strigi uses anonymeous nodes prefixed with ':'. However, it is possible that literals
            // start with a ':'. Thus, we also check the range of the property
            //
            if ( value[0] == ':' ) {
                Nepomuk::Types::Property property( rfd->property );
                if ( property.range().isValid() ) {
                    statement.setObject( d->mapNode( md, value ) );
                }
            }
        }

        d->repository->addStatement( statement );
    }
}


// the main addValue method
void Strigi::Soprano::IndexWriter::addValue( const AnalysisResult* idx,
                                             const RegisteredField* field,
                                             const unsigned char* data,
                                             uint32_t size )
{
    addValue( idx, field, std::string( ( const char* )data, size ) );
}


void Strigi::Soprano::IndexWriter::addValue( const AnalysisResult*, const RegisteredField*,
                                             const std::string&, const std::string& )
{
    // we do not support map types
}


void Strigi::Soprano::IndexWriter::addValue( const AnalysisResult* idx,
                                             const RegisteredField* field,
                                             uint32_t value )
{
    if ( idx->depth() > 0 ) {
        return;
    }

    FileMetaData* md = reinterpret_cast<FileMetaData*>( idx->writerData() );
    RegisteredFieldData* rfd = reinterpret_cast<RegisteredFieldData*>( field->writerData() );

    LiteralValue val( value );
    if ( field->type() == FieldRegister::datetimeType ) {
        val = QDateTime::fromTime_t( value );
    }

    d->repository->addStatement( Statement( md->fileUri,
                                            rfd->property,
                                            val,
                                            md->context) );
}


void Strigi::Soprano::IndexWriter::addValue( const AnalysisResult* idx,
                                             const RegisteredField* field,
                                             int32_t value )
{
    if ( idx->depth() > 0 ) {
        return;
    }

    FileMetaData* md = reinterpret_cast<FileMetaData*>( idx->writerData() );
    RegisteredFieldData* rfd = reinterpret_cast<RegisteredFieldData*>( field->writerData() );

    d->repository->addStatement( Statement( md->fileUri,
                                            rfd->property,
                                            LiteralValue( value ),
                                            md->context) );
}


void Strigi::Soprano::IndexWriter::addValue( const AnalysisResult* idx,
                                             const RegisteredField* field,
                                             double value )
{
    if ( idx->depth() > 0 ) {
        return;
    }

    FileMetaData* md = reinterpret_cast<FileMetaData*>( idx->writerData() );
    RegisteredFieldData* rfd = reinterpret_cast<RegisteredFieldData*>( field->writerData() );

    d->repository->addStatement( Statement( md->fileUri,
                                            rfd->property,
                                            LiteralValue( value ),
                                            md->context) );
}


void Strigi::Soprano::IndexWriter::addTriplet( const std::string& s,
                                               const std::string& p,
                                               const std::string& o )
{
    if ( d->currentResultStack.top()->depth() > 0 ) {
        return;
    }

    //
    // The Strigi API does not provide context information here, i.e. the AnalysisResult this triple
    // belongs to. However, we only use one thread, only one AnalysisResult at the time.
    // Thus, we can just remember that and use it here.
    //
    FileMetaData* md = static_cast<FileMetaData*>( d->currentResultStack.top()->writerData() );

    QUrl subject = d->mapNode( md, s );
    Nepomuk::Types::Property property( d->mapNode( md, p ) );
    ::Soprano::Node object;
    if ( property.range().isValid() )
        object = d->mapNode( md, o );
    else
        object = ::Soprano::LiteralValue::fromString( QString::fromUtf8( o.c_str() ), property.literalRangeType().dataTypeUri() );

    d->repository->addStatement( subject, property.uri(), object, md->context );
}


// called after each indexed file
void Strigi::Soprano::IndexWriter::finishAnalysis( const AnalysisResult* idx )
{
    d->currentResultStack.pop();

    if ( idx->depth() > 0 ) {
        return;
    }

    FileMetaData* md = static_cast<FileMetaData*>( idx->writerData() );

    if ( md->content.length() > 0 ) {
        d->repository->addStatement( Statement( md->fileUri,
                                                Nepomuk::Vocabulary::NIE::plainTextContent(),
                                                LiteralValue( QString::fromUtf8( md->content.c_str() ) ),
                                                md->context ) );
        if ( d->repository->lastError() )
            qDebug() << "Failed to add" << md->fileUri << "as text" << QString::fromUtf8( md->content.c_str() );
    }

    // Strigi only indexes files and extractors mostly (if at all) store the nie:DataObject type (i.e. the contents)
    // Thus, here we go the easy way and mark each indexed file as a nfo:FileDataObject.
    if ( QFileInfo( QFile::decodeName( idx->path().c_str() ) ).isDir() )
        d->repository->addStatement( Statement( md->fileUri,
                                                Vocabulary::RDF::type(),
                                                Nepomuk::Vocabulary::NFO::Folder(),
                                                md->context ) );
    else
        d->repository->addStatement( Statement( md->fileUri,
                                                Vocabulary::RDF::type(),
                                                Nepomuk::Vocabulary::NFO::FileDataObject(),
                                                md->context ) );


    // create the provedance data for the data graph
    // TODO: add more data at some point when it becomes of interest
    QUrl metaDataContext = md->context.toString() + "-metadata";
    d->repository->addStatement( Statement( md->context,
                                            Vocabulary::RDF::type(),
                                            Vocabulary::NRL::InstanceBase(),
                                            metaDataContext ) );
    d->repository->addStatement( Statement( md->context,
                                            Vocabulary::NAO::created(),
                                            LiteralValue( QDateTime::currentDateTime() ),
                                            metaDataContext ) );
    d->repository->addStatement( Statement( md->context,
                                            Strigi::Ontology::indexGraphFor(),
                                            md->fileUri,
                                            metaDataContext ) );
    d->repository->addStatement( Statement( metaDataContext,
                                            Vocabulary::RDF::type(),
                                            Vocabulary::NRL::GraphMetadata(),
                                            metaDataContext ) );
    d->repository->addStatement( metaDataContext,
                                 Vocabulary::NRL::coreGraphMetadataFor(),
                                 md->context,
                                 metaDataContext );

    // cleanup
    delete md;
    idx->setWriterData( 0 );
}


void Strigi::Soprano::IndexWriter::initWriterData( const Strigi::FieldRegister& f )
{
    map<string, RegisteredField*>::const_iterator i;
    map<string, RegisteredField*>::const_iterator end = f.fields().end();
    for (i = f.fields().begin(); i != end; ++i) {
        QUrl prop = Util::fieldUri( i->second->key() );
        i->second->setWriterData( new RegisteredFieldData( prop,
                                                           prop == Vocabulary::RDF::type()
                                                           ? QVariant::Invalid
                                                           : d->literalType( i->second->properties() ) ) );
    }
}


void Strigi::Soprano::IndexWriter::releaseWriterData( const Strigi::FieldRegister& f )
{
    map<string, RegisteredField*>::const_iterator i;
    map<string, RegisteredField*>::const_iterator end = f.fields().end();
    for (i = f.fields().begin(); i != end; ++i) {
        delete static_cast<RegisteredFieldData*>( i->second->writerData() );
        i->second->setWriterData( 0 );
    }
}
