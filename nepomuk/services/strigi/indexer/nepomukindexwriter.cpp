/*
  Copyright (C) 2007-2011 Sebastian Trueg <trueg@kde.org>
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
#include "../util.h"
#include "kext.h"
#include "datamanagement.h"
#include "simpleresource.h"
#include "simpleresourcegraph.h"

#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/NRL>
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
#include <QtCore/QMutex>
#include <QtCore/QProcess>

#include <KUrl>
#include <KDebug>
#include <KMimeType>
#include <kde_file.h>
#include <KJob>
#include <KStandardDirs>

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <map>
#include <sstream>
#include <algorithm>

#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NIE>



// IMPORTANT: strings in Strigi are apparently UTF8! Except for file names. Those are in local encoding.

using namespace Soprano;
using namespace Strigi;
using namespace Soprano::Vocabulary;
using namespace Nepomuk::Vocabulary;


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
     * A simple cache for properties that are used in Strigi.
     * This avoids matching them again and again.
     *
     * Also the class provides easy conversion methods for
     * values provided by Strigi to values that Nepomuk understands.
     */
    class RegisteredFieldData
    {
    public:
        RegisteredFieldData( const QUrl& prop )
            : m_property( prop ) {
        }

        QUrl property() const { return m_property; }

    private:
        /// The actual property URI
        QUrl m_property;
    };


    /**
     * Data objects that are used to store information relative to one
     * indexing run.
     */
    class FileMetaData
    {
    public:
        FileMetaData( const Strigi::AnalysisResult* idx, const QUrl & resUri );

        /// convert the Strigi ids found in object values into the corresponding sub-resource URIs
        Nepomuk::SimpleResource convertSubResourceIds(const Nepomuk::SimpleResource& res) const;

        /// The resource URI
        QUrl resourceUri;

        /// The file URL (nie:url)
        KUrl fileUrl;

        /// The file info - saved to prevent multiple stats
        QFileInfo fileInfo;

        /// a buffer for all plain-text content generated by strigi
        std::string content;

        /// The main file's metadata
        Nepomuk::SimpleResource data;

        /// The sub-resources, mapped by the identifier libstreamanalyzer provided
        QHash<QString, Nepomuk::SimpleResource> subResources;

    private:
        /// The Strigi result
        const Strigi::AnalysisResult* m_analysisResult;
    };


    FileMetaData::FileMetaData( const Strigi::AnalysisResult* idx, const QUrl & resUri )
        : m_analysisResult( idx )
    {
        fileUrl = createFileUrl( idx );
        fileInfo = QFileInfo( fileUrl.toLocalFile() );
        resourceUri = resUri;
        if(resourceUri.isEmpty()) {
            data.setUri(fileUrl);
        }
        else {
            data.setUri(resourceUri);
        }
    }

    Nepomuk::SimpleResource FileMetaData::convertSubResourceIds(const Nepomuk::SimpleResource& res) const
    {
        Nepomuk::PropertyHash props = res.properties();
        QMutableHashIterator<QUrl, QVariant> it(props);
        while(it.hasNext()) {
            it.next();
            if(it.value().type() == QVariant::String) {
                QHash<QString, Nepomuk::SimpleResource>::const_iterator subResIt = subResources.constFind(it.value().toString());
                if(subResIt != subResources.constEnd()) {
                    it.setValue(subResIt.value().uri());
                }
            }
        }
        Nepomuk::SimpleResource newRes(res);
        newRes.setProperties(props);
        return newRes;
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

    // Some services may need to force a specific resource uri
    QUrl resourceUri;
};


Nepomuk::StrigiIndexWriter::StrigiIndexWriter()
    : Strigi::IndexWriter(),
    d( new Private() )
{
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
    Q_ASSERT(false);
}


// unused
void Nepomuk::StrigiIndexWriter::deleteEntries( const std::vector<std::string>& entries )
{
    // do nothing
    Q_UNUSED(entries);
    Q_ASSERT(false);
}


