/*
  Copyright (c) 2008-2009 Sebastian Trueg <trueg@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License version 2 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "queryservice.h"
#include "folder.h"
#include "folderconnection.h"
#include "queryadaptor.h"
#include "dbusoperators_p.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusServiceWatcher>

#include <KPluginFactory>
#include <KUrl>
#include <KDebug>

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Types/Property>
#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/QueryParser>


NEPOMUK_EXPORT_SERVICE( Nepomuk::Query::QueryService, "nepomukqueryservice" )


Nepomuk::Query::QueryService* Nepomuk::Query::QueryService::s_instance = 0;

Nepomuk::Query::QueryService::QueryService( QObject* parent, const QVariantList& )
    : Service( parent ),
      m_folderConnectionCnt( 0 )
{
    Nepomuk::Query::registerDBusTypes();

    s_instance = this;

    m_serviceWatcher = new QDBusServiceWatcher( this );
    m_serviceWatcher->setWatchMode( QDBusServiceWatcher::WatchForUnregistration );

    connect( m_serviceWatcher, SIGNAL( serviceUnregistered(const QString& ) ),
             this, SLOT( slotServiceUnregistered( const QString& ) ) );
}


Nepomuk::Query::QueryService::~QueryService()
{
    // cannot use qDeleteAll since deleting a folder changes m_openQueryFolders
    while ( !m_openConnections.isEmpty() )
        delete m_openConnections.begin().value();
}


QDBusObjectPath Nepomuk::Query::QueryService::query( const QString& query, const QDBusMessage& msg )
{
    Query q = QueryParser::parseQuery( query );
    kDebug() << "Query request:" << q;
    QString sparqlQueryString = q.toSparqlQuery();
    kDebug() << "Resolved query request:" << q << sparqlQueryString;
    return sparqlQuery( sparqlQueryString, RequestPropertyMapDBus(), msg );
}


namespace {
    Nepomuk::Query::RequestPropertyMap decodeRequestPropertiesList( const RequestPropertyMapDBus& requestProps )
    {
        Nepomuk::Query::RequestPropertyMap rpm;
        for ( RequestPropertyMapDBus::const_iterator it = requestProps.constBegin();
              it != requestProps.constEnd(); ++it )
            rpm.insert( it.key(), KUrl( it.value() ) );
        return rpm;
    }
}

QDBusObjectPath Nepomuk::Query::QueryService::sparqlQuery( const QString& sparql, const RequestPropertyMapDBus& requestProps, const QDBusMessage& msg )
{
    kDebug() << "Query request:" << sparql << requestProps;

    // create query folder + connection
    Folder* folder = getFolder( sparql, decodeRequestPropertiesList( requestProps ) );
    FolderConnection* conn = new FolderConnection( folder );
    connect( conn, SIGNAL( destroyed( QObject* ) ),
             this, SLOT( slotFolderConnectionDestroyed( QObject* ) ) );

    // register the new connection with dbus
    ( void )new QueryAdaptor( conn );
    QString dbusObjectPath = QString( "/nepomukqueryservice/query%1" ).arg( ++m_folderConnectionCnt );
    QDBusConnection::sessionBus().registerObject( dbusObjectPath, conn );

    // remember the client for automatic cleanup
    QString dbusClient = msg.service();
    m_openConnections.insert( dbusClient, conn );
    m_connectionDBusServiceHash.insert( conn, dbusClient );
    m_serviceWatcher->addWatchedService( dbusClient );

    return QDBusObjectPath( dbusObjectPath );
}


Nepomuk::Query::QueryService* Nepomuk::Query::QueryService::instance()
{
    return s_instance;
}


Nepomuk::Query::Folder* Nepomuk::Query::QueryService::getFolder( const QString& query, const Nepomuk::Query::RequestPropertyMap& requestProps )
{
    QHash<QString, Folder*>::iterator it = m_openFolders.find( query );
    if ( it != m_openFolders.end() ) {
        kDebug() << "Recycling folder" << *it;
        return *it;
    }
    else {
        kDebug() << "Creating new search folder for query:" << query;
        Folder* newFolder = new Folder( query, requestProps );
        connect( newFolder, SIGNAL( destroyed( QObject* ) ),
                 this, SLOT( slotFolderDestroyed( QObject* ) ) );
        m_openFolders.insert( query, newFolder );
        m_folderQueryHash.insert( newFolder, query );
        return newFolder;
    }
}


void Nepomuk::Query::QueryService::slotFolderDestroyed( QObject* folder )
{
    kDebug() << folder;
    QHash<Folder*, QString>::iterator it = m_folderQueryHash.find( ( Folder* )folder );
    if ( it != m_folderQueryHash.end() ) {
        m_openFolders.remove( *it );
        m_folderQueryHash.erase( it );
    }
}


void Nepomuk::Query::QueryService::slotFolderConnectionDestroyed( QObject* o )
{
    kDebug() << o;
    FolderConnection* conn = ( FolderConnection* )o;
    QHash<FolderConnection*, QString>::iterator it = m_connectionDBusServiceHash.find( conn );
    if ( it != m_connectionDBusServiceHash.end() ) {
        m_openConnections.remove( *it, conn );
        m_connectionDBusServiceHash.erase( it );
    }
}


void Nepomuk::Query::QueryService::slotServiceUnregistered( const QString& serviceName )
{
    QList<FolderConnection*> conns = m_openConnections.values( serviceName );
    if ( !conns.isEmpty() ) {
        kDebug() << "Service" << serviceName << "went down. Removing connections";
        // hash cleanup will be triggered automatically
        qDeleteAll( conns );
    }
    m_serviceWatcher->removeWatchedService(serviceName);
}

#include "queryservice.moc"
