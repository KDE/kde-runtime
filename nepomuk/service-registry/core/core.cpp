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

#include "core.h"
#include "service.h"
#include "backend.h"
#include "servicewaiter.h"

#include <nepomuk/servicedesc.h>

#include <QMap>
#include <QProcess>

#include <kdebug.h>
#include <kstandarddirs.h>
#include <kglobal.h>
#include <KDesktopFile>



namespace Nepomuk {
    namespace Middleware {
        namespace Registry {
            typedef QMap<QString, Nepomuk::Middleware::Registry::Service*> ServiceMap;
        }
    }
}


class Nepomuk::Middleware::Registry::Core::Private
{
public:
    ServiceMap services;
};


Nepomuk::Middleware::Registry::Core::Core( QObject* parent )
    : QObject( parent )
{
    d = new Private;
    autoStartServices();
}


Nepomuk::Middleware::Registry::Core::~Core()
{
    delete d;
}


const QList<const Nepomuk::Middleware::Registry::Service*>
Nepomuk::Middleware::Registry::Core::allServices()
{
    QList<const Nepomuk::Middleware::Registry::Service*> l;
    for( ServiceMap::const_iterator it = d->services.constBegin();
         it != d->services.constEnd(); ++it ) {
        l.append( it.value() );
    }
    return l;
}


const QList<const Nepomuk::Middleware::Registry::Service*>
Nepomuk::Middleware::Registry::Core::findServicesByName( const QString& name )
{
    QRegExp rex( name );
    QList<const Nepomuk::Middleware::Registry::Service*> l;
    for( ServiceMap::const_iterator it = d->services.constBegin();
         it != d->services.constEnd(); ++it ) {
        if( rex.exactMatch( it.value()->name() ) )
            l.append( *it );
    }
    return l;
}


Nepomuk::Middleware::Registry::Service*
Nepomuk::Middleware::Registry::Core::findServiceByUrl_internal( const QString& url )
{
    for( ServiceMap::const_iterator it = d->services.constBegin();
         it != d->services.constEnd(); ++it ) {
        if( it.value()->url() == url )
            return it.value();
    }

    return 0;
}


const Nepomuk::Middleware::Registry::Service*
Nepomuk::Middleware::Registry::Core::findServiceByUrl( const QString& url )
{
    return findServiceByUrl_internal( url );
}


const Nepomuk::Middleware::Registry::Service*
Nepomuk::Middleware::Registry::Core::findServiceByType_internal( const QString& type )
{
    for( ServiceMap::const_iterator it = d->services.constBegin();
         it != d->services.constEnd(); ++it ) {
        if( it.value()->type() == type )
            return it.value();
    }

    return 0;
}


const Nepomuk::Middleware::Registry::Service*
Nepomuk::Middleware::Registry::Core::findServiceByType( const QString& type )
{
    kDebug() << k_funcinfo;
    const Service* service = findServiceByType_internal( type );
    if ( !service ) {
        if ( loadServiceOnDemand( type ) ) {
            service = findServiceByType_internal( type );
        }
    }
    return service;
}


const QList<const Nepomuk::Middleware::Registry::Service*>
Nepomuk::Middleware::Registry::Core::findServicesByType( const QString& type )
{
    loadServiceOnDemand( type );

    QList<const Nepomuk::Middleware::Registry::Service*> l;
    for( ServiceMap::const_iterator it = d->services.constBegin();
         it != d->services.constEnd(); ++it ) {
        if( it.value()->type() == type )
            l.append( it.value() );
    }
    return l;
}


int Nepomuk::Middleware::Registry::Core::registerService( const Nepomuk::Middleware::ServiceDesc& desc,
							Nepomuk::Middleware::Registry::Backend* backend )
{
    //
    // 1. Check if the uri already exists
    // 2. Check if it is a valid Nepomuk::Middleware::KDE service
    //
    if( findServiceByUrl_internal( desc.url ) )
        return URI_ALREDAY_REGISTERED;

    Service* newService = Service::createFromDefinition( desc, backend );
    if( newService ) {
        kDebug(300003) << "(Nepomuk::Middleware::Registry::Core) Found valid service: " << newService->url();
        d->services[newService->url()] = newService;

        emit serviceRegistered( desc.url );
        emit serviceRegistered( newService );

        return SERVICE_REGISTERED;
    }
    else {
        return INVALID_SERVICE;
    }
}


