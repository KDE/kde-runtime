/*  This file is part of kdebase/workspace/solid
    Copyright (C) 2005,2007 Will Stephenson <wstephenson@kde.org>

    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin.ottens@kdab.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#include "networkstatus.h"

#include <QDBusServiceWatcher>
#include <QMap>

#include <KDebug>

#include "network.h"
#include "clientadaptor.h"
#include "serviceadaptor.h"

#include "systemstatusinterface.h"

#ifdef NM_BACKEND_ENABLED
#include "networkmanagerstatus.h"
#endif

#include "wicdstatus.h"

#include <kpluginfactory.h>
#include "ntracknetworkstate.h"

K_PLUGIN_FACTORY(NetworkStatusFactory,
                 registerPlugin<NetworkStatusModule>();
    )
K_EXPORT_PLUGIN(NetworkStatusFactory("networkstatus"))

// INTERNALLY USED STRUCTS AND TYPEDEFS

typedef QMap< QString, Network * > NetworkMap;

class NetworkStatusModule::Private
{
public:
    Private() : status( Solid::Networking::Unknown ), backend( 0 ), serviceWatcher( 0 )
    {

    }
    ~Private()
    {

    }
    NetworkMap networks;
    Solid::Networking::Status status;
    SystemStatusInterface *backend;
    QDBusServiceWatcher *serviceWatcher;
};

// CTORS/DTORS

NetworkStatusModule::NetworkStatusModule(QObject* parent, const QList<QVariant>&)
    : KDEDModule(parent), d( new Private )
{
    new ClientAdaptor( this );
    new ServiceAdaptor( this );

#ifdef HAVE_QNTRACK
    new NtrackNetworkState( this );
#endif

    init();
}

NetworkStatusModule::~NetworkStatusModule()
{
    Q_FOREACH ( Network * net, d->networks ) {
        delete net;
    }

    delete d;
}

// CLIENT INTERFACE

int NetworkStatusModule::status()
{
    kDebug( 1222 ) << " status: " << (int)d->status;
    return (int)d->status;
}

//protected:

void NetworkStatusModule::updateStatus()
{
    Solid::Networking::Status bestStatus = Solid::Networking::Unknown;
    const Solid::Networking::Status oldStatus = d->status;

    Q_FOREACH ( Network * net, d->networks ) {
        if ( net->status() > bestStatus )
            bestStatus = net->status();
    }
    d->status = bestStatus;

    if ( oldStatus != d->status ) {
        emit statusChanged( (uint)d->status );
    }
}

void NetworkStatusModule::serviceUnregistered( const QString & name )
{
    // unregister and delete any networks owned by a service that has just unregistered
    d->serviceWatcher->removeWatchedService( name );
    QMutableMapIterator<QString,Network*> it( d->networks );
    while ( it.hasNext() ) {
        it.next();
        if ( it.value()->service() == name ) {
            kDebug( 1222 ) << "Departing service " << name << " owned network " << it.value()->name() << ", removing it";
            Network * removedNet = it.value();
            it.remove();
            updateStatus();
            delete removedNet;
        }
    }
}

// SERVICE INTERFACE //

QStringList NetworkStatusModule::networks()
{
    if ( d->networks.count() ) {
      kDebug() << "Network status module is aware of " << d->networks.count() << " networks";
    } else {
      kDebug( 1222 ) << "Network status module is not aware of any networks";
    }
    return d->networks.keys();
}

void NetworkStatusModule::setNetworkStatus( const QString & networkName, int st )
{
    kDebug( 1222 ) << networkName << ", " << st;
    Solid::Networking::Status changedStatus = (Solid::Networking::Status)st;
    if ( d->networks.contains( networkName ) ) {
        Network * net = d->networks[ networkName ];
        net->setStatus( changedStatus );
        updateStatus();
    } else {
      kDebug( 1222 ) << "  No network named '" << networkName << "' known.";
    }
}

void NetworkStatusModule::registerNetwork( const QString & networkName, int status, const QString & serviceName )
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    QDBusConnectionInterface * sessionBus = dbus.interface();
    QString uniqueOwner = sessionBus->serviceOwner( serviceName ).value();

    kDebug( 1222 ) << networkName << ", with status " << status << " is owned by " << uniqueOwner;

    d->networks.insert( networkName, new Network( networkName, status, uniqueOwner ) );

    if ( d->serviceWatcher ) {
        d->serviceWatcher->addWatchedService( uniqueOwner );
    }

    updateStatus();
}

void NetworkStatusModule::unregisterNetwork( const QString & networkName )
{
    if ( networkName != QLatin1String("SolidNetwork") ) {
        kDebug( 1222 ) << networkName << " unregistered.";

        if ( d->serviceWatcher ) {
            Network * net = d->networks.value( networkName );
            if ( net ) {
                d->serviceWatcher->removeWatchedService( net->service() );
            }
        }

        d->networks.remove( networkName );
        updateStatus();
    }
}

void NetworkStatusModule::init()
{
    QList<SystemStatusInterface*> backends;
#ifdef NM_BACKEND_ENABLED
    backends << new NetworkManagerStatus( this );
#endif
    backends << new WicdStatus( this );

    while ( !backends.isEmpty() ) {
        d->backend = backends.takeFirst();
        if ( d->backend->isSupported() ) {
            qDeleteAll(backends);
            backends.clear();
        } else {
            delete d->backend;
            d->backend = 0;
        }
    }

    if ( d->backend != 0 ) {
        connect( d->backend, SIGNAL(statusChanged(Solid::Networking::Status)),
                 this, SLOT(solidNetworkingStatusChanged(Solid::Networking::Status)));
        Solid::Networking::Status status = d->backend->status();
        registerNetwork( QLatin1String("SolidNetwork"), status, QLatin1String("org.kde.kded") );
    }

    d->serviceWatcher = new QDBusServiceWatcher(this);
    d->serviceWatcher->setConnection(QDBusConnection::sessionBus());
    d->serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(d->serviceWatcher, SIGNAL(serviceUnregistered(QString)), SLOT(serviceUnregistered(QString)));
}

void NetworkStatusModule::solidNetworkingStatusChanged( Solid::Networking::Status status )
{
    kDebug( 1222 ) << "SolidNetwork changed status: " << status;
    setNetworkStatus( QLatin1String("SolidNetwork"), status );
}


#include "networkstatus.moc"
