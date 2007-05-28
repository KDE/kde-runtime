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
#include <knepomuk/error.h>

#include <kstandarddirs.h>
#include <kdebug.h>
#include <klocale.h>
#include <kio/deletejob.h>


class Nepomuk::CoreServices::SopranoRDFRepository::Private
{
public:
    Private() : resolver(0L), system(0L)
    {}

    QMap<QString, Soprano::Model *> *resolver;
    Soprano::Model *system;

    UniqueUriGenerator uug;
    Cache cache;
};

Nepomuk::CoreServices::SopranoRDFRepository::SopranoRDFRepository( Soprano::Model *system,
								   QMap<QString, Soprano::Model *> *resolver )
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
    if ( d->resolver->contains( graphId ) ) {
        kDebug(300002) << k_funcinfo << ": " << graphId << " already present!" << endl;
        return;
    }

    if( graphId.contains( '/' ) ) {
        kDebug(300002) << k_funcinfo << ": " << "Invalid repository name: " << graphId << endl;
        setError( "org.semanticdesktop.nepomuk.error.InvalidParameter",
                  i18n("Invalid repository name: %1", graphId) );
        return;
    }

    QString storagePath = createStoragePath( graphId );

    KStandardDirs::makeDir( storagePath );

    // FIXME: add configuration for stuff like this
    Soprano::Model* m = Soprano::createModel( graphId, QString( "new=no,dir="+ storagePath ).split(",") );
    if( m ) {
        QUrl repositoryUri( "http://nepomuk.semanticdesktop.org/repositories/localhost/" + graphId );

        //
        // For easier handling we put all statements into one named graph although that might be redundant
        //
        d->system->add( Soprano::Statement( repositoryUri,
                                            QUrl("http://nepomuk-kde.semanticdesktop.org/rdfrepository#created"),
                                            Soprano::LiteralValue( QDateTime::currentDateTime() ),
                                            repositoryUri ) );
        d->system->add( Soprano::Statement( repositoryUri,
                                            QUrl("http://www.w3.org/1999/02/22-rdf-syntax-ns#type"),
                                            QUrl( "http://www.soprano.org/types#Model" ),
                                            repositoryUri ) );
        d->system->add( Soprano::Statement( repositoryUri,
                                            QUrl("http://www.w3.org/2000/01/rdf-schema#label"),
                                            Soprano::LiteralValue( graphId ),
                                            repositoryUri ) );
        d->system->add( Soprano::Statement( repositoryUri,
                                            QUrl("http://nepomuk-kde.semanticdesktop.org/rdfrepository#hasStoragePath"),
                                            Soprano::LiteralValue( storagePath ),
                                            repositoryUri ) );

        d->resolver->insert( graphId, m );
    }
    else {
        // FIXME: use the Soprano error codes and messages
        setError( "org.semanticdesktop.nepomuk.error.UnknownError",
                  i18n("Failed to create a new repository.") );
    }
}


QStringList Nepomuk::CoreServices::SopranoRDFRepository::listRepositoriyIds( )
{
    return d->resolver->keys();

    // Return a list of all graphId from system model
//     Soprano::Query query( QueryDefinition::FIND_GRAPHS, Soprano::Query::RDQL );

//     Soprano::ResultSet res = d->system->executeQuery( query );

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
        Soprano::Model* model = d->resolver->value( repositoryId );
        d->resolver->remove( repositoryId );
        delete model;

        // remove the metadata
        d->system->remove( QUrl( "http://nepomuk.semanticdesktop.org/repositories/localhost/" + repositoryId ) );

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
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
        return 0;
    }

    Soprano::Model *model = d->resolver->value( graphId );
    return model->size();
}


int Nepomuk::CoreServices::SopranoRDFRepository::contains( const QString& graphId, const Soprano::Statement& statement )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
        return 0;
    }

    Soprano::Model *model = d->resolver->value( graphId );
    if ( model->contains( statement ) ) {
        return 1;
    }
    else {
        return 0;
    }
}


void Nepomuk::CoreServices::SopranoRDFRepository::addStatement( const QString& graphId, const Soprano::Statement& statement )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
    }
    else {
        if ( !d->resolver->contains( "index" ) ) {
            createRepository( "index" );
        }

        Soprano::Model *model = d->resolver->value( graphId );

        if ( model->add( statement ) ) {
            setError( "org.semanticdesktop.nepomuk.error.UnknownError",
                      "Failed to add statement ["
                      + statement.subject().toString() + ','
                      + statement.predicate().toString() + ','
                      + statement.object().toString() + ']' );
        }
        else {
            d->resolver->value( "index" )->add( buildIndexGraph( statement ) );
        }
    }
}


