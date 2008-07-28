/*
   Copyright (C) 2007-2008 Sebastian Trueg <trueg@kde.org>

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

#include <Soprano/Soprano>
#include <Soprano/Index/IndexFilterModel>
#include <Soprano/Index/CLuceneIndex>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/Xesam>
#include <Soprano/LiteralValue>

#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QVariant>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QUrl>
#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QDateTime>

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <map>
#include <sstream>
#include <algorithm>


// IMPORTANT: strings in Strigi are apparently UTF8! Except for file names. Those are in local encoding.

using namespace Soprano;


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
        QString path = QFile::decodeName( idx->path().c_str() );
        QUrl url = QUrl::fromLocalFile( QFileInfo( path ).absoluteFilePath() );
        if ( idx->depth() > 0 ) {
            QString archivePath = findArchivePath( path );
            if ( QFile::exists( archivePath ) ) {
                if ( archivePath.endsWith( QLatin1String( ".tar" ) ) ||
                     archivePath.endsWith( QLatin1String( ".tar.gz" ) ) ||
                     archivePath.endsWith( QLatin1String( ".tar.bz2" ) ) ) {
                    url.setScheme( "tar" );
                }
                else if ( archivePath.endsWith( QLatin1String( ".zip" ) ) ) {
                    url.setScheme( "zip" );
                }
            }
        }

        // fallback for all
        if ( url.scheme().isEmpty() ) {
            url.setScheme( "file" );
        }

        return url;
    }

    class FileMetaData
    {
    public:
        // caching URIs for little speed improvement
        QUrl fileUri;
        QUrl context;
        std::string content;
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
        QHash<std::string, QVariant::Type>::const_iterator it = literalTypes.find( strigiType.typeUri() );
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

//    ::Soprano::Index::IndexFilterModel* repository;
    ::Soprano::Model* repository;
    int indexTransactionID;

private:
    QHash<std::string, QVariant::Type> literalTypes;
};


Strigi::Soprano::IndexWriter::IndexWriter( ::Soprano::Model* model )
    : Strigi::IndexWriter()
{
//    qDebug() << "IndexWriter::IndexWriter in thread" << QThread::currentThread();
    d = new Private;
    d->repository = model;
    Util::storeStrigiMiniOntology( d->repository );
//    qDebug() << "IndexWriter::IndexWriter done in thread" << QThread::currentThread();
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
//    qDebug() << "IndexWriter::deleteEntries in thread" << QThread::currentThread();

    QString systemLocationUri = Util::fieldUri( FieldRegister::pathFieldName ).toString();
    for ( unsigned int i = 0; i < entries.size(); ++i ) {
        QString path = QString::fromUtf8( entries[i].c_str() );
        QString query = QString( "select ?g where { ?r <%1> \"%2\"^^<%3> . "
                                 "?g <http://www.strigi.org/fields#indexGraphFor> ?r . }" )
                        .arg( systemLocationUri )
                        .arg( path )
                        .arg( Vocabulary::XMLSchema::string().toString() );

//        qDebug() << "deleteEntries query:" << query;

        QueryResultIterator result = d->repository->executeQuery( query, ::Soprano::Query::QUERY_LANGUAGE_SPARQL );
        if ( result.next() ) {
            Node indexGraph = result.binding( "g" );
            result.close();

//            qDebug() << "Found indexGraph to delete:" << indexGraph;

            // delete the indexed data
            d->repository->removeContext( indexGraph );

            // delete the metadata
            d->repository->removeAllStatements( Statement( indexGraph, Node(), Node() ) );
        }
    }
}


void Strigi::Soprano::IndexWriter::deleteAllEntries()
{
//    qDebug() << "IndexWriter::deleteAllEntries in thread" << QThread::currentThread();

    // query all index graphs (FIXME: would a type derived from nrl:Graph be better than only the predicate?)
    QString query = QString( "select ?g where { ?g <http://www.strigi.org/fields#indexGraphFor> ?r . }" );

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
    if ( idx->depth() > 0 ) {
        return;
    }

//    qDebug() << "IndexWriter::startAnalysis in thread" << QThread::currentThread();
    FileMetaData* data = new FileMetaData();
    data->fileUri = createResourceUri( idx );

    // let's check if we already have data on the file
    StatementIterator it = d->repository->listStatements( Node(),
                                                          QUrl::fromEncoded( "http://www.strigi.org/fields#indexGraphFor", QUrl::StrictMode ), // FIXME: put the URI somewhere else
                                                          data->fileUri );
    if ( it.next() ) {
        data->context = it.current().subject().uri();
    }
    else {
        data->context = Util::uniqueUri( "http://www.strigi.org/contexts/", d->repository );
    }

//    qDebug() << "Starting analysis for" << data->fileUri << "in thread" << QThread::currentThread();

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

//    qDebug() << "IndexWriter::addValue in thread" << QThread::currentThread();
    if ( value.length() > 0 ) {
        FileMetaData* md = reinterpret_cast<FileMetaData*>( idx->writerData() );
        RegisteredFieldData* rfd = reinterpret_cast<RegisteredFieldData*>( field->writerData() );

        if ( rfd->isRdfType ) {

            // Strigi uses rdf:type improperly since it stores the value as a string. We have to
            // make sure it is a resource. The problem is that this results in the type not being
            // indexed properly. Thus, it cannot be searched with normal lucene queries.
            // That is why we need to introduce a stringType property

            d->repository->addStatement( Statement( md->fileUri,
                                                    ::Soprano::Vocabulary::RDF::type(),
                                                    QUrl::fromEncoded( value.c_str(), QUrl::StrictMode ), // fromEncoded is faster than the plain constructor and all Xesam URIs work here
                                                    md->context) );
            d->repository->addStatement( Statement( md->fileUri,
                                                    QUrl::fromEncoded( "http://strigi.sourceforge.net/fields#rdf-string-type", QUrl::StrictMode ),
                                                    LiteralValue( QString::fromUtf8( value.c_str() ) ),
                                                    md->context) );
        }

        else {
            d->repository->addStatement( Statement( md->fileUri,
                                                    rfd->property,
                                                    d->createLiteralValue( rfd->dataType, ( unsigned char* )value.c_str(), value.length() ),
                                                    md->context) );
        }
    }
//    qDebug() << "IndexWriter::addValue done in thread" << QThread::currentThread();
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

//    qDebug() << "IndexWriter::addValue in thread" << QThread::currentThread();
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
//    qDebug() << "IndexWriter::addValue done in thread" << QThread::currentThread();
}


void Strigi::Soprano::IndexWriter::addValue( const AnalysisResult* idx,
                                             const RegisteredField* field,
                                             int32_t value )
{
    if ( idx->depth() > 0 ) {
        return;
    }

//    qDebug() << "IndexWriter::addValue in thread" << QThread::currentThread();
    FileMetaData* md = reinterpret_cast<FileMetaData*>( idx->writerData() );
    RegisteredFieldData* rfd = reinterpret_cast<RegisteredFieldData*>( field->writerData() );

    d->repository->addStatement( Statement( md->fileUri,
                                            rfd->property,
                                            LiteralValue( value ),
                                            md->context) );
//    qDebug() << "IndexWriter::addValue done in thread" << QThread::currentThread();
}


void Strigi::Soprano::IndexWriter::addValue( const AnalysisResult* idx,
                                             const RegisteredField* field,
                                             double value )
{
    if ( idx->depth() > 0 ) {
        return;
    }

//    qDebug() << "IndexWriter::addValue in thread" << QThread::currentThread();
    FileMetaData* md = reinterpret_cast<FileMetaData*>( idx->writerData() );
    RegisteredFieldData* rfd = reinterpret_cast<RegisteredFieldData*>( field->writerData() );

    d->repository->addStatement( Statement( md->fileUri,
                                            rfd->property,
                                            LiteralValue( value ),
                                            md->context) );
//    qDebug() << "IndexWriter::addValue done in thread" << QThread::currentThread();
}


void Strigi::Soprano::IndexWriter::addTriplet( const std::string& subject,
                                               const std::string& predicate, const std::string& object )
{
    // PROBLEM: which named graph (context) should we use here? Create a new one for each triple? Use one until the
    // next commit()?

    // FIXME: create an NRL metadata graph
    d->repository->addStatement( Statement( Node( QUrl( QString::fromUtf8( subject.c_str() ) ) ),
                                            Node( QUrl( QString::fromUtf8( predicate.c_str() ) ) ),
                                            Node( QUrl( QString::fromUtf8( object.c_str() ) ) ),
                                            Node() ) );
}


// called after each indexed file
void Strigi::Soprano::IndexWriter::finishAnalysis( const AnalysisResult* idx )
{
    if ( idx->depth() > 0 ) {
        return;
    }

//    qDebug() << "IndexWriter::finishAnalysis in thread" << QThread::currentThread();
    FileMetaData* md = static_cast<FileMetaData*>( idx->writerData() );

    if ( md->content.length() > 0 ) {
        d->repository->addStatement( Statement( md->fileUri,
                                                Vocabulary::Xesam::asText(),
                                                LiteralValue( QString::fromUtf8( md->content.c_str() ) ),
                                                md->context ) );
    }

    // Strigi only indexes files and extractors mostly (if at all) store the xesam:DataObject type (i.e. the contents)
    // Thus, here we go the easy way and mark each indexed file as a xesam:File.
    if ( QFileInfo( QFile::decodeName( idx->path().c_str() ) ).isDir() )
        d->repository->addStatement( Statement( md->fileUri,
                                                Vocabulary::RDF::type(),
                                                Vocabulary::Xesam::Folder(),
                                                md->context ) );
    else
        d->repository->addStatement( Statement( md->fileUri,
                                                Vocabulary::RDF::type(),
                                                Vocabulary::Xesam::File(),
                                                md->context ) );


    // create the provedance data for the data graph
    // TODO: add more data at some point when it becomes of interest
    QUrl metaDataContext = Util::uniqueUri( "http://www.strigi.org/graphMetaData/", d->repository );
    d->repository->addStatement( Statement( md->context,
                                            Vocabulary::RDF::type(),
                                            Vocabulary::NRL::InstanceBase(),
                                            metaDataContext ) );
    d->repository->addStatement( Statement( md->context,
                                            Vocabulary::NAO::created(),
                                            LiteralValue( QDateTime::currentDateTime() ),
                                            metaDataContext ) );
    d->repository->addStatement( Statement( md->context,
                                            QUrl::fromEncoded( "http://www.strigi.org/fields#indexGraphFor", QUrl::StrictMode ), // FIXME: put the URI somewhere else
                                            md->fileUri,
                                            metaDataContext ) );
    d->repository->addStatement( Statement( metaDataContext,
                                            Vocabulary::RDF::type(),
                                            Vocabulary::NRL::GraphMetadata(),
                                            metaDataContext ) );

    // cleanup
    delete md;
    idx->setWriterData( 0 );

//    qDebug() << "IndexWriter::finishAnalysis done in thread" << QThread::currentThread();
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
