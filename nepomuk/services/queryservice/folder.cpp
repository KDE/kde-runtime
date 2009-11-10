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

#include "folder.h"
#include "folderconnection.h"
#include "queryservice.h"

#include <Soprano/Model>
#include <Soprano/Node>

#include <Nepomuk/Resource>

#include <KDebug>


Nepomuk::Query::Folder::Folder( const QString& query, const RequestPropertyMap& requestProps, QObject* parent )
    : QObject( parent ),
      m_query( query ),
      m_requestProperties( requestProps ),
      m_initialListingDone( false ),
      m_storageChanged( false )
{
    m_updateTimer.setSingleShot( true );
    m_updateTimer.setInterval( 2000 );

    m_searchCore = new SearchCore( this );

    connect( m_searchCore, SIGNAL( newResult( const Nepomuk::Query::Result& ) ),
             this, SLOT( slotSearchNewResult( const Nepomuk::Query::Result& ) ) );
    connect( m_searchCore, SIGNAL( scoreChanged( const Nepomuk::Query::Result& ) ),
             this, SLOT( slotSearchScoreChanged( const Nepomuk::Query::Result& ) ) );
    connect( m_searchCore, SIGNAL( finished() ),
             this, SLOT( slotSearchFinished() ) );
    connect( QueryService::instance()->mainModel(), SIGNAL( statementsAdded() ),
             this, SLOT( slotStorageChanged() ) );
    connect( QueryService::instance()->mainModel(), SIGNAL( statementsRemoved() ),
             this, SLOT( slotStorageChanged() ) );
    connect( &m_updateTimer, SIGNAL( timeout() ),
             this, SLOT( slotUpdateTimeout() ) );
}


Nepomuk::Query::Folder::~Folder()
{
}


void Nepomuk::Query::Folder::update()
{
    if ( !m_searchCore->isActive() ) {
        // run the search and forward signals to all connections that requested it
        m_searchCore->query( m_query, m_requestProperties );
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


void Nepomuk::Query::Folder::slotSearchScoreChanged( const Nepomuk::Query::Result& )
{
    // TODO: implement this
    if ( m_initialListingDone ) {

    }
    else {

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
        m_initialListingDone = true;
        emit finishedListing();
    }

    // make sure we do not update again right away
    m_updateTimer.start();
}


void Nepomuk::Query::Folder::slotStorageChanged()
{
    if ( !m_updateTimer.isActive() && !m_searchCore->isActive() ) {
        update();
    }
    else {
        m_storageChanged = true;
    }
}


// if there was a change in the nepomuk store we update
void Nepomuk::Query::Folder::slotUpdateTimeout()
{
    if ( m_storageChanged && !m_searchCore->isActive() ) {
        m_storageChanged = false;
        update();
    }
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
        deleteLater();
    }
}


QList<Nepomuk::Query::FolderConnection*> Nepomuk::Query::Folder::openConnections() const
{
    return m_connections;
}

#include "folder.moc"
