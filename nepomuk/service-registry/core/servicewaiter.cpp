/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "servicewaiter.h"
#include "core.h"

#include <QTimer>
#include <QEventLoop>

#include <kdebug.h>


class Nepomuk::Middleware::Registry::ServiceWaiter::Private
{
public:
    QStringList serviceUris;
    QEventLoop loop;
    QTimer timer;
    Core* core;
};


Nepomuk::Middleware::Registry::ServiceWaiter::ServiceWaiter( Core* parent )
    : QObject( parent )
{
    d = new Private;
    d->core = parent;
    connect( &d->timer, SIGNAL( timeout() ), this, SLOT( slotTimeout() ) );
    connect( parent, SIGNAL( serviceRegistered( const QString& ) ), this, SLOT( slotServiceRegistered( const QString& ) ) );
}


Nepomuk::Middleware::Registry::ServiceWaiter::~ServiceWaiter()
{
    delete d;
}


bool Nepomuk::Middleware::Registry::ServiceWaiter::waitForService( const QString& service, int msecs )
{
    return waitForServices( QStringList( service ), msecs );
}


bool Nepomuk::Middleware::Registry::ServiceWaiter::waitForServices( const QStringList& services, int msecs )
{
    if ( d->loop.isRunning() ) {
        kDebug(300003) << "(Nepomuk::Middleware::Registry::ServiceWaiter) called wait() on a running instance.";
        return false;
    }

    d->serviceUris = services;

    // check for already running services
    for ( QStringList::iterator it = d->serviceUris.begin(); it != d->serviceUris.end(); ) {
        if ( d->core->findServiceByUrl( *it ) ) {
            d->serviceUris.erase( it );
        }
        else {
            ++it;
        }
    }

    if ( services.isEmpty() ) {
        return true;
    }


    if ( msecs > 0 ) {
        d->timer.start( msecs );
    }

    return( d->loop.exec() == 0 );
}


void Nepomuk::Middleware::Registry::ServiceWaiter::slotServiceRegistered( const QString& uri )
{
    if ( d->loop.isRunning() ) {
        d->serviceUris.removeAll( uri );
        if ( d->serviceUris.isEmpty() ) {
            kDebug(300003) << "(Nepomuk::Middleware::Registry::ServiceWaiter) all services found";
            d->timer.stop();
            d->loop.exit( 0 );
        }
    }
}


void Nepomuk::Middleware::Registry::ServiceWaiter::slotTimeout()
{
    d->loop.exit( 1 );
}

#include "servicewaiter.moc"
