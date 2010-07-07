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

#include <Soprano/Model>
#include <Soprano/Node>
#include <Soprano/Util/AsyncQuery>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>

#include <KDebug>


Nepomuk::Query::Folder::Folder( const Query& query, QObject* parent )
    : QObject( parent ),
      m_query( query )
{
    init();
}


Nepomuk::Query::Folder::Folder( const QString& query, const RequestPropertyMap& requestProps, QObject* parent )
    : QObject( parent ),
      m_sparqlQuery( query ),
      m_requestProperties( requestProps )
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

    m_searchThread = new SearchThread( this );

    connect( m_searchThread, SIGNAL( newResult( const Nepomuk::Query::Result& ) ),
             this, SLOT( slotSearchNewResult( const Nepomuk::Query::Result& ) ),
             Qt::QueuedConnection );
    connect( m_searchThread, SIGNAL( finished() ),
             this, SLOT( slotSearchFinished() ) );
    connect( ResourceManager::instance()->mainModel(), SIGNAL( statementsAdded() ),
             this, SLOT( slotStorageChanged() ) );
    connect( ResourceManager::instance()->mainModel(), SIGNAL( statementsRemoved() ),
             this, SLOT( slotStorageChanged() ) );
    connect( &m_updateTimer, SIGNAL( timeout() ),
             this, SLOT( slotUpdateTimeout() ) );
}


Nepomuk::Query::Folder::~Folder()
{
    m_searchThread->cancel();
}


void Nepomuk::Query::Folder::update()
{
    if ( !m_searchThread->isRunning() ) {
        if ( m_query.isValid() ) {
            // count the results
            kDebug() << "Count query:" << m_query.toSparqlQuery( Query::CreateCountQuery );
            delete m_countQuery;
            m_countQuery = Soprano::Util::AsyncQuery::executeQuery( ResourceManager::instance()->mainModel(),
                                                                    m_query.toSparqlQuery( Query::CreateCountQuery ),
                                                                    Soprano::Query::QueryLanguageSparql );
            connect( m_countQuery, SIGNAL( nextReady( Soprano::Util::AsyncQuery* ) ),
                     this, SLOT( slotCountQueryFinished( Soprano::Util::AsyncQuery* ) ) );

            // and get the results
            m_searchThread->query( m_query.toSparqlQuery(), m_query.requestPropertyMap() );
        }
        else {
            m_searchThread->query( m_sparqlQuery, m_requestProperties );
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
    if ( m_query.isValid() )
        return m_query.toSparqlQuery();
    else
        return m_sparqlQuery;
}


void Nepomuk::Query::Folder::slotSearchNewResult( const Nepomuk::Query::Result& result )
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


void Nepomuk::Query::Folder::slotSearchFinished()
{
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
    if ( !m_updateTimer.isActive() && !m_searchThread->isRunning() ) {
        update();
    }
    else {
        m_storageChanged = true;
    }
}


// if there was a change in the nepomuk store we update
void Nepomuk::Query::Folder::slotUpdateTimeout()
{
    if ( m_storageChanged && !m_searchThread->isRunning() ) {
        m_storageChanged = false;
        update();
    }
}


void Nepomuk::Query::Folder::slotCountQueryFinished( Soprano::Util::AsyncQuery* query )
{
    m_countQuery = 0;
    m_totalCount = query->binding( 0 ).literal().toInt();
    kDebug() << m_totalCount;
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
