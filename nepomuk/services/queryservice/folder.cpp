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

#include "folder.h"
#include "folderconnection.h"
#include "queryservice.h"
#include "countqueryrunnable.h"
#include "searchrunnable.h"

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>

#include <Soprano/Model>

#include <KDebug>

#include <QtCore/QThreadPool>


Nepomuk::Query::Folder::Folder( const Query& query, QObject* parent )
    : QObject( parent ),
      m_isSparqlQueryFolder( false ),
      m_query( query ),
      m_currentSearchRunnable( 0 )
{
    init();
}


Nepomuk::Query::Folder::Folder( const QString& query, const RequestPropertyMap& requestProps, QObject* parent )
    : QObject( parent ),
      m_isSparqlQueryFolder( true ),
      m_sparqlQuery( query ),
      m_requestProperties( requestProps ),
      m_currentSearchRunnable( 0 )
{
    init();
}


void Nepomuk::Query::Folder::init()
{
    m_totalCount = -1;
    m_initialListingDone = false;
    m_storageChanged = false;

    m_updateTimer.setSingleShot( true );
    m_updateTimer.setInterval( 2000 );

    connect( ResourceManager::instance()->mainModel(), SIGNAL( statementsAdded() ),
             this, SLOT( slotStorageChanged() ) );
    connect( ResourceManager::instance()->mainModel(), SIGNAL( statementsRemoved() ),
             this, SLOT( slotStorageChanged() ) );
    connect( &m_updateTimer, SIGNAL( timeout() ),
             this, SLOT( slotUpdateTimeout() ) );
}


Nepomuk::Query::Folder::~Folder()
{
    if( m_currentSearchRunnable )
        m_currentSearchRunnable->cancel();

    // cannot use qDeleteAll since deleting a connection changes m_connections
    while ( !m_connections.isEmpty() )
        delete m_connections.first();
}


void Nepomuk::Query::Folder::update()
{
    if ( !m_currentSearchRunnable ) {
        m_currentSearchRunnable = new SearchRunnable( this );
        QueryService::searchThreadPool()->start( m_currentSearchRunnable, 1 );

        // we only need the count for initialListingDone
        if ( !m_initialListingDone &&
             m_sparqlQuery.isEmpty() ) {
            QueryService::searchThreadPool()->start( new CountQueryRunnable( this ), 0 );
        }
    }
}


QList<Nepomuk::Query::Result> Nepomuk::Query::Folder::entries() const
{
    return m_results.values();
}


bool Nepomuk::Query::Folder::initialListingDone() const
{
    return m_initialListingDone;
}


QString Nepomuk::Query::Folder::sparqlQuery() const
{
    if ( m_sparqlQuery.isEmpty() )
        return m_query.toSparqlQuery();
    else
        return m_sparqlQuery;
}


Nepomuk::Query::RequestPropertyMap Nepomuk::Query::Folder::requestPropertyMap() const
{
    if ( m_sparqlQuery.isEmpty() )
        return m_query.requestPropertyMap();
    else
        return m_requestProperties;
}


void Nepomuk::Query::Folder::addResult( const Nepomuk::Query::Result& result )
{
    if ( m_initialListingDone ) {
        m_newResults.insert( result.resource().resourceUri(), result );
        if ( !m_results.contains( result.resource().resourceUri() ) ) {
            emit newEntries( QList<Result>() << result );
        }
    }
    else {
        m_results.insert( result.resource().resourceUri(), result );
        emit newEntries( QList<Result>() << result );
    }
}


void Nepomuk::Query::Folder::listingFinished()
{
    m_currentSearchRunnable = 0;

    if ( m_initialListingDone ) {
        // inform about removed items
        foreach( const Result& result, m_results ) {
            if ( !m_newResults.contains( result.resource().resourceUri() ) ) {
                emit entriesRemoved( QList<QUrl>() << result.resource().resourceUri() );
            }
        }

        // reset
        m_results = m_newResults;
        m_newResults.clear();
    }
    else {
        kDebug() << "Listing done. Total:" << m_results.count();
        m_initialListingDone = true;
        emit finishedListing();
    }

    // make sure we do not update again right away
    m_updateTimer.start();
}


void Nepomuk::Query::Folder::slotStorageChanged()
{
    if ( !m_updateTimer.isActive() && !m_currentSearchRunnable ) {
        update();
    }
    else {
        m_storageChanged = true;
    }
}


// if there was a change in the nepomuk store we update
void Nepomuk::Query::Folder::slotUpdateTimeout()
{
    if ( m_storageChanged && !m_currentSearchRunnable ) {
        m_storageChanged = false;
        update();
    }
}


void Nepomuk::Query::Folder::countQueryFinished( int count )
{
    m_totalCount = count;
    kDebug() << m_totalCount;
    if( count >= 0 )
        emit totalCount( m_totalCount );
}


void Nepomuk::Query::Folder::addConnection( FolderConnection* conn )
{
    Q_ASSERT( conn != 0 );
    Q_ASSERT( !m_connections.contains( conn ) );

    m_connections.append( conn );
}


void Nepomuk::Query::Folder::removeConnection( FolderConnection* conn )
{
    Q_ASSERT( conn != 0 );
    Q_ASSERT( m_connections.contains( conn ) );

    m_connections.removeAll( conn );

    if ( m_connections.isEmpty() ) {
        kDebug() << "Folder unused. Deleting.";
        emit aboutToBeDeleted( this );
        deleteLater();
    }
}


QList<Nepomuk::Query::FolderConnection*> Nepomuk::Query::Folder::openConnections() const
{
    return m_connections;
}

#include "folder.moc"
