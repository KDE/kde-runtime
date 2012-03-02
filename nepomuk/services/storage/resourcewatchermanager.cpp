/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2011  Vishesh Handa <handa.vish@gmail.com>
    Copyright (C) 2011-2012 Sebastian Trueg <trueg@kde.org>

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
#include "datamanagementmodel.h"

#include <Soprano/Statement>
#include <Soprano/StatementIterator>
#include <Soprano/NodeIterator>
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

template<typename T> QVariantList nodeListToVariantList(const T &nodes) {
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

template<typename T> QStringList convertUris(const T& uris) {
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

/**
 * Returns true if the given hash contains at least one of the possible combinations of con and "c in candidates".
 */
bool hashContainsAtLeastOneOf(Nepomuk::ResourceWatcherConnection* con, const QSet<QUrl>& candidates, const QMultiHash<QUrl, Nepomuk::ResourceWatcherConnection*>& hash) {
    for(QSet<QUrl>::const_iterator it = candidates.constBegin();
        it != candidates.constEnd(); ++it) {
        if(hash.contains(*it, con)) {
            return true;
        }
    }
    return false;
}
}


Nepomuk::ResourceWatcherManager::ResourceWatcherManager(DataManagementModel* parent)
    : QObject(parent),
      m_model(parent),
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
    // FIXME!
    kError() << "FIXME";
    //addProperty( st.subject(), st.predicate().uri(), QList<Soprano::Node>() << st.object() );
}


void Nepomuk::ResourceWatcherManager::changeProperty(const QUrl &res, const QUrl &property, const QList<Soprano::Node> &addedValues, const QList<Soprano::Node> &removedValues)
{
    kDebug() << res << property << addedValues << removedValues;

    //
    // We only need the resource types if any connections are watching types.
    //
    QSet<QUrl> types;
    if(!m_typeHash.isEmpty()) {
        types = getTypes(res);
    }


    //
    // special case: rdf:type
    //
    if(property == RDF::type()) {
        QSet<QUrl> addedTypes, removedTypes;
        for(QList<Soprano::Node>::const_iterator it = addedValues.constBegin();
            it != addedValues.constEnd(); ++it) {
            addedTypes << it->uri();
        }
        for(QList<Soprano::Node>::const_iterator it = removedValues.constBegin();
            it != removedValues.constEnd(); ++it) {
            removedTypes << it->uri();
        }
        changeTypes(res, types, addedTypes, removedTypes);
    }


    // first collect all the connections we need to emit the signals for
    QSet<ResourceWatcherConnection*> connections;

    //
    // Emit signals for all the connections that are only watching specific resources
    //
    foreach( ResourceWatcherConnection* con, m_resHash.values( res ) ) {
        if( m_propHash.contains(property, con) ||
            !m_propHash.values().contains(con) ) {
            connections << con;
        }
    }


    //
    // Emit signals for the connections that are watching specific resources and properties
    //
    foreach( ResourceWatcherConnection* con, m_propHash.values( property ) ) {
        //
        // Emit for those connections which watch the property and either no
        // type or once of the types of the resource.
        // Only query the types if we have any type watching connections.
        //
        bool conIsWatchingResType = !m_typeHash.values().contains(con);
        foreach(const QUrl& type, types) {
            if(m_typeHash.contains(type, con)) {
                conIsWatchingResType = true;
                break;
            }
        }

        if( !m_resHash.values().contains(con) && conIsWatchingResType ) {
            connections << con;
        }
    }



    //
    // Emit signals for all connections which watch one of the types of the resource
    // but no properties (that is handled above).
    //
    foreach(const QUrl& type, types) {
        foreach(ResourceWatcherConnection* con, m_typeHash.values(type)) {
            if(!m_propHash.values(property).contains(con)) {
                connections << con;
            }
        }
    }


    //
    // Finally emit the signals for all connections
    //
    foreach(ResourceWatcherConnection* con, connections) {
        emit con->propertyChanged( convertUri(res),
                                   convertUri(property),
                                   nodeListToVariantList(addedValues),
                                   nodeListToVariantList(removedValues) );
        if(!addedValues.isEmpty()) {
            emit con->propertyAdded(convertUri(res),
                                    convertUri(property),
                                    nodeListToVariantList(addedValues));
        }
        if(!removedValues.isEmpty()) {
            emit con->propertyRemoved(convertUri(res),
                                      convertUri(property),
                                      nodeListToVariantList(removedValues));
        }
    }
}

