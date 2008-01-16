/*
   $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $

   This file is part of the Strigi project.
   Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>

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
    class FileMetaData
    {
    public:
        // caching URIs for little speed improvement
        QUrl fileUri;
        QUrl context;
        std::string content;
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

    QVariant::Type literalType( const std::string& strigiType ) {
        QHash<std::string, QVariant::Type>::const_iterator it = literalTypes.find( strigiType );
        if ( it == literalTypes.constEnd() ) {
//            qDebug() << "Unknown field type: " << strigiType.c_str() << "falling back to string";
            return QVariant::String;
        }
        else {
            return *it;
        }
    }

    LiteralValue createLiteraValue( const std::string& strigiDataType,
                                    const unsigned char* data,
                                    uint32_t size ) {
        QString value = QString::fromUtf8( ( const char* )data, size );
        QVariant::Type type = literalType( strigiDataType );
        if ( type == QVariant::DateTime ) {
            return LiteralValue( QDateTime::fromTime_t( value.toUInt() ) );
        }
        else {
            return LiteralValue::fromString( value, type );
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
//        QString path = QString::fromUtf8( entries[i].c_str() );
        QString query = QString( "select ?g where { ?r <%1> \"%2\"^^<%3> . "
                                 "?g <http://www.strigi.org/fields#indexGraphFor> ?r . }" )
                        .arg( systemLocationUri )
                        .arg( path )
                        .arg( Vocabulary::XMLSchema::string().toString() );

        qDebug() << "deleteEntries query:" << query;

        QueryResultIterator result = d->repository->executeQuery( query, ::Soprano::Query::QUERY_LANGUAGE_SPARQL );
        if ( result.next() ) {
            Node indexGraph = result.binding( "g" );
            result.close();

            qDebug() << "Found indexGraph to delete:" << indexGraph;

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


// cache the field type mapping in the RegisteredFields
void Strigi::Soprano::IndexWriter::initWriterData( const Strigi::FieldRegister& )
{
    // nothing to do ATM
}


// cleanup field type caching
void Strigi::Soprano::IndexWriter::releaseWriterData( const Strigi::FieldRegister& )
{
    // nothing to do ATM
}


// called for each indexed file
void Strigi::Soprano::IndexWriter::startAnalysis( const AnalysisResult* idx )
{
//    qDebug() << "IndexWriter::startAnalysis in thread" << QThread::currentThread();
    FileMetaData* data = new FileMetaData();
    data->fileUri = Util::fileUrl( idx->path() );

    // let's check if we already have data on the file
    StatementIterator it = d->repository->listStatements( Statement( Node(),
                                                                     QUrl( "http://www.strigi.org/fields#indexGraphFor" ), // FIXME: put the URI somewhere else
                                                                     data->fileUri ) );
    if ( it.next() ) {
        data->context = it.current().subject().uri();
    }
    else {
        data->context = Util::uniqueUri( "http://www.strigi.org/contexts/", d->repository );
    }

    qDebug() << "Starting analysis for" << data->fileUri << "in thread" << QThread::currentThread();

    idx->setWriterData( data );
}


// plain text accociated with the indexed file but no field name.
void Strigi::Soprano::IndexWriter::addText( const AnalysisResult* idx, const char* text, int32_t length )
{
    FileMetaData* md = reinterpret_cast<FileMetaData*>( idx->writerData() );
    md->content.append( text, length );
}


// convenience method for adding string fields
void Strigi::Soprano::IndexWriter::addValue( const AnalysisResult* idx,
                                             const RegisteredField* fieldname,
                                             const std::string& value )
{
    addValue( idx, fieldname, ( unsigned char* )value.c_str(), value.length() );
}


// the main addValue method
void Strigi::Soprano::IndexWriter::addValue( const AnalysisResult* idx,
                                             const RegisteredField* fieldname,
                                             const unsigned char* data,
                                             uint32_t size )
{
    addValue( idx, fieldname, fieldname->key(), std::string( ( const char* )data, size ) );
}


void Strigi::Soprano::IndexWriter::addValue( const AnalysisResult* idx, const RegisteredField* field,
                                             const std::string& name, const std::string& value )
{
//    qDebug() << "IndexWriter::addValue in thread" << QThread::currentThread();
    if ( value.length() > 0 ) {
        FileMetaData* md = reinterpret_cast<FileMetaData*>( idx->writerData() );

        if ( d->literalType( field->type() ) == QVariant::Invalid ) {
            // FIXME: only save it in the index: binary data (how does strigi handle that anyway??)
        }
        else if ( QString( name.c_str() ) == ::Soprano::Vocabulary::RDF::type().toString() ) {
            // Strigi uses rdf:type improperly since it stores the value as a string. We have to
            // make sure it is a resource. The problem is that this results in the type not being
            // indexed properly. Thus, it cannot be searched with normal lucene queries.
            // That is why we need to introduce a stringType property

            d->repository->addStatement( Statement( md->fileUri,
                                                    ::Soprano::Vocabulary::RDF::type(),
                                                    QUrl( QString::fromUtf8( value.c_str() ) ),
                                                    md->context) );
            d->repository->addStatement( Statement( md->fileUri,
                                                    QUrl( "http://strigi.sourceforge.net/fields#rdf-string-type" ),
                                                    d->createLiteraValue( field->type(), ( unsigned char* )value.c_str(), value.length() ),
                                                    md->context) );
        }
        else {
            d->repository->addStatement( Statement( md->fileUri,
                                                    Util::fieldUri( name ),
                                                    d->createLiteraValue( field->type(), ( unsigned char* )value.c_str(), value.length() ),
                                                    md->context) );
        }
    }
//    qDebug() << "IndexWriter::addValue done in thread" << QThread::currentThread();
}


// convenience method for adding unsigned int (or datetime!) fields
void Strigi::Soprano::IndexWriter::addValue( const AnalysisResult* idx,
                                             const RegisteredField* fieldname,
                                             uint32_t value )
{
//    qDebug() << "IndexWriter::addValue in thread" << QThread::currentThread();
    FileMetaData* md = reinterpret_cast<FileMetaData*>( idx->writerData() );
    LiteralValue val( value );
    if ( fieldname->type() == FieldRegister::datetimeType ) {
//        qDebug() << "(Soprano::IndexWriter) adding datetime value.";
        val = QDateTime::fromTime_t( value );
    }

    d->repository->addStatement( Statement( md->fileUri,
                                            Util::fieldUri( fieldname->key() ),
                                            val,
                                            md->context) );
//    qDebug() << "IndexWriter::addValue done in thread" << QThread::currentThread();
}


// convenience method for adding int fields
void Strigi::Soprano::IndexWriter::addValue( const AnalysisResult* idx,
                                             const RegisteredField* fieldname,
                                             int32_t value )
{
//    qDebug() << "IndexWriter::addValue in thread" << QThread::currentThread();
    FileMetaData* md = reinterpret_cast<FileMetaData*>( idx->writerData() );
    d->repository->addStatement( Statement( md->fileUri,
                                            Util::fieldUri( fieldname->key() ),
                                            LiteralValue( value ),
                                            md->context) );
//    qDebug() << "IndexWriter::addValue done in thread" << QThread::currentThread();
}


// convenience method for adding double fields
void Strigi::Soprano::IndexWriter::addValue( const AnalysisResult* idx,
                                             const RegisteredField* fieldname,
                                             double value )
{
//    qDebug() << "IndexWriter::addValue in thread" << QThread::currentThread();
    FileMetaData* md = reinterpret_cast<FileMetaData*>( idx->writerData() );
    d->repository->addStatement( Statement( md->fileUri,
                                            Util::fieldUri( fieldname->key() ),
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
//    qDebug() << "IndexWriter::finishAnalysis in thread" << QThread::currentThread();
    FileMetaData* md = static_cast<FileMetaData*>( idx->writerData() );

    if ( md->content.length() > 0 ) {
        d->repository->addStatement( Statement( md->fileUri,
                                                Util::fieldUri( FieldRegister::contentFieldName ),
                                                LiteralValue( QString::fromUtf8( md->content.c_str() ) ),
                                                md->context ) );
    }

    // Strigi only indexes files and extractors mostly (if at all) store the xesam:DataObject type (i.e. the contents)
    // Thus, here we go the easy way and amrk each indexed file as a xesam:File.
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
                                            QUrl( "http://www.strigi.org/fields#indexGraphFor" ), // FIXME: put the URI somewhere else
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