// unused
void Nepomuk::StrigiIndexWriter::deleteAllEntries()
{
    // do nothing
    Q_ASSERT(false);
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
    FileMetaData* data = new FileMetaData( idx, d->resourceUri );

    // remove previously indexed data
    // but only for files. We cannot remove folders since that would also remove the
    // nie:isPartOf links from the files.
    if(!data->fileInfo.isDir()) {
        if( !data->resourceUri.isEmpty() ) {
            Nepomuk::clearIndexedData(data->resourceUri)->exec();
        }
        else {
            Nepomuk::clearIndexedData(data->fileUrl)->exec();
        }
    }

    // remember the file data
    idx->setWriterData( data );
}


void Nepomuk::StrigiIndexWriter::addText( const AnalysisResult* idx, const char* text, int32_t length )
{
    if ( idx->depth() > 0 ) {
        return;
    }

    FileMetaData* md = fileDataForResult( idx );

    // make sure text fragments are separated
    if(md->content.size() > 0)
        md->content.append( " " );

    // append the new fragment
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

        //
        // ignore the path as we handle that ourselves in startAnalysis
        // ignore the mimetype since we handle that ourselves in finishAnalysis
        //  and KDE's mimetype is much more reliable than Strigi's.
        // TODO: remember the mimetype and use it only if KMimeType does not find one.
        //
        if ( field->key() != FieldRegister::pathFieldName &&
             field->key() != FieldRegister::mimetypeFieldName ) {
            md->data.addProperty(rfd->property(), QString::fromUtf8(value.c_str()));
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

    md->data.addProperty(rfd->property(), value);
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

    md->data.addProperty(rfd->property(), value);
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

    md->data.addProperty(rfd->property(), value);
}


void Nepomuk::StrigiIndexWriter::addTriplet( const std::string& s,
                                             const std::string& p,
                                             const std::string& o )
{
    if ( d->currentResultStack.top()->depth() > 0 ) {
        kDebug() << "Depth > 0 -" << s.c_str() << p.c_str() << o.c_str();
        return;
    }
    FileMetaData* md = fileDataForResult( d->currentResultStack.top() );

    // convert the std strings to Qt values
    const QString subResId(QString::fromUtf8(s.c_str()));
    const QUrl property(QString::fromUtf8(p.c_str()));
    const QString value(QString::fromUtf8(o.c_str()));

    // the subject might be the indexed file itself
    if(KUrl(subResId) == md->fileUrl) {
        md->data.addProperty(property, value);
    }
    else {
        // Find the corresponding sub-resource
        QHash<QString, Nepomuk::SimpleResource>::iterator it = md->subResources.find(subResId);
        if(it == md->subResources.end()) {
            Nepomuk::SimpleResource subRes;
            subRes.addProperty(property, value);
            md->subResources.insert(subResId, subRes);
        }
        else {
            it->addProperty(property, value);
        }
    }
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
        md->data.addProperty(Nepomuk::Vocabulary::NIE::plainTextContent(),
                             QString::fromUtf8( md->content.c_str() ));
    }

    // store the nie:url in any case
    md->data.addProperty(Nepomuk::Vocabulary::NIE::url(), md->fileUrl);

    if( md->fileInfo.exists() ) {
        //
        // Strigi only indexes files and many extractors are still not perfect with types.
        // Each file is a nie:DataObject and a nie:InformationElement. Many Strigi plugins
        // forget at least one of the two.
        // Thus, here we go the easy way and mark each indexed file as a nfo:FileDataObject
        // and a nie:InformationElement, thus, at least providing the basic types to Nepomuk's
        // domain and range checking.
        //
        md->data.addType(Nepomuk::Vocabulary::NFO::FileDataObject());
        md->data.addType(Nepomuk::Vocabulary::NIE::InformationElement());
        if ( md->fileInfo.isDir() ) {
            md->data.addType(Nepomuk::Vocabulary::NFO::Folder());
        }

        // write the mimetype for local files
        if(md->fileInfo.isSymLink()) {
            md->data.addProperty( Nepomuk::Vocabulary::NIE::mimeType(),
                                  QLatin1String("application/x-symlink"));
        }
        else {
            md->data.addProperty( Nepomuk::Vocabulary::NIE::mimeType(),
                                  KMimeType::findByUrl(md->fileUrl, 0 /* default mode */, true /* is local file */)->name() );
        }
#ifdef Q_OS_UNIX
        KDE_struct_stat statBuf;
        if( KDE_stat( QFile::encodeName(md->fileInfo.absoluteFilePath()).data(), &statBuf ) == 0 ) {
            md->data.addProperty( Nepomuk::Vocabulary::KExt::unixFileMode(),
                                  int(statBuf.st_mode) );
        }
        if( !md->fileInfo.owner().isEmpty() )
            md->data.addProperty( Nepomuk::Vocabulary::KExt::unixFileOwner(),
                                  md->fileInfo.owner() );
        if( !md->fileInfo.group().isEmpty() )
            md->data.addProperty( Nepomuk::Vocabulary::KExt::unixFileGroup(),
                                  md->fileInfo.group() );
#endif // Q_OS_UNIX
    }


    //
    // A small hack for PDF files. The strigi analyzer is not very powerful yet. Thus,
    // we use prdftotext to extract the contents instead.
    //
    if(md->data.contains(NIE::mimeType(), QLatin1String("application/pdf"))) {
        const QString txt = extractTextFromPdf(md->fileUrl.toLocalFile());
        if(!txt.isEmpty()) {
            md->data.setProperty(NIE::plainTextContent(), txt);
        }
    }


    //
    // Finally push all the information to Nepomuk
    //
    Nepomuk::SimpleResourceGraph graph;

    // Add all the sub-resources as sub-resources of the main resource
    for(QHash<QString, Nepomuk::SimpleResource>::const_iterator it = md->subResources.constBegin();
        it != md->subResources.constEnd(); ++it) {
        md->data.addProperty(NAO::hasSubResource(), it.value());
        graph << md->convertSubResourceIds(it.value());
    }

    // finally add the file resource itself to the graph
    graph << md->convertSubResourceIds(md->data);

    // Add additional metadata - mark the information as discardable
    QHash<QUrl, QVariant> additionalMetadata;
    additionalMetadata.insert( RDF::type(), NRL::DiscardableInstanceBase() );

    // we do not have an event loop - thus, we need to delete the job ourselves
    QScopedPointer<KJob> job( Nepomuk::storeResources( graph, Nepomuk::IdentifyNew, Nepomuk::LazyCardinalities|Nepomuk::OverwriteProperties, additionalMetadata ) );
    job->setAutoDelete(false);
    job->exec();
    if( job->error() ) {
        kDebug() << job->errorString();
    }

    // cleanup
    delete md;
    idx->setWriterData( 0 );
}


void Nepomuk::StrigiIndexWriter::initWriterData( const Strigi::FieldRegister& f )
{
    // cache type conversion for all strigi fields
    std::map<std::string, RegisteredField*>::const_iterator i;
    std::map<std::string, RegisteredField*>::const_iterator end = f.fields().end();
    for (i = f.fields().begin(); i != end; ++i) {
        const QUrl prop = QUrl::fromEncoded( i->second->key().c_str() );
        i->second->setWriterData( new RegisteredFieldData( prop ) );
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


void Nepomuk::StrigiIndexWriter::forceUri(const QUrl& uri)
{
    d->resourceUri = uri;
}

// static
QString Nepomuk::StrigiIndexWriter::extractTextFromPdf(const QString &path)
{
    const QString pdf2txtExe = KStandardDirs::findExe(QLatin1String("pdftotext"));
    if(!pdf2txtExe.isEmpty()) {
        QString txt;
        QProcess p;
        p.start(pdf2txtExe, QStringList() << path << QLatin1String("-"));
        while(p.waitForReadyRead(-1)) {
            txt.append(QString::fromLocal8Bit(p.readAllStandardOutput()));
        }
        p.waitForFinished(-1);
        return txt.simplified();
    }
    else {
        return QString();
    }
}