void Nepomuk::ResourceWatcherManager::changeProperty(const QMultiHash< QUrl, Soprano::Node >& oldValues,
                                                     const QUrl& property,
                                                     const QList<Soprano::Node>& nodes)
{
    QList<QUrl> uniqueKeys = oldValues.keys();
    foreach( const QUrl resUri, uniqueKeys ) {
        const QList<Soprano::Node> old = oldValues.values( resUri );
        changeProperty(resUri, property, old, nodes);
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

QSet<QUrl> Nepomuk::ResourceWatcherManager::getTypes(const Soprano::Node &res) const
{
    QSet<QUrl> types;
    Soprano::StatementIterator it = m_model->listStatements(res, RDF::type(), Soprano::Node());
    while(it.next()) {
        types.insert(it.current().object().uri());
    }
    return types;
}

// FIXME: also take super-classes into account
void Nepomuk::ResourceWatcherManager::changeTypes(const QUrl &res, const QSet<QUrl>& resTypes, const QSet<QUrl> &addedTypes, const QSet<QUrl> &removedTypes)
{
    // first collect all the connections we need to emit the signals for
    QSet<ResourceWatcherConnection*> addConnections, removeConnections;

    // all connections watching the resource and not a special property
    // and no special type or one of the changed types
    foreach( ResourceWatcherConnection* con, m_resHash.values( res ) ) {
        if( m_propHash.contains(RDF::type(), con) ||
            !m_propHash.values().contains(con) ) {
            if(!addedTypes.isEmpty() &&
               connectionWatchesOneType(con, addedTypes)) {
                addConnections << con;
            }
            if(!removedTypes.isEmpty() &&
               connectionWatchesOneType(con, removedTypes)) {
                removeConnections << con;
            }
        }
    }

    // all connections watching one of the types and no special resource or property
    if(!addedTypes.isEmpty()) {
        foreach(const QUrl& type, addedTypes + resTypes) {
            foreach(ResourceWatcherConnection* con, m_typeHash.values(type)) {
                if(!m_resHash.values().contains(con) &&
                   !m_propHash.values().contains(con)) {
                    addConnections << con;
                }
            }
        }
    }
    if(!removedTypes.isEmpty()) {
        foreach(const QUrl& type, removedTypes + resTypes) {
            foreach(ResourceWatcherConnection* con, m_typeHash.values(type)) {
                if(!m_resHash.values().contains(con) &&
                   !m_propHash.values().contains(con)) {
                    removeConnections << con;
                }
            }
        }
    }

    // all connections watching rdf:type
    foreach(ResourceWatcherConnection* con, m_propHash.values(RDF::type())) {
        if(!m_resHash.values().contains(con) ) {
            if(connectionWatchesOneType(con, addedTypes + resTypes)) {
                addConnections << con;
            }
            if(connectionWatchesOneType(con, removedTypes + resTypes)) {
                removeConnections << con;
            }
        }
    }

    // finally emit the actual signals
    if(!addedTypes.isEmpty()) {
        foreach(ResourceWatcherConnection* con, addConnections) {
            emit con->resourceTypesAdded(convertUri(res),
                                         convertUris(addedTypes));
        }
    }
    if(!removedTypes.isEmpty()) {
        foreach(ResourceWatcherConnection* con, removeConnections) {
            emit con->resourceTypesRemoved(convertUri(res),
                                           convertUris(removedTypes));
        }
    }
}

bool Nepomuk::ResourceWatcherManager::connectionWatchesOneType(Nepomuk::ResourceWatcherConnection *con, const QSet<QUrl> &types) const
{
    return !m_typeHash.values().contains(con) || hashContainsAtLeastOneOf(con, types, m_typeHash);
}

#include "resourcewatchermanager.moc"
