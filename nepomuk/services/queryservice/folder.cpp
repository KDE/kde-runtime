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

#include "folder.h"
#include "folderconnection.h"
#include "queryservice.h"

#include <Soprano/Model>

#include <KDebug>


Nepomuk::Search::Folder::Folder( const Query& query, QObject* parent )
    : QObject( parent ),
      m_query( query ),
      m_initialListingDone( false ),
      m_storageChanged( false )
{
    m_updateTimer.setSingleShot( true );
    m_updateTimer.setInterval( 2000 );

    m_searchCore = new SearchCore( this );

    connect( m_searchCore, SIGNAL( newResult( const Nepomuk::Search::Result& ) ),
             this, SLOT( slotSearchNewResult( const Nepomuk::Search::Result& ) ) );
    connect( m_searchCore, SIGNAL( scoreChanged( const Nepomuk::Search::Result& ) ),
             this, SLOT( slotSearchScoreChanged( const Nepomuk::Search::Result& ) ) );
    connect( m_searchCore, SIGNAL( finished() ),
             this, SLOT( slotSearchFinished() ) );
    connect( QueryService::instance()->mainModel(), SIGNAL( statementsAdded() ),
             this, SLOT( slotStorageChanged() ) );
    connect( QueryService::instance()->mainModel(), SIGNAL( statementsRemoved() ),
             this, SLOT( slotStorageChanged() ) );
    connect( &m_updateTimer, SIGNAL( timeout() ),
             this, SLOT( slotUpdateTimeout() ) );
}


Nepomuk::Search::Folder::~Folder()
{
}


void Nepomuk::Search::Folder::update()
{
    if ( !m_searchCore->isActive() ) {
        // run the search and forward signals to all connections that requested it
        m_searchCore->query( m_query );
    }
}


QList<Nepomuk::Search::Result> Nepomuk::Search::Folder::entries() const
{
    return m_results.values();
}


bool Nepomuk::Search::Folder::initialListingDone() const
{
    return m_initialListingDone;
}


void Nepomuk::Search::Folder::slotSearchNewResult( const Nepomuk::Search::Result& result )
{
    if ( m_initialListingDone ) {
        m_newResults.insert( result.resourceUri(), result );
        if ( !m_results.contains( result.resourceUri() ) ) {
            emit newEntries( QList<Result>() << result );
        }
    }
    else {
        m_results.insert( result.resourceUri(), result );
        emit newEntries( QList<Result>() << result );
    }
}


void Nepomuk::Search::Folder::slotSearchScoreChanged( const Nepomuk::Search::Result& )
{
    if ( m_initialListingDone ) {
#warning FIXME: handle scoreChanged
    }
    else {

    }
}


void Nepomuk::Search::Folder::slotSearchFinished()
{
    if ( m_initialListingDone ) {
        // inform about removed items
        foreach( const Result& result, m_results ) {
            if ( !m_newResults.contains( result.resourceUri() ) ) {
                emit entriesRemoved( QList<QUrl>() << result.resourceUri() );
            }
        }

        // reset
        m_results = m_newResults;
        m_newResults.clear();
    }
    else {
        m_initialListingDone = true;
        emit finishedListing();
    }

    // make sure we do not update again right away
    m_updateTimer.start();
}


void Nepomuk::Search::Folder::slotStorageChanged()
{
    if ( !m_updateTimer.isActive() && !m_searchCore->isActive() ) {
        update();
    }
    else {
        m_storageChanged = true;
    }
}


// if there was a change in the nepomuk store we update
void Nepomuk::Search::Folder::slotUpdateTimeout()
{
    if ( m_storageChanged && !m_searchCore->isActive() ) {
        m_storageChanged = false;
        update();
    }
}


void Nepomuk::Search::Folder::addConnection( FolderConnection* conn )
{
    Q_ASSERT( conn != 0 );
    Q_ASSERT( !m_connections.contains( conn ) );

    m_connections.append( conn );
}


void Nepomuk::Search::Folder::removeConnection( FolderConnection* conn )
{
    Q_ASSERT( conn != 0 );
    Q_ASSERT( m_connections.contains( conn ) );

    m_connections.removeAll( conn );

    if ( m_connections.isEmpty() ) {
        kDebug() << "Folder unused. Deleting.";
        deleteLater();
    }
}


QList<Nepomuk::Search::FolderConnection*> Nepomuk::Search::Folder::openConnections() const
{
    return m_connections;
}

#include "folder.moc"