void Nepomuk::CoreServices::SopranoRDFRepository::addStatements( const QString& graphId, const QList<Soprano::Statement>& statements )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
    }
    else {
        if ( !d->resolver->contains( "index" ) ) {
            createRepository( "index" );
        }

        Soprano::Model *model = d->resolver->value( graphId );

        QList<Soprano::Statement> ssl;
        for( QList<Soprano::Statement>::const_iterator it = statements.constBegin();
             it != statements.constEnd(); ++it ) {
            if( Soprano::ErrorCode r = model->add( *it ) ) {
                kDebug(300002) << k_funcinfo << " failed to add " << *it << endl;
                setError( "org.semanticdesktop.nepomuk.error.UnknownError",
                          '(' + Soprano::errorMessage(r) + ") Failed to add statement ["
                          + (*it).subject().toString() + ','
                          + (*it).predicate().toString() + ','
                          + (*it).object().toString() + ']' );
            }
            else {
                d->resolver->value( "index" )->add( buildIndexGraph( *it ) );
            }
        }
    }
}


int Nepomuk::CoreServices::SopranoRDFRepository::removeStatement( const QString& graphId, const Soprano::Statement& statement )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
        return 0;
    }

    Soprano::Model *model = d->resolver->value( graphId );

    if( model->remove( statement ) == Soprano::ERROR_NONE ) {
        if ( d->resolver->contains( "index" ) ) {
            d->resolver->value( "index" )->remove( buildIndexGraph( statement ) );
        }
        return 1;
    }
    else
        return 0;
}


int Nepomuk::CoreServices::SopranoRDFRepository::removeStatements( const QString& graphId, const QList<Soprano::Statement>& statements )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
        return 0;
    }

    Soprano::Model *model = d->resolver->value( graphId );

    int cnt = 0;

    QList<Soprano::Statement> ssl;
    for( QList<Soprano::Statement>::const_iterator it = statements.constBegin();
         it != statements.constEnd(); ++it ) {
        if( model->remove( *it ) == Soprano::ERROR_NONE ) {
            ++cnt;
            if ( d->resolver->contains( "index" ) ) {
                d->resolver->value( "index" )->remove( buildIndexGraph( *it ) );
            }
        }
    }

    return cnt;
}


int Nepomuk::CoreServices::SopranoRDFRepository::removeAllStatements( const QString& graphId, const Soprano::Statement& statement )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
        return 0;
    }

    Soprano::Model *model = d->resolver->value( graphId );
    Soprano::StatementIterator iter = model->listStatements( statement );
    int cnt = 0;
    int errorCnt = 0;

    while ( iter.hasNext() ) {
        Soprano::Statement s = iter.next();
        if ( model->remove( s ) == Soprano::ERROR_NONE ) {
            ++cnt;
            if ( d->resolver->contains( "index" ) ) {
                d->resolver->value( "index" )->remove( buildIndexGraph( s ) );
            }
        }
        else {
            ++errorCnt;
        }
    }

    if ( errorCnt ) {
        setError( "org.semanticdesktop.nepomuk.error.UnknownError",
                  QString( "Failed to remove %1 of %2 statements" ).arg( errorCnt ).arg( errorCnt+cnt ) );
    }

    return cnt;
}


void Nepomuk::CoreServices::SopranoRDFRepository::removeContext( const QString& graphId, const Soprano::Node& context )
{
    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
        return;// ERROR_FAILURE;
    }

    Soprano::Model *model = d->resolver->value( graphId );

    // all index statements have the same context as the statement they were created from
    if ( d->resolver->contains( "index" ) ) {
        d->resolver->value( "index" )->remove( context );
    }

    if ( model->remove( context ) != Soprano::ERROR_NONE ) {
        setError( "org.semanticdesktop.nepomuk.error.UnknownError",
                  "Failed to remove context " + context.uri().toString() );
    }
}


QList<Soprano::Statement>
Nepomuk::CoreServices::SopranoRDFRepository::listStatements( const QString& graphId, const Soprano::Statement& statement )
{
//    kDebug(300002) << k_funcinfo << graphId << ", " << statement << endl << flush;

    QList<Soprano::Statement> stmList;

    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
    }
    else {
        Soprano::Model *model = d->resolver->value( graphId );
        Soprano::StatementIterator iter = model->listStatements( statement );

        while ( iter.hasNext() ) {
            stmList.append( iter.next() );
        }
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

    Soprano::ResultSet r = executeQuery( repositoryId, query, "sparql" );
    if ( Soprano::Model* model = r.model() ) {
        Soprano::StatementIterator iter = model->listStatements();

        while ( iter.hasNext() ) {
            stmList.append( iter.next() );
        }
    }

    return stmList;
}


QList<Soprano::Statement> Nepomuk::CoreServices::SopranoRDFRepository::construct( const QString& repositoryId, const QString& query,
                                                                                  const QString& querylanguage )
{
    QList<Soprano::Statement> stmList;

    Soprano::ResultSet r = executeQuery( repositoryId, query, querylanguage );
    if ( Soprano::Model* model = r.model() ) {
        Soprano::StatementIterator iter = model->listStatements();

        while ( iter.hasNext() ) {
            stmList.append( iter.next() );
        }
    }

    return stmList;
}


