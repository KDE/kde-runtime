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
    if ( d->resolver->contains( graphId ) )
    {
        kDebug(300002) << k_funcinfo << ": " << graphId << " already present!" << endl;
        return;// ERROR_FAILURE;
    }

    if( graphId.contains( '/' ) ) {
        kDebug(300002) << k_funcinfo << ": " << "Invalid repository name: " << graphId << endl;
        setError( "org.semanticdesktop.nepomuk.error.InvalidParameter",
                  i18n("Invalid repository name: %1").arg(graphId) );
        return;
    }

    // FIXME: use KStandardDirs properly to get the data path
    QString storagePath = QDir::homePath() + "/.nepomuk/share/storage/" + graphId;

    KStandardDirs::makeDir( storagePath );

    // FIXME: add configuration for stuff like this
    Soprano::Model* m = Soprano::createModel( graphId, QString( "new=no,dir="+ storagePath ).split(",") );
    if( m ) {
        // create a URI for the repository (do we need this? couldn't we just use a blank node?)
        QUrl uri = "http://nepomuk.semanticdesktop.org/repositories/localhost/" + graphId;

        // FIXME: use NGM
        Soprano::Node subject0( uri );
        Soprano::Node predicate0( QUrl("http://nepomuk-kde.semanticdesktop.org/rdfrepository#created") );

        Soprano::LiteralValue now( QDateTime::currentDateTime() );
        Soprano::Node object0( now );

        Soprano::Node predicate1( QUrl("http://www.w3.org/1999/02/22-rdf-syntax-ns#type") );
        Soprano::Node object1( QUrl( "http://www.soprano.org/types#Model" ) );

        Soprano::Node predicate2( QUrl("http://www.w3.org/2000/01/rdf-schema#label") );
        Soprano::Node object2 = Soprano::LiteralValue( graphId );

        // Add a return code to the model API
        d->system->add( Soprano::Statement( subject0, predicate0, object0 ) );
        d->system->add( Soprano::Statement( subject0, predicate1, object1 ) );
        d->system->add( Soprano::Statement( subject0, predicate2, object2 ) );

        // FIXME: use NIO or whatever like File::hasLocation
        d->system->add( Soprano::Statement( subject0,
                                            Soprano::Node( QUrl("http://nepomuk-kde.semanticdesktop.org/rdfrepository#hasStoragePath") ),
                                            Soprano::Node( Soprano::LiteralValue( storagePath ) ) ) );

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
    // Return a list of all graphId from system model
    Soprano::Query query( QueryDefinition::FIND_GRAPHS, Soprano::Query::RDQL );

    Soprano::ResultSet res = d->system->executeQuery( query );

    QStringList graphs;
    while ( res.next() )
    {
        Soprano::Node id = res.binding( "modelId" );
        graphs << id.literal().toString();
    }

    return graphs;
}


void Nepomuk::CoreServices::SopranoRDFRepository::removeRepository( const QString& repositoryId )
{
#ifdef __GNUC__
#warning removeRepository_not_implemented_yet
#endif
    // FIXME: implement removeRepository
    setError( Nepomuk::Backbone::Error::MethodNotImplemented,
              "removeRepository is not yet implemented in the Soprano RDFRepository." );
}


int Nepomuk::CoreServices::SopranoRDFRepository::getRepositorySize( const QString& graphId )
{
    if ( !d->resolver->contains( graphId ) )
    {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        return 0;
    }

    Soprano::Model *model = d->resolver->value( graphId );
    return model->size();
}


int Nepomuk::CoreServices::SopranoRDFRepository::contains( const QString& graphId, const Soprano::Statement& statement )
{
    if ( !d->resolver->contains( graphId ) )
    {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
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
        Soprano::Model *model = d->resolver->value( graphId );

        if ( model->add( statement ) ) {
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
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        setError( "org.semanticdesktop.nepomuk.services.rdfrepository.error.UnknownRepositoryId",
                  "No such repository: " + graphId );
    }
    else {
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
        }
    }
}


