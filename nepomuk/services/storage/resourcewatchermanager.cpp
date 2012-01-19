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
QDBusVariant nodeToVariant(const Soprano::Node& node) {
    if(node.isResource()) {
        return QDBusVariant(node.uri());
    }
    else {
        return QDBusVariant(node.literal().variant());
    }
}

QVariantList nodeListToVariantList(const QList<Soprano::Node> &nodes) {
    QVariantList list;
    list.reserve(nodes.size());
    foreach( const Soprano::Node &n, nodes ) {
        list << nodeToVariant(n).variant();
    }

    return list;
}

QString convertUri(const QUrl& uri) {
    return KUrl(uri).url();
}

QStringList convertUris(const QList<QUrl>& uris) {
    QStringList sl;
    foreach(const QUrl& uri, uris)
        sl << convertUri(uri);
    return sl;
}

QUrl convertUri(const QString& s) {
    return KUrl(s);
}

QList<QUrl> convertUris(const QStringList& uris) {
    QList<QUrl> sl;
    foreach(const QString& uri, uris)
        sl << convertUri(uri);
    return sl;
}
}

Nepomuk::ResourceWatcherManager::ResourceWatcherManager(QObject* parent)
    : QObject(parent),
      m_connectionCount(0)
{
    QDBusConnection::sessionBus().registerObject("/resourcewatcher", this, QDBusConnection::ExportScriptableSlots);
}

Nepomuk::ResourceWatcherManager::~ResourceWatcherManager()
{
    // the connections call removeConnection() from their descrutors. Thus,
    // we need to clean them up before we are deleted ourselves
    QSet<ResourceWatcherConnection*> allConnections
            = QSet<ResourceWatcherConnection*>::fromList(m_resHash.values())
            + QSet<ResourceWatcherConnection*>::fromList(m_propHash.values())
            + QSet<ResourceWatcherConnection*>::fromList(m_typeHash.values());
    qDeleteAll(allConnections);
}


void Nepomuk::ResourceWatcherManager::addStatement(const Soprano::Statement& st)
{
    addProperty( st.subject(), st.predicate().uri(), st.object() );
}

