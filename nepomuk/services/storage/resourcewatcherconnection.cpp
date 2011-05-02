/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2010-11 Vishesh handa <handa.vish@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "resourcewatcherconnection.h"
#include "resourcewatchermodel.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusObjectPath>

int Nepomuk::ResourceWatcherConnection::Id = 0;

Nepomuk::ResourceWatcherConnection::ResourceWatcherConnection( QObject* parent )
    : QObject( parent )
{
    // create the query adaptor on this connection
    //( void )new QueryAdaptor( this );
    
    // build the dbus object path from the id and register the connection as a Query dbus object
    m_objectPath = QString( "/watcher/%1" ).arg( Id++ );
    QDBusConnection::sessionBus().registerObject( m_objectPath, this );
    
    // watch the dbus client for unregistration for auto-cleanup
//     m_serviceWatcher = new QDBusServiceWatcher( dbusClient,
//                                                 QDBusConnection::sessionBus(),
//                                                 QDBusServiceWatcher::WatchForUnregistration,
//                                                 this );
//     connect( m_serviceWatcher, SIGNAL(serviceUnregistered(QString)),
//              this, SLOT(close()) );
    
    // finally return the dbus object path this connection can be found on
    //return QDBusObjectPath( dbusObjectPath );
}


QDBusObjectPath Nepomuk::ResourceWatcherConnection::dBusPath() const
{
    return QDBusObjectPath( m_objectPath );
}