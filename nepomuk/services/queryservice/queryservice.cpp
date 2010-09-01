/*
  Copyright (c) 2008-2010 Sebastian Trueg <trueg@kde.org>

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
#include "dbusoperators_p.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusMessage>

#include <KPluginFactory>
#include <KUrl>
#include <KDebug>

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Types/Property>
#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/QueryParser>


NEPOMUK_EXPORT_SERVICE( Nepomuk::Query::QueryService, "nepomukqueryservice" )


Nepomuk::Query::QueryService::QueryService( QObject* parent, const QVariantList& )
    : Service( parent ),
      m_folderConnectionCnt( 0 )
{
    Nepomuk::Query::registerDBusTypes();
}


Nepomuk::Query::QueryService::~QueryService()
{
}


QDBusObjectPath Nepomuk::Query::QueryService::query( const QString& query, const QDBusMessage& msg )
{
    Query q = Query::fromString( query );
    if ( !q.isValid() ) {
        // backwards compatibility: in KDE <= 4.5 query() was what desktopQuery() is now
        return desktopQuery( query, msg );
    }

    kDebug() << "Query request:" << q;
    Folder* folder = getFolder( q );
    return ( new FolderConnection( folder ) )->registerDBusObject( msg.service(), ++m_folderConnectionCnt );
}


QDBusObjectPath Nepomuk::Query::QueryService::desktopQuery( const QString& query, const QDBusMessage& msg )
{
    Query q = QueryParser::parseQuery( query );
    kDebug() << "Query request:" << q;
    Folder* folder = getFolder( q );
    return ( new FolderConnection( folder ) )->registerDBusObject( msg.service(), ++m_folderConnectionCnt );
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
    return ( new FolderConnection( folder ) )->registerDBusObject( msg.service(), ++m_folderConnectionCnt );
}


Nepomuk::Query::Folder* Nepomuk::Query::QueryService::getFolder( const Query& query )
{
    QHash<Query, Folder*>::const_iterator it = m_openQueryFolders.constFind( query );
    if ( it != m_openQueryFolders.constEnd() ) {
        kDebug() << "Recycling folder" << *it;
        return *it;
    }
    else {
        kDebug() << "Creating new search folder for query:" << query;
        Folder* newFolder = new Folder( query, this );
        connect( newFolder, SIGNAL( aboutToBeDeleted( Nepomuk::Query::Folder* ) ),
                 this, SLOT( slotFolderAboutToBeDeleted( Nepomuk::Query::Folder* ) ) );
        m_openQueryFolders.insert( query, newFolder );
        return newFolder;
    }
}


Nepomuk::Query::Folder* Nepomuk::Query::QueryService::getFolder( const QString& query, const Nepomuk::Query::RequestPropertyMap& requestProps )
{
    QHash<QString, Folder*>::const_iterator it = m_openSparqlFolders.constFind( query );
    if ( it != m_openSparqlFolders.constEnd() ) {
        kDebug() << "Recycling folder" << *it;
        return *it;
    }
    else {
        kDebug() << "Creating new search folder for query:" << query;
        Folder* newFolder = new Folder( query, requestProps, this );
        connect( newFolder, SIGNAL( aboutToBeDeleted( Nepomuk::Query::Folder* ) ),
                 this, SLOT( slotFolderAboutToBeDeleted( Nepomuk::Query::Folder* ) ) );
        m_openSparqlFolders.insert( query, newFolder );
        return newFolder;
    }
}


void Nepomuk::Query::QueryService::slotFolderAboutToBeDeleted( Folder* folder )
{
    kDebug() << folder;
    if ( folder->query().isValid() )
        m_openQueryFolders.remove( folder->query() );
    else
        m_openSparqlFolders.remove( folder->sparqlQuery() );
}

#include "queryservice.moc"