int Nepomuk::CoreServices::SopranoRDFRepository::removeStatement( const QString& graphId, const Soprano::Statement& statement )
{
    if ( !d->resolver->contains( graphId ) )
    {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        return 0;
    }

    Soprano::Model *model = d->resolver->value( graphId );

    if( model->remove( statement ) == Soprano::ERROR_NONE )
        return 1;
    else
        return 0;
}


int Nepomuk::CoreServices::SopranoRDFRepository::removeStatements( const QString& graphId, const QList<Soprano::Statement>& statements )
{
    if ( !d->resolver->contains( graphId ) )
    {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        return 0;
    }

    Soprano::Model *model = d->resolver->value( graphId );

    int cnt = 0;

    QList<Soprano::Statement> ssl;
    for( QList<Soprano::Statement>::const_iterator it = statements.constBegin();
         it != statements.constEnd(); ++it ) {
        if( model->remove( *it ) == Soprano::ERROR_NONE )
            ++cnt;
    }

    return cnt;
}


int Nepomuk::CoreServices::SopranoRDFRepository::removeAllStatements( const QString& graphId, const Soprano::Statement& statement )
{
    if ( !d->resolver->contains( graphId ) )
    {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        return 0;
    }

    Soprano::Model *model = d->resolver->value( graphId );

    // FIXME: is there a way to get the number of removed statements? Maybe we should make the Soprano API comply more with our repository
    if ( model->removeAll( statement ) == Soprano::ERROR_NONE )
        return 1;
    else
        return 0;
}


void Nepomuk::CoreServices::SopranoRDFRepository::removeContext( const QString& graphId, const Soprano::Node& context )
{
    if ( !d->resolver->contains( graphId ) )
    {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
        return;// ERROR_FAILURE;
    }

    Soprano::Model *model = d->resolver->value( graphId );

    model->remove( context );
}


QList<Soprano::Statement>
Nepomuk::CoreServices::SopranoRDFRepository::listStatements( const QString& graphId, const Soprano::Statement& statement )
{
    kDebug(300002) << k_funcinfo << graphId << ", " << statement << endl << flush;

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
    kDebug(300002) << k_funcinfo << repositoryId << ", " << statement << endl << flush;

    if ( !d->resolver->contains( repositoryId ) ) {
        kDebug(300002) << k_funcinfo << ": " << " repository " << repositoryId << " not found!" << endl;
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
        while( cnt < max || max == 0 ) {
            if( !rs.next() )
                break;

            QList<Soprano::Node> row;
            for( int i = 0; i < rs.bindingCount(); ++i )
                row.append( rs.binding( i ) );
            rt.rows.append( row );

            ++cnt;
        }

        if( cnt < max )
            closeQuery( queryId );
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
    return( ( lang.toLower() == "rqdl" || lang.toLower() == "sparql" ) ? 1 : 0 );
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

    if ( !d->resolver->contains( graphId ) )
    {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
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

//     if( sopranoResultSet.isBinding() ) {
//       while( sopranoResultSet.next() ) {
// 	const QStringList& bindings = sopranoResultSet.bindingNames();
// 	Soprano::NodeMap nm;
// 	for( QStringList::const_iterator it = bindings.begin(); it != bindings.end(); ++it )
// 	  nm.insert( *it, Util::createNode( sopranoResultSet.binding( *it ) ) );
// 	result.append( nm );
//       }
//     }
//     else {
//       kDebug(300002) << k_funcinfo << ": " << "Only binding results supported at the moment." << endl;
//     }
    }

    return result;
}


QStringList Nepomuk::CoreServices::SopranoRDFRepository::dumpGraph( const QString& graphId )
{
    QStringList s;

    if ( !d->resolver->contains( graphId ) )
    {
        kDebug(300002) << k_funcinfo << ": " << " repository " << graphId << " not found!" << endl;
    }
    else {
        kDebug(300002) << k_funcinfo << ": " << "(SopranoRDFRepository) Listing statements in " << graphId << endl;
        Soprano::Model *model = d->resolver->value( graphId );
        model->print();
    }

    return s;
}

#include "sopranordfrepository.moc"
