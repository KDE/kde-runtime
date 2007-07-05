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
    QMap<QString, Soprano::Model*> resolver;

    // the services
    SopranoRDFRepository* repository;
};


Nepomuk::CoreServices::DaemonImpl::DaemonImpl( QObject* parent )
    : QObject( parent )
{
    d = new Private;
    d->registry = new Nepomuk::Middleware::Registry( this );
}


bool Nepomuk::CoreServices::DaemonImpl::registerServices()
{
    QString storagePath = QDir::homePath() + "/.nepomuk/share/storage/";
    if( !KStandardDirs::makeDir( storagePath + "system" ) ) {
        kDebug(300002) << "Failed to create the system storage folder: " << storagePath + "system" << endl;
        return false;
    }

    if( !Soprano::usedBackend() ) {
        kDebug(300002) << "Failed to load the Soprano Redland backend" << endl;
        return false;
    }

    d->system = Soprano::createModel( "system", QString("new=no,dir="+ storagePath + "system").split( "," ) );
    if( !d->system ) {
        kDebug(300002) << "Failed to create the system store" << endl;
        return false;
    }

    // /////////////////////////////////////////////////
    // Load graphs
    // /////////////////////////////////////////////////

    Soprano::Query query( QueryDefinition::FIND_GRAPHS, Soprano::Query::RDQL );
    Soprano::ResultSet res = d->system->executeQuery( query );

    while( res.next() ) {
        QString modelId = res.binding( "modelId" ).literal();
        if ( !d->resolver.contains( modelId ) ) {
            KStandardDirs::makeDir( storagePath + modelId );
            d->resolver.insert( modelId, Soprano::createModel( modelId,
                                                               QString("dir=" + storagePath + modelId ).split(",") ) );
            kDebug(300002) << "(Nepomuk::CoreServices) found repository: " << modelId << endl;
        }
    }

    // FIXME: add error handling
    d->repository = new SopranoRDFRepository( d->system, &d->resolver );

    if( d->registry->registerService( d->repository ) ) {
        kDebug(300002) << "Failed to register Nepomuk services." << endl;
        return false;
    }


    (void)new DBusInterface( this, d->repository );

    QDBusConnection::sessionBus().registerService( "org.semanticdesktop.nepomuk.CoreServices" );
    QDBusConnection::sessionBus().registerObject( "/org/semanticdesktop/nepomuk/CoreServices", this );

    return true;
}


Nepomuk::CoreServices::DaemonImpl::~DaemonImpl()
{
    kDebug(300002) << k_funcinfo << endl;

    delete d->system;
    for ( QMap<QString, Soprano::Model*>::iterator it = d->resolver.begin();
          it != d->resolver.end(); ++it ) {
        delete it.value();
    }
    delete d->repository;
    delete d;
}

#include "coreservices.moc"
