/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "coreservices.h"

#include "sopranordfrepository.h"
#include "querydefinition.h"
#include "dbusinterface.h"
#include "repository.h"

#include <nepomuk/registry.h>
#include <soprano/soprano.h>

#include <kstandarddirs.h>
#include <kdebug.h>

#include <QtCore/QDir>


class Nepomuk::CoreServices::DaemonImpl::Private
{
public:
    Private()
        : registry(0),
          system(0),
          repository(0) {
    }

    // nepomuk registry
    Nepomuk::Middleware::Registry* registry;

    // RDF storage
    Soprano::Model* system;
    RepositoryMap resolver;

    // the services
    SopranoRDFRepository* repository;
};


Nepomuk::CoreServices::DaemonImpl::DaemonImpl( QObject* parent )
    : Soprano::Server::ServerCore( parent )
{
    d = new Private;
//    d->registry = new Nepomuk::Middleware::Registry( this );
}


bool Nepomuk::CoreServices::DaemonImpl::registerServices()
{
    QString storagePath = QDir::homePath() + "/.nepomuk/share/storage/";
    if( !KStandardDirs::makeDir( storagePath + "system" ) ) {
        kDebug(300002) << "Failed to create the system storage folder: " << storagePath + "system";
        return false;
    }

    // FIXME: make the Seasem2 backend work for better performance
//    kDebug(300002) << "Trying the Sesame2 backend...";
    const Soprano::Backend* backend = 0;//Soprano::discoverBackendByName( "sesame2" );
    if ( !backend ) {
//        kDebug(300002) << "...Sesame2 backend not found. Trying redland backend...";
        backend = Soprano::discoverBackendByName( "redland" );
    }
    if( !backend ) {
        kDebug(300002) << "Failed to load the Soprano Redland backend";
        return false;
    }
    Soprano::setUsedBackend( backend );

    QList<Soprano::BackendSetting> settings;
    settings.append( Soprano::BackendSetting( Soprano::BACKEND_OPTION_STORAGE_DIR, storagePath + "system" ) );
    d->system = Soprano::createModel( settings );
    if( !d->system ) {
        kDebug(300002) << "Failed to create the system store";
        return false;
    }

    // /////////////////////////////////////////////////
    // Load graphs
    // /////////////////////////////////////////////////

    Soprano::QueryResultIterator res = d->system->executeQuery( QueryDefinition::FIND_GRAPHS,
                                                                Soprano::Query::QUERY_LANGUAGE_SPARQL );

    while( res.next() ) {
        QString modelId = res.binding( "modelId" ).literal().toString();
        if ( !d->resolver.contains( modelId ) ) {
            Repository* rep = Repository::open( storagePath + modelId, modelId );
            if ( rep ) {
                d->resolver.insert( modelId, rep );
                kDebug(300002) << "(Nepomuk::CoreServices) found repository: " << modelId;
            }
        }
    }

    // FIXME: add error handling
    d->repository = new SopranoRDFRepository( d->system, &d->resolver );

//     if( d->registry->registerService( d->repository ) ) {
//         kDebug(300002) << "Failed to register Nepomuk services.";
//         return false;
//     }


    (void)new DBusInterface( this, d->repository );

    QDBusConnection::sessionBus().registerService( "org.semanticdesktop.nepomuk.CoreServices" );
    QDBusConnection::sessionBus().registerObject( "/org/semanticdesktop/nepomuk/CoreServices", this );

    // register the soprano interface
    registerAsDBusObject();

    return true;
}


Nepomuk::CoreServices::DaemonImpl::~DaemonImpl()
{
    qDebug() << "Shutting down Nepomuk core services.";
    delete d->system;
    for ( RepositoryMap::iterator it = d->resolver.begin();
          it != d->resolver.end(); ++it ) {
        delete it.value();
    }
    delete d->repository;
    delete d;
}


Soprano::Model* Nepomuk::CoreServices::DaemonImpl::model( const QString& name )
{
    if ( !d->resolver.contains( name ) ) {
        kDebug(300002) << "Creating new repository with name " << name;
        d->repository->createRepository( name );
    }
    return d->resolver[name]->model();
}

#include "coreservices.moc"
