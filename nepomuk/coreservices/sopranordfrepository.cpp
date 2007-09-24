/*
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006 Daniele Galdi <daniele.galdi@gmail.com>
 * Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "sopranordfrepository.h"

#include "cache.h"
#include "uniqueurigenerator.h"
#include "querydefinition.h"

#include <QtCore/QtGlobal>
#include <QtCore/QDateTime>
#include <QtCore/QDir>

#include <soprano/soprano.h>
#include <soprano/cluceneindex.h>
#include <nepomuk/error.h>

#include <kstandarddirs.h>
#include <kdebug.h>
#include <klocale.h>
#include <kio/deletejob.h>


class Nepomuk::CoreServices::SopranoRDFRepository::Private
{
public:
    Private() : resolver(0L), system(0L)
    {}

    RepositoryMap *resolver;
    Soprano::Model *system;

    UniqueUriGenerator uug;
    Cache cache;
};

Nepomuk::CoreServices::SopranoRDFRepository::SopranoRDFRepository( Soprano::Model *system,
								   RepositoryMap *resolver )
    : RDFRepositoryPublisher( "SopranoRDFRepository", "file://tmp/soprano/services/SopranoRDFRepository" )
{
    d = new Private;
    d->system = system;
    d->resolver = resolver;
}


Nepomuk::CoreServices::SopranoRDFRepository::~SopranoRDFRepository()
{
    delete d;
}


void Nepomuk::CoreServices::SopranoRDFRepository::createRepository( const QString& graphId )
{
    // FIXME: this method is not thread-safe!
    if ( d->resolver->contains( graphId ) ) {
        kDebug(300002) << ": " << graphId << " already present!";
        return;
    }

    if( graphId.contains( '/' ) ) {
        kDebug(300002) << ": " << "Invalid repository name: " << graphId;
        setError( "org.semanticdesktop.nepomuk.error.InvalidParameter",
                  i18n("Invalid repository name: %1", graphId) );
        return;
    }

    QString storagePath = createStoragePath( graphId );

    Repository* newRepo = Repository::open( storagePath, graphId );
    if( newRepo ) {
        QUrl repositoryUri( "http://nepomuk.semanticdesktop.org/repositories/localhost/" + graphId );

        //
        // For easier handling we put all statements into one named graph although that might be redundant
        //
        d->system->addStatement( Soprano::Statement( repositoryUri,
                                                     QUrl("http://nepomuk-kde.semanticdesktop.org/rdfrepository#created"),
                                                     Soprano::LiteralValue( QDateTime::currentDateTime() ),
                                                     repositoryUri ) );
        d->system->addStatement( Soprano::Statement( repositoryUri,
                                                     QUrl("http://www.w3.org/1999/02/22-rdf-syntax-ns#type"),
                                                     QUrl( "http://www.soprano.org/types#Model" ),
                                                     repositoryUri ) );
        d->system->addStatement( Soprano::Statement( repositoryUri,
                                                     QUrl("http://www.w3.org/2000/01/rdf-schema#label"),
                                                     Soprano::LiteralValue( graphId ),
                                                     repositoryUri ) );
        d->system->addStatement( Soprano::Statement( repositoryUri,
                                                     QUrl("http://nepomuk-kde.semanticdesktop.org/rdfrepository#hasStoragePath"),
                                                     Soprano::LiteralValue( storagePath ),
                                                     repositoryUri ) );

        d->resolver->insert( graphId, newRepo );
    }
    else {
        // FIXME: use the Soprano error codes and messages
        setError( "org.semanticdesktop.nepomuk.error.UnknownError",
                  i18n("Failed to create a new repository.") );
    }
}


QStringList Nepomuk::CoreServices::SopranoRDFRepository::listRepositoryIds( )
{
    return d->resolver->keys();

    // Return a list of all graphId from system model
//     Soprano::QueryLegacy query( QueryDefinition::FIND_GRAPHS, Soprano::QueryLegacy::RDQL );

//     Soprano::QueryResultIterator res = d->system->executeQuery( query );

//     QStringList graphs;
//     while ( res.next() )
//     {
//         Soprano::Node id = res.binding( "modelId" );
//         graphs << id.literal().toString();
//     }

//     return graphs;
}


void Nepomuk::CoreServices::SopranoRDFRepository::removeRepository( const QString& repositoryId )
{
    if ( d->resolver->contains( repositoryId ) ) {
        // cleanup the local data
        Repository* repo = d->resolver->value( repositoryId );
        d->resolver->remove( repositoryId );
        delete repo;

        // remove the metadata
        d->system->removeContext( QUrl( "http://nepomuk.semanticdesktop.org/repositories/localhost/" + repositoryId ) );

        // delete the actual data
        // FIXME: check the return state of the job
        KIO::del( createStoragePath( repositoryId ) );
    }
    else {
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + repositoryId );
    }
}


int Nepomuk::CoreServices::SopranoRDFRepository::getRepositorySize( const QString& graphId )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << ": " << " repository " << graphId << " not found!";
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
        return 0;
    }

    Soprano::Model *model = d->resolver->value( graphId )->model();
    return model->statementCount();
}


int Nepomuk::CoreServices::SopranoRDFRepository::contains( const QString& graphId, const Soprano::Statement& statement )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << ": " << " repository " << graphId << " not found!";
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
        return 0;
    }

    Soprano::Model *model = d->resolver->value( graphId )->model();
    if ( model->containsAnyStatement( statement ) ) {
        return 1;
    }
    else {
        return 0;
    }
}


void Nepomuk::CoreServices::SopranoRDFRepository::addStatement( const QString& graphId, const Soprano::Statement& statement )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << ": " << " repository " << graphId << " not found!";
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
    }
    else {
        Soprano::Model *model = d->resolver->value( graphId )->model();

        if ( model->addStatement( statement ) ) {
            setError( "org.semanticdesktop.nepomuk.error.UnknownError",
                      "Failed to add statement ["
                      + statement.subject().toString() + ','
                      + statement.predicate().toString() + ','
                      + statement.object().toString() + ']' );
        }
    }
}


void Nepomuk::CoreServices::SopranoRDFRepository::addStatements( const QString& graphId, const QList<Soprano::Statement>& statements )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << ": " << " repository " << graphId << " not found!";
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
    }
    else {
        Soprano::Model *model = d->resolver->value( graphId )->model();

        QList<Soprano::Statement> ssl;
        for( QList<Soprano::Statement>::const_iterator it = statements.constBegin();
             it != statements.constEnd(); ++it ) {
            if( Soprano::Error::ErrorCode r = model->addStatement( *it ) ) {
                kDebug(300002) << " failed to add " << (*it).subject().toString();
                setError( "org.semanticdesktop.nepomuk.error.UnknownError",
                          '(' + Soprano::Error::errorMessage(r) + ") Failed to add statement ["
                          + (*it).subject().toString() + ','
                          + (*it).predicate().toString() + ','
                          + (*it).object().toString() + ']' );
            }
        }
    }
}


int Nepomuk::CoreServices::SopranoRDFRepository::removeStatement( const QString& graphId, const Soprano::Statement& statement )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << ": " << " repository " << graphId << " not found!";
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
        return 0;
    }

    Soprano::Model *model = d->resolver->value( graphId )->model();

    if( model->removeStatement( statement ) == Soprano::Error::ErrorNone ) {
        return 1;
    }
    else
        return 0;
}


int Nepomuk::CoreServices::SopranoRDFRepository::removeStatements( const QString& graphId, const QList<Soprano::Statement>& statements )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << ": " << " repository " << graphId << " not found!";
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
        return 0;
    }

    Soprano::Model *model = d->resolver->value( graphId )->model();

    int cnt = 0;

    QList<Soprano::Statement> ssl;
    for( QList<Soprano::Statement>::const_iterator it = statements.constBegin();
         it != statements.constEnd(); ++it ) {
        if( model->removeStatement( *it ) == Soprano::Error::ErrorNone ) {
            ++cnt;
        }
    }

    return cnt;
}


int Nepomuk::CoreServices::SopranoRDFRepository::removeAllStatements( const QString& graphId, const Soprano::Statement& statement )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << ": " << " repository " << graphId << " not found!";
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
        return 0;
    }

    Soprano::Model *model = d->resolver->value( graphId )->model();
    if ( model->removeAllStatements( statement ) == Soprano::Error::ErrorNone ) {
        // FIXME
        return 1;
    }
    else {
        setError( "org.semanticdesktop.nepomuk.error.UnknownError",
                  QString( "Failed to remove statements" ) );
        return 0;
    }
}


void Nepomuk::CoreServices::SopranoRDFRepository::removeContext( const QString& graphId, const Soprano::Node& context )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << ": " << " repository " << graphId << " not found!";
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
        return;// Error::ERROR_FAILURE;
    }

    Soprano::Model *model = d->resolver->value( graphId )->model();

    if ( model->removeContext( context ) != Soprano::Error::ErrorNone ) {
        setError( "org.semanticdesktop.nepomuk.error.UnknownError",
                  "Failed to remove context " + context.uri().toString() );
    }
}


QList<Soprano::Statement>
Nepomuk::CoreServices::SopranoRDFRepository::listStatements( const QString& graphId, const Soprano::Statement& statement )
{
//    kDebug(300002) << graphId << ", " << statement<< flush;

    QList<Soprano::Statement> stmList;

    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << ": " << " repository " << graphId << " not found!";
    }
    else {
        Soprano::Model *model = d->resolver->value( graphId )->model();
        stmList = model->listStatements( statement ).allStatements();
    }

    return stmList;
}


QList<Soprano::Statement> Nepomuk::CoreServices::SopranoRDFRepository::constructSparql( const QString& repositoryId, const QString& query )
{
    return construct( repositoryId, query, "sparql" );
}


Nepomuk::RDF::QueryResultTable Nepomuk::CoreServices::SopranoRDFRepository::selectSparql( const QString& repositoryId, const QString& query )
{
    return select( repositoryId, query, "sparql" );
}


QList<Soprano::Statement> Nepomuk::CoreServices::SopranoRDFRepository::describeSparql( const QString& repositoryId, const QString& query )
{
    QList<Soprano::Statement> stmList;

    Soprano::QueryResultIterator r = executeQuery( repositoryId, query, "sparql" );
    while ( r.next() ) {
        stmList.append( r.currentStatement() );
    }

    return stmList;
}


QList<Soprano::Statement> Nepomuk::CoreServices::SopranoRDFRepository::construct( const QString& repositoryId, const QString& query,
                                                                                  const QString& querylanguage )
{
    QList<Soprano::Statement> stmList;

    Soprano::QueryResultIterator r = executeQuery( repositoryId, query, querylanguage );
    while ( r.next() ) {
        stmList.append( r.currentStatement() );
    }

    return stmList;
}


Nepomuk::RDF::QueryResultTable Nepomuk::CoreServices::SopranoRDFRepository::select( const QString& repositoryId, const QString& query,
										    const QString& querylanguage )
{
    Soprano::QueryResultIterator r = executeQuery( repositoryId, query, querylanguage );
    RDF::QueryResultTable rt;
    rt.columns = r.bindingNames();

    while( r.next() ) {
        QList<Soprano::Node> row;
        for( int i = 0; i < r.bindingCount(); ++i )
            row.append( r.binding( i ) );
        rt.rows.append( row );
    }

    return rt;
}


int Nepomuk::CoreServices::SopranoRDFRepository::queryListStatements( const QString& repositoryId, const Soprano::Statement& statement,
								      int timeoutMSec )
{
//    kDebug(300002) << repositoryId << ", " << statement<< flush;

    if ( !d->resolver->contains( repositoryId ) ) {
        kDebug(300002) << ": " << " repository " << repositoryId << " not found!";
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + repositoryId );
        return 0;
    }
    else {
        Soprano::Model *model = d->resolver->value( repositoryId )->model();
        Soprano::StatementIterator iter = model->listStatements( statement );

        return d->cache.insert( iter, timeoutMSec );
    }
}


int Nepomuk::CoreServices::SopranoRDFRepository::queryConstruct( const QString& repositoryId, const QString& query,
								 const QString& querylanguage, int timeoutMSec )
{
    if ( !d->resolver->contains( repositoryId ) ) {
        kDebug(300002) << ": " << " repository " << repositoryId << " not found!";
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + repositoryId );
        return 0;
    }
    else {
        return d->cache.insert( executeQuery( repositoryId, query, querylanguage ), timeoutMSec );
    }
}


int Nepomuk::CoreServices::SopranoRDFRepository::querySelect( const QString& repositoryId, const QString& query,
							      const QString& querylanguage, int timeoutMSec )
{
    if ( !d->resolver->contains( repositoryId ) ) {
        kDebug(300002) << ": " << " repository " << repositoryId << " not found!";
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + repositoryId );
        return 0;
    }
    else {
        return d->cache.insert( executeQuery( repositoryId, query, querylanguage ), timeoutMSec );
    }
}


int Nepomuk::CoreServices::SopranoRDFRepository::queryConstructSparql( const QString& repositoryId, const QString& query,
								       int timeoutMSec )
{
    return queryConstruct( repositoryId, query, "sparql", timeoutMSec );
}


int Nepomuk::CoreServices::SopranoRDFRepository::querySelectSparql( const QString& repositoryId, const QString& query,
								    int timeoutMSec )
{
    return querySelect( repositoryId, query, "sparql", timeoutMSec );
}


int Nepomuk::CoreServices::SopranoRDFRepository::queryDescribeSparql( const QString& repositoryId, const QString& query,
								      int timeoutMSec )
{
    if ( !d->resolver->contains( repositoryId ) ) {
        kDebug(300002) << ": " << " repository " << repositoryId << " not found!";
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + repositoryId );
        return 0;
    }
    else {
        return d->cache.insert( executeQuery( repositoryId, query, "sparql" ), timeoutMSec );
    }
}


int Nepomuk::CoreServices::SopranoRDFRepository::askSparql( const QString& repositoryId, const QString& query )
{
    if ( !d->resolver->contains( repositoryId ) ) {
        kDebug(300002) << ": " << " repository " << repositoryId << " not found!";
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + repositoryId );
        return 0;
    }
    else {
        Soprano::QueryResultIterator r = executeQuery( repositoryId, query, "sparql" );
        return ( r.boolValue() ? 1 : 0 );
    }
}


QList<Soprano::Statement> Nepomuk::CoreServices::SopranoRDFRepository::fetchListStatementsResults( int queryId, int max )
{
    QList<Soprano::Statement> sl;
    if( d->cache.contains( queryId ) ) {
        Soprano::StatementIterator it = d->cache.getStatements( queryId );
        int cnt = 0;
        while( it.next() && ( cnt < max || max == 0 ) ) {
            sl.append( *it );
            ++cnt;
        }

        if( cnt < max )
            closeQuery( queryId );
    }
    else {
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.InvalidQueryId",
                  QString( "Could not find query id %1" ).arg( queryId ) );
    }

    return sl;
}


QList<Soprano::Statement> Nepomuk::CoreServices::SopranoRDFRepository::fetchConstructResults( int queryId, int max )
{
    return fetchListStatementsResults( queryId, max );
}


QList<Soprano::Statement> Nepomuk::CoreServices::SopranoRDFRepository::fetchDescribeResults( int queryId, int max )
{
    return fetchListStatementsResults( queryId, max );
}


Nepomuk::RDF::QueryResultTable Nepomuk::CoreServices::SopranoRDFRepository::fetchSelectResults( int queryId, int max )
{
    RDF::QueryResultTable rt;

    if( d->cache.contains( queryId ) ) {
        Soprano::QueryResultIterator rs = d->cache.getResultSet( queryId );

        rt.columns = rs.bindingNames();

        int cnt = 0;
        while( ( cnt < max || max == 0 ) && rs.next() ) {
            QList<Soprano::Node> row;
            for( int i = 0; i < rs.bindingCount(); ++i )
                row.append( rs.binding( i ) );
            rt.rows.append( row );

            ++cnt;
        }

        if( cnt < max )
            closeQuery( queryId );
    }
    else {
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.InvalidQueryId",
                  QString( "Could not find query id %1" ).arg( queryId ) );
        kDebug( 300002 ) << " invalid query id: " << queryId;
    }

    return rt;
}


void Nepomuk::CoreServices::SopranoRDFRepository::closeQuery( int listId )
{
    //  kDebug(300002) << ": " << listId;

    d->cache.remove( listId );
}


QStringList Nepomuk::CoreServices::SopranoRDFRepository::supportedQueryLanguages()
{
    QStringList tmp;
    tmp << "rdql";
    tmp << "sparql";

    return tmp;
}

int Nepomuk::CoreServices::SopranoRDFRepository::supportsQueryLanguage( const QString& lang )
{
    return( ( lang.toLower() == "rdql" || lang.toLower() == "sparql" ) ? 1 : 0 );
}


QStringList Nepomuk::CoreServices::SopranoRDFRepository::supportedSerializations()
{
    return QStringList();
}


int Nepomuk::CoreServices::SopranoRDFRepository::supportsSerialization( const QString& serializationMimeType )
{
    return 0;
}


void Nepomuk::CoreServices::SopranoRDFRepository::addGraph( const QString& repositoryId, const QString& graph,
							    const QString& formatMimetype, const Soprano::Node& context )
{
    // FIXME: implement addGraph
}


void Nepomuk::CoreServices::SopranoRDFRepository::removeGraph( const QString& repositoryId, const QString& graph,
							       const QString& formatMimetype, const Soprano::Node& context )
{
    // FIXME: implement removeGraph
}


Soprano::QueryResultIterator Nepomuk::CoreServices::SopranoRDFRepository::executeQuery( const QString& graphId,
                                                                                        const QString& query,
                                                                                        const QString& queryType )
{
    Soprano::QueryResultIterator result;

    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << ": " << " repository " << graphId << " not found!";
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
    }
    else {
        Soprano::Model *model = d->resolver->value( graphId )->model();

        Soprano::Query::QueryLanguage sopranoQueryType( Soprano::Query::QUERY_LANGUAGE_SPARQL );

        if( queryType.toLower() == QString("rdql") )
            sopranoQueryType = Soprano::Query::QUERY_LANGUAGE_RDQL;
        else if( queryType.toLower() != QString("sparql") ) {
            kDebug(300002) << ": " << "Unsupported query language: " << queryType;
        }
        else
            result = model->executeQuery( query, sopranoQueryType );
    }

    return result;
}


void Nepomuk::CoreServices::SopranoRDFRepository::dumpGraph( const QString& graphId )
{
    if ( graphId == "system" ) {
        QTextStream s( stdout );
        d->system->write( s );
    }
    else if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << ": " << " repository " << graphId << " not found!";
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
    }
    else {
        kDebug(300002) << ": " << "(SopranoRDFRepository) Listing statements in " << graphId;
        Soprano::Model *model = d->resolver->value( graphId )->model();
        QTextStream s( stdout );
        model->write( s );
    }
}


void Nepomuk::CoreServices::SopranoRDFRepository::dumpIndex( const QString& graphId )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
    }
    else {
        QTextStream s( stdout );
        d->resolver->value( graphId )->index()->dump( s );
    }
}


QString Nepomuk::CoreServices::SopranoRDFRepository::createStoragePath( const QString& repositoryId ) const
{
    // FIXME: use KStandardDirs properly to get the data path
    return QDir::homePath() + "/.nepomuk/share/storage/" + repositoryId;
}

#include "sopranordfrepository.moc"