bool Nepomuk::Middleware::Registry::Core::loadServiceOnDemand( const QString& typeUri )
{
    // FIXME: check if the service is already running

    kDebug(300003) << k_funcinfo;

    QStringList allServices = KGlobal::dirs()->findAllResources( "services", "nepomuk/*" );
    foreach( QString service, allServices ) {
        KDesktopFile serviceDesktopFile( service );
        if ( serviceDesktopFile.readType() == "Service" &&
             serviceDesktopFile.readEntry( "ServiceTypes" ) == "NepomukService" &&
             serviceDesktopFile.readEntry( "X-Nepomuk-ServiceType" ) == typeUri &&
             serviceDesktopFile.readEntry( "X-Nepomuk-LoadOnDemand", false ) ) {

            if ( serviceDesktopFile.readEntry( "URL" ).isEmpty() ) {
                kDebug( 300003 ) << "(Nepomuk::Middleware::Registry::Core) invalid Nepomuk service desktop file: missing URL.";
                continue;
            }

            kDebug(300003) << "(Nepomuk::Middleware::Registry::Core) starting service "
                           << serviceDesktopFile.readEntry( "Exec" ) << " on demand." << endl;

            // service fits. Load it and go on
            if ( QProcess::startDetached( serviceDesktopFile.readEntry( "Exec" ) ) ) {
                // wait up to 5 seconds for the service
                ServiceWaiter waiter( this );
                if ( waiter.waitForService( serviceDesktopFile.readEntry( "URL" ), 5000 ) ) {
                    return true;
                }
            }
            else {
                kDebug(300003) << "(Nepomuk::Middleware::Registry::Core) failed to start service: "
                               << serviceDesktopFile.readEntry( "Exec" ) << endl;
            }
        }
    }

    return false;
}


void Nepomuk::Middleware::Registry::Core::autoStartServices()
{
    QStringList serviceUris;
    QStringList allServices = KGlobal::dirs()->findAllResources( "services", "nepomuk/*" );
    foreach( QString service, allServices ) {
        KDesktopFile serviceDesktopFile( service );
        if ( serviceDesktopFile.readType() == "Service" &&
             serviceDesktopFile.readEntry( "ServiceTypes" ) == "NepomukService" &&
             serviceDesktopFile.readEntry( "X-Nepomuk-AutoLoad", false ) ) {

            if ( serviceDesktopFile.readEntry( "URL" ).isEmpty() ) {
                kDebug( 300003 ) << "(Nepomuk::Middleware::Registry::Core) invalid Nepomuk service desktop file: missing URL.";
                continue;
            }

            // service fits. Load it and go on
            if ( QProcess::startDetached( serviceDesktopFile.readEntry( "Exec" ) ) ) {
                serviceUris.append( serviceDesktopFile.readEntry( "URL" ) );
                kDebug(300003) << "(Nepomuk::Middleware::Registry::Core) started service "
                               << serviceDesktopFile.readEntry( "Exec" ) << endl;
            }
            else {
                kDebug(300003) << "(Nepomuk::Middleware::Registry::Core) failed to start service: "
                               << serviceDesktopFile.readEntry( "Exec" ) << endl;
            }
        }
    }

    ServiceWaiter waiter( this );
    waiter.waitForServices( serviceUris );
}


int Nepomuk::Middleware::Registry::Core::unregisterService( const Nepomuk::Middleware::ServiceDesc& desc,
							  Nepomuk::Middleware::Registry::Backend* backend )
{
    return unregisterService( desc.url, backend );
}


int Nepomuk::Middleware::Registry::Core::unregisterService( const QString& uri, Nepomuk::Middleware::Registry::Backend* backend )
{
    Q_UNUSED( backend );

    Service* s = findServiceByUrl_internal( uri );
    if( s ) {
        kDebug(300003) << "(Nepomuk::Middleware::Registry::Core) Removing service " << uri;

        d->services.remove( uri );
        emit serviceUnregistered( uri );
        emit serviceUnregistered( s );
        delete s;

        return SERVICE_REGISTERED;
    }
    else {
        kDebug(300003) << "(Nepomuk::Middleware::Registry::Core) No service with this URI found: " << uri;
        return INVALID_SERVICE;
    }
}

#include "core.moc"