void Nepomuk::ResourceWatcherManager::addProperty(const Soprano::Node& res, const QUrl& property, const Soprano::Node& value)
{
    typedef ResourceWatcherConnection RWC;

    // FIXME: take care of duplicate signals!

    //
    // Emit signals for all the connections that are only watching specific resources
    //
    QSet<RWC*> resConnections;
    QList<RWC*> connections = m_resHash.values( res.uri() );
    foreach( RWC* con, connections ) {
        if( !m_propHash.values().contains(con) ) {
            emit con->propertyAdded( convertUri(res.uri()),
                                     convertUri(property),
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
            emit con->propertyAdded( convertUri(res.uri()),
                                     convertUri(property),
                                     nodeToVariant(value) );
        }
    }

    //
    // Emit type + property signals
    //
    //TODO: Implement me! ( How? )
}

void Nepomuk::ResourceWatcherManager::removeProperty(const Soprano::Node& res, const QUrl& property, const QList<Soprano::Node>& values)
{
    typedef ResourceWatcherConnection RWC;

    //
    // Emit signals for all the connections that are only watching specific resources
    //
    QSet<RWC*> resConnections;
    QList<RWC*> connections = m_resHash.values( res.uri() );
    foreach( RWC* con, connections ) {
        if( !m_propHash.values().contains(con) ) {
            emit con->propertyRemoved( convertUri(res.uri()),
                                       convertUri(property),
                                       nodeListToVariantList(values) );
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
            emit con->propertyRemoved( convertUri(res.uri()),
                                       convertUri(property),
                                       nodeListToVariantList(values) );
        }
    }
}

void Nepomuk::ResourceWatcherManager::setProperty(const QMultiHash< QUrl, Soprano::Node >& oldValues,
                                                  const QUrl& property,
                                                  const QList<Soprano::Node>& nodes)
{
    typedef ResourceWatcherConnection RWC;

    QList<QUrl> uniqueKeys = oldValues.keys();
    foreach( const QUrl resUri, uniqueKeys ) {
        QList<Soprano::Node> old = oldValues.values( resUri );

        //
        // Emit signals for all the connections that are only watching specific resources
        //
        QSet<RWC*> resConnections;
        QList<RWC*> connections = m_resHash.values( resUri );
        foreach( RWC* con, connections ) {
            if( !m_propHash.values().contains(con) ) {
                emit con->propertyChanged( convertUri(resUri),
                                           convertUri(property),
                                           nodeListToVariantList(old),
                                           nodeListToVariantList(nodes) );
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
                emit con->propertyChanged( convertUri(resUri),
                                           convertUri(property),
                                           nodeListToVariantList(old),
                                           nodeListToVariantList(nodes) );
            }
        }

        //
        // Emit type + property signals
        //
        //TODO: Implement me! ( How? )
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
        emit con->resourceCreated(convertUri(uri), convertUris(types));
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
        emit con->resourceRemoved(convertUri(res), convertUris(types));
    }
}

Nepomuk::ResourceWatcherConnection* Nepomuk::ResourceWatcherManager::createConnection(const QList<QUrl> &resources,
                                                                                      const QList<QUrl> &properties,
                                                                                      const QList<QUrl> &types)
{
    kDebug() << resources << properties << types;

    if( resources.isEmpty() && properties.isEmpty() && types.isEmpty() ) {
        return 0;
    }

    ResourceWatcherConnection* con = new ResourceWatcherConnection( this );
    foreach( const QUrl& res, resources ) {
        m_resHash.insert(res, con);
    }

    foreach( const QUrl& prop, properties ) {
        m_propHash.insert(prop, con);
    }

    foreach( const QUrl& type, types ) {
        m_typeHash.insert(type, con);
    }

    return con;
}

QDBusObjectPath Nepomuk::ResourceWatcherManager::watch(const QStringList& resources,
                                                       const QStringList& properties,
                                                       const QStringList& types)
{
    kDebug() << resources << properties << types;

    if(ResourceWatcherConnection* con = createConnection(convertUris(resources), convertUris(properties), convertUris(types))) {
        return con->registerDBusObject(message().service(), ++m_connectionCount);
    }
    else {
        return QDBusObjectPath();
    }
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

void Nepomuk::ResourceWatcherManager::setResources(Nepomuk::ResourceWatcherConnection *conn, const QStringList &resources)
{
    const QSet<QUrl> newRes = convertUris(resources).toSet();
    const QSet<QUrl> oldRes = m_resHash.keys(conn).toSet();

    foreach(const QUrl& res, newRes - oldRes) {
        m_resHash.insert(res, conn);
    }
    foreach(const QUrl& res, oldRes - newRes) {
        m_resHash.remove(res, conn);
    }
}

void Nepomuk::ResourceWatcherManager::addResource(Nepomuk::ResourceWatcherConnection *conn, const QString &resource)
{
    m_resHash.insert(convertUri(resource), conn);
}

void Nepomuk::ResourceWatcherManager::removeResource(Nepomuk::ResourceWatcherConnection *conn, const QString &resource)
{
    m_resHash.remove(convertUri(resource), conn);
}

void Nepomuk::ResourceWatcherManager::setProperties(Nepomuk::ResourceWatcherConnection *conn, const QStringList &properties)
{
    const QSet<QUrl> newprop = convertUris(properties).toSet();
    const QSet<QUrl> oldprop = m_propHash.keys(conn).toSet();

    foreach(const QUrl& prop, newprop - oldprop) {
        m_propHash.insert(prop, conn);
    }
    foreach(const QUrl& prop, oldprop - newprop) {
        m_propHash.remove(prop, conn);
    }
}

void Nepomuk::ResourceWatcherManager::addProperty(Nepomuk::ResourceWatcherConnection *conn, const QString &property)
{
    m_propHash.insert(convertUri(property), conn);
}

void Nepomuk::ResourceWatcherManager::removeProperty(Nepomuk::ResourceWatcherConnection *conn, const QString &property)
{
    m_propHash.remove(convertUri(property), conn);
}

void Nepomuk::ResourceWatcherManager::setTypes(Nepomuk::ResourceWatcherConnection *conn, const QStringList &types)
{
    const QSet<QUrl> newtype = convertUris(types).toSet();
    const QSet<QUrl> oldtype = m_typeHash.keys(conn).toSet();

    foreach(const QUrl& type, newtype - oldtype) {
        m_typeHash.insert(type, conn);
    }
    foreach(const QUrl& type, oldtype - newtype) {
        m_typeHash.remove(type, conn);
    }
}

void Nepomuk::ResourceWatcherManager::addType(Nepomuk::ResourceWatcherConnection *conn, const QString &type)
{
    m_typeHash.insert(convertUri(type), conn);
}

void Nepomuk::ResourceWatcherManager::removeType(Nepomuk::ResourceWatcherConnection *conn, const QString &type)
{
    m_typeHash.remove(convertUri(type), conn);
}

#include "resourcewatchermanager.moc"
