/*
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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
#include "query.h"
#include "queryparser.h"
#include "queryadaptor.h"
#include "dbusoperators.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusMessage>

#include <KPluginFactory>
#include <KDebug>


NEPOMUK_EXPORT_SERVICE( Nepomuk::Search::QueryService, "nepomukqueryservice" )


Nepomuk::Search::QueryService* Nepomuk::Search::QueryService::s_instance = 0;

Nepomuk::Search::QueryService::QueryService( QObject* parent, const QVariantList& )
    : Service( parent ),
      m_folderConnectionCnt( 0 )
{
    Nepomuk::Search::registerDBusTypes();

    s_instance = this;

    connect( QDBusConnection::sessionBus().interface(),
             SIGNAL( serviceOwnerChanged( const QString&, const QString&, const QString& ) ),
             this,
             SLOT( slotServiceOwnerChanged( const QString&, const QString&, const QString& ) ) );
}


Nepomuk::Search::QueryService::~QueryService()
{
}


QDBusObjectPath Nepomuk::Search::QueryService::query( const QString& queryString, const QStringList& props, const QDBusMessage& msg )
{
    kDebug() << "Query request:" << queryString;

    // create query folder + connection
    Query q = QueryParser::parseQuery( queryString );
    foreach( const QString& rp, props ) {
        q.addRequestProperty( QUrl( rp ) );
    }
    return query( q, msg );
}


QDBusObjectPath Nepomuk::Search::QueryService::query( const Nepomuk::Search::Query& q, const QDBusMessage& msg )
{
    kDebug() << "Query request:" << q;

    Folder* folder = getFolder( q );
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

    return QDBusObjectPath( dbusObjectPath );
}


Soprano::Model* Nepomuk::Search::QueryService::mainModel()
{
    return Nepomuk::Service::mainModel();
}


Nepomuk::Search::QueryService* Nepomuk::Search::QueryService::instance()
{
    return s_instance;
}


Nepomuk::Search::Folder* Nepomuk::Search::QueryService::getFolder( const Query& query )
{
    QHash<Query, Folder*>::iterator it = m_openFolders.find( query );
    if ( it != m_openFolders.end() ) {
        kDebug() << "Recycling folder" << *it;
        return *it;
    }
    else {
        kDebug() << "Creating new search folder for query:" << query;
        Folder* newFolder = new Folder( query );
        connect( newFolder, SIGNAL( destroyed( QObject* ) ),
                 this, SLOT( slotFolderDestroyed( QObject* ) ) );
        m_openFolders.insert( query, newFolder );
        m_folderQueryHash.insert( newFolder, query );
        return newFolder;
    }
}


void Nepomuk::Search::QueryService::slotFolderDestroyed( QObject* folder )
{
    kDebug() << folder;
    QHash<Folder*, Query>::iterator it = m_folderQueryHash.find( ( Folder* )folder );
    if ( it != m_folderQueryHash.end() ) {
        m_openFolders.remove( *it );
        m_folderQueryHash.erase( it );
    }
}


void Nepomuk::Search::QueryService::slotFolderConnectionDestroyed( QObject* o )
{
    kDebug() << o;
    FolderConnection* conn = ( FolderConnection* )o;
    QHash<FolderConnection*, QString>::iterator it = m_connectionDBusServiceHash.find( conn );
    if ( it != m_connectionDBusServiceHash.end() ) {
        m_openConnections.remove( *it, conn );
        m_connectionDBusServiceHash.erase( it );
    }
}


void Nepomuk::Search::QueryService::slotServiceOwnerChanged( const QString& serviceName,
                                                             const QString&,
                                                             const QString& newOwner )
{
    if ( newOwner.isEmpty() ) {
        QList<FolderConnection*> conns = m_openConnections.values( serviceName );
        if ( !conns.isEmpty() ) {
            kDebug() << "Service" << serviceName << "went down. Removing connections";
            // hash cleanup will be triggered automatically
            qDeleteAll( conns );
        }
    }
}

#include "queryservice.moc"