Nepomuk::RDF::QueryResultTable Nepomuk::CoreServices::SopranoRDFRepository::select( const QString& repositoryId, const QString& query,
										    const QString& querylanguage )
{
    Soprano::ResultSet r = executeQuery( repositoryId, query, querylanguage );
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
//    kDebug(300002) << k_funcinfo << repositoryId << ", " << statement << endl << flush;

    if ( !d->resolver->contains( repositoryId ) ) {
        kDebug(300002) << k_funcinfo << ": " << " repository " << repositoryId << " not found!" << endl;
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + repositoryId );
        return 0;
    }
    else {
        Soprano::Model *model = d->resolver->value( repositoryId );
        Soprano::StatementIterator iter = model->listStatements( statement );

        return d->cache.insert( iter, timeoutMSec );
    }
}


int Nepomuk::CoreServices::SopranoRDFRepository::queryConstruct( const QString& repositoryId, const QString& query,
								 const QString& querylanguage, int timeoutMSec )
{
    if ( !d->resolver->contains( repositoryId ) ) {
        kDebug(300002) << k_funcinfo << ": " << " repository " << repositoryId << " not found!" << endl;
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
        kDebug(300002) << k_funcinfo << ": " << " repository " << repositoryId << " not found!" << endl;
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
        kDebug(300002) << k_funcinfo << ": " << " repository " << repositoryId << " not found!" << endl;
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
        kDebug(300002) << k_funcinfo << ": " << " repository " << repositoryId << " not found!" << endl;
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + repositoryId );
        return 0;
    }
    else {
        Soprano::ResultSet r = executeQuery( repositoryId, query, "sparql" );
        if( r.isBool() )
            return ( r.boolValue() ? 1 : 0 );
        else
            return 0;
    }
}


QList<Soprano::Statement> Nepomuk::CoreServices::SopranoRDFRepository::fetchListStatementsResults( int queryId, int max )
{
    QList<Soprano::Statement> sl;
    if( d->cache.contains( queryId ) ) {
        Soprano::StatementIterator it = d->cache.getStatements( queryId );
        int cnt = 0;
        while( it.hasNext() && ( cnt < max || max == 0 ) ) {
            sl.append( it.next() );
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
        Soprano::ResultSet rs = d->cache.getResultSet( queryId );

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
        kDebug( 300002 ) << k_funcinfo << " invalid query id: " << queryId << endl;
    }

    return rt;
}


void Nepomuk::CoreServices::SopranoRDFRepository::closeQuery( int listId )
{
    //  kDebug(300002) << k_funcinfo << ": " << k_funcinfo << listId << endl;

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


Soprano::ResultSet Nepomuk::CoreServices::SopranoRDFRepository::executeQuery( const QString& graphId,
									      const QString& query,
									      const QString& queryType )
{
    Soprano::ResultSet result;

    if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
    }
    else {
        Soprano::Model *model = d->resolver->value( graphId );

        Soprano::Query::QueryType sopranoQueryType( Soprano::Query::SPARQL );

        if( queryType.toLower() == QString("rdql") )
            sopranoQueryType = Soprano::Query::RDQL;
        else if( queryType.toLower() != QString("sparql") ) {
            kDebug(300002) << k_funcinfo << ": " << "Unsupported query language: " << queryType << endl;
        }
        else
            result = model->executeQuery( Soprano::Query( query, sopranoQueryType ) );
    }

    return result;
}


void Nepomuk::CoreServices::SopranoRDFRepository::dumpGraph( const QString& graphId )
{
    if ( graphId == "system" ) {
        d->system->print();
    }
    else if ( !d->resolver->contains( graphId ) ) {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
    }
    else {
        kDebug(300002) << k_funcinfo << ": " << "(SopranoRDFRepository) Listing statements in " << graphId << endl;
        Soprano::Model *model = d->resolver->value( graphId );
        model->print();
    }
}


QList<Soprano::Statement> Nepomuk::CoreServices::SopranoRDFRepository::buildIndexGraph( const Soprano::Statement& statement ) const
{
    QList<Soprano::Statement> indexGraph;

    if ( statement.object().isLiteral() ) {
        QString literal = statement.object().toString();
        QStringList words = literal.split( QRegExp( "\\W+" ), QString::SkipEmptyParts );
        for ( QStringList::const_iterator it = words.constBegin(); it != words.constEnd(); ++it ) {
            QString keyword = ( *it ).toLower();
            indexGraph.append( Soprano::Statement( QUrl( "http://nepomuk-kde.semanticdesktop.org/words#" + keyword ),
                                                   QUrl( "http://nepomuk-kde.semanticdesktop.org/words/appearsIn" ),
                                                   statement.subject(),
                                                   statement.context() ) );
        }
    }

    return indexGraph;
}


QString Nepomuk::CoreServices::SopranoRDFRepository::createStoragePath( const QString& repositoryId ) const
{
    // FIXME: use KStandardDirs properly to get the data path
    return QDir::homePath() + "/.nepomuk/share/storage/" + repositoryId;
}

#include "sopranordfrepository.moc"
