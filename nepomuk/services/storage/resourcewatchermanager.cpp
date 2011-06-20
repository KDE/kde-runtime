/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2011  Vishesh Handa <handa.vish@gmail.com>
    Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

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


#include "resourcewatchermanager.h"
#include "resourcewatcherconnection.h"

#include <Soprano/Statement>
#include <Soprano/Vocabulary/RDF>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>

#include <KUrl>
#include <KDebug>

#include <QtCore/QStringList>
#include <QtCore/QSet>


using namespace Soprano::Vocabulary;

namespace {
QVariant nodeToVariant(const Soprano::Node& node) {
    if(node.isResource()) {
        return node.uri();
    }
    else {
        return node.literal().variant();
    }
}

QStringList convertUris(const QList<QUrl>& uris) {
    QStringList sl;
    foreach(const QUrl& uri, uris)
        sl << KUrl(uri).url();
    return sl;
}
}

Nepomuk::ResourceWatcherManager::ResourceWatcherManager(QObject* parent)
    : QObject(parent),
      m_connectionCount(0)
{
    QDBusConnection::sessionBus().registerObject("/resourcewatcher", this, QDBusConnection::ExportScriptableSlots);
}


void Nepomuk::ResourceWatcherManager::addProperty(const Soprano::Node res, const QUrl& property, const Soprano::Node& value)
{
    typedef ResourceWatcherConnection RWC;

    // FIXME: take care of duplicate signals!

    //
    // Emit signals for all the connections that are only watching specific resources
    //
    QSet<RWC*> resConnections;
    QList<RWC*> connections = m_resHash.values( res.uri() );
    foreach( RWC* con, connections ) {
        if( !con->hasProperties() ) {
            emit con->propertyAdded( KUrl(res.uri()).url(),
                                     property.toString(),
                                     nodeToVariant(value) );
        }
        else {
            resConnections << con;
        }
    }

    //
    // Emit signals for the connections that are watching specific resources and properties
    //
    QList<RWC*> propConnections = m_propHash.values( property );
    foreach( RWC* con, propConnections ) {
        QSet<RWC*>::const_iterator it = resConnections.constFind( con );
        if( it != resConnections.constEnd() ) {
            emit con->propertyAdded( KUrl(res.uri()).url(),
                                     property.toString(),
                                     nodeToVariant(value) );
        }
    }

    //
    // Emit type + property signals
    //
    //TODO: Implement me! ( How? )
}

void Nepomuk::ResourceWatcherManager::removeProperty(const Soprano::Node res, const QUrl& property, const Soprano::Node& value)
{
    typedef ResourceWatcherConnection RWC;
    
    //
    // Emit signals for all the connections that are only watching specific resources
    //
    QSet<RWC*> resConnections;
    QList<RWC*> connections = m_resHash.values( res.uri() );
    foreach( RWC* con, connections ) {
        if( !con->hasProperties() ) {
            emit con->propertyRemoved( KUrl(res.uri()).url(),
                                       property.toString(),
                                       nodeToVariant(value) );
        }
        else {
            resConnections << con;
        }
    }
    
    //
    // Emit signals for the conn2ections that are watching specific resources and properties
    //
    QList<RWC*> propConnections = m_propHash.values( property );
    foreach( RWC* con, propConnections ) {
        QSet<RWC*>::const_iterator it = resConnections.constFind( con );
        if( it != resConnections.constEnd() ) {
            emit con->propertyRemoved( KUrl(res.uri()).url(),
                                       property.toString(),
                                       nodeToVariant(value) );
        }
    }
}

void Nepomuk::ResourceWatcherManager::createResource(const QUrl &uri, const QList<QUrl> &types)
{
    QSet<ResourceWatcherConnection*> connections;
    foreach(const QUrl& type, types) {
        foreach(ResourceWatcherConnection* con, m_typeHash.values( type )) {
            connections += con;
        }
    }

    foreach(ResourceWatcherConnection* con, connections) {
        con->resourceCreated(KUrl(uri).url(), convertUris(types));
    }
}

void Nepomuk::ResourceWatcherManager::removeResource(const QUrl &res, const QList<QUrl>& types)
{
    QSet<ResourceWatcherConnection*> connections;
    foreach(const QUrl& type, types) {
        foreach(ResourceWatcherConnection* con, m_typeHash.values( type )) {
            connections += con;
        }
    }
    foreach(ResourceWatcherConnection* con, m_resHash.values( res )) {
        connections += con;
    }

    foreach(ResourceWatcherConnection* con, connections) {
        con->resourceRemoved(KUrl(res).url(), convertUris(types));
    }
}

QDBusObjectPath Nepomuk::ResourceWatcherManager::watch(const QStringList& resources,
                                                       const QStringList& properties,
                                                       const QStringList& types)
{
    kDebug();

    if( resources.isEmpty() && properties.isEmpty() && types.isEmpty() )
        return QDBusObjectPath();

    ResourceWatcherConnection * con = new ResourceWatcherConnection( this, !properties.isEmpty() );
    foreach( const QString & res, resources ) {
        m_resHash.insert( QUrl(res), con );
    }

    foreach( const QString & prop, properties ) {
        m_propHash.insert( QUrl(prop), con );
    }

    foreach( const QString & type, properties ) {
        m_typeHash.insert( QUrl(type), con );
    }
    
    return con->registerDBusObject(message().service(), ++m_connectionCount);
}

namespace {
    void removeConnectionFromHash( QMultiHash<QUrl, Nepomuk::ResourceWatcherConnection*> & hash,
                 const Nepomuk::ResourceWatcherConnection * con )
    {
        QMutableHashIterator<QUrl, Nepomuk::ResourceWatcherConnection*> it( hash );
        while( it.hasNext() ) {
            if( it.next().value() == con )
                it.remove();
        }
    }
}

void Nepomuk::ResourceWatcherManager::removeConnection(Nepomuk::ResourceWatcherConnection *con)
{
    removeConnectionFromHash( m_resHash, con );
    removeConnectionFromHash( m_propHash, con );
    removeConnectionFromHash( m_typeHash, con );
}

#include "resourcewatchermanager.moc"
