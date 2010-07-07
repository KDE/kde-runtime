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

#include "folderconnection.h"
#include "folder.h"
#include "queryadaptor.h"

#include <QtCore/QStringList>
#include <QtDBus/QDBusServiceWatcher>
#include <QtDBus/QDBusConnection>

#include <KDebug>

Nepomuk::Query::FolderConnection::FolderConnection( Folder* folder )
    : QObject( folder ),
      m_folder( folder )
{
    m_folder->addConnection( this );
}


Nepomuk::Query::FolderConnection::~FolderConnection()
{
    m_folder->removeConnection( this );
}


void Nepomuk::Query::FolderConnection::list()
{
    kDebug();

    m_folder->disconnect( this );
    connect( m_folder, SIGNAL( newEntries( QList<Nepomuk::Query::Result> ) ),
             this, SIGNAL( newEntries( QList<Nepomuk::Query::Result> ) ) );
    connect( m_folder, SIGNAL( entriesRemoved( QList<QUrl> ) ),
             this, SLOT( slotEntriesRemoved( QList<QUrl> ) ) );
    connect( m_folder, SIGNAL( totalCount( int ) ),
             this, SIGNAL( totalCount( int ) ) );

    // report cached entries
    if ( !m_folder->entries().isEmpty() ) {
        emit newEntries( m_folder->entries() );
    }

    // report listing finished or connect to the folder
    if ( m_folder->initialListingDone() ) {
        emit finishedListing();
    }
    else {
        // We do NOT connect to slotFinishedListing!
        connect( m_folder, SIGNAL( finishedListing() ),
                 this, SIGNAL( finishedListing() ) );

        // make sure the search has actually been started
        m_folder->update();
    }
}


void Nepomuk::Query::FolderConnection::listen()
{
    m_folder->disconnect( this );

    // if the folder has already finished listing it will only emit
    // changed - exactly what we want
    if ( m_folder->initialListingDone() ) {
        connect( m_folder, SIGNAL( newEntries( QList<Nepomuk::Query::Result> ) ),
                 this, SIGNAL( newEntries( QList<Nepomuk::Query::Result> ) ) );
        connect( m_folder, SIGNAL( entriesRemoved( QList<QUrl> ) ),
                 this, SLOT( slotEntriesRemoved( QList<QUrl> ) ) );
        connect( m_folder, SIGNAL( totalCount( int ) ),
                 this, SLOT( totalCount( int ) ) );
    }

    // otherwise we need to wait for it finishing the listing
    else {
        connect( m_folder, SIGNAL( finishedListing() ),
                 this, SLOT( slotFinishedListing() ) );
    }
}


void Nepomuk::Query::FolderConnection::slotEntriesRemoved( const QList<QUrl>& entries )
{
    QStringList uris;
    for ( int i = 0; i < entries.count(); ++i ) {
        uris.append( entries[i].toString() );
    }
    emit entriesRemoved( uris );
}


void Nepomuk::Query::FolderConnection::slotFinishedListing()
{
    // this slot is only called in listen mode. Once the listing is
    // finished we can start listening for changes
    connect( m_folder, SIGNAL( newEntries( QList<Nepomuk::Query::Result> ) ),
             this, SIGNAL( newEntries( QList<Nepomuk::Query::Result> ) ) );
    connect( m_folder, SIGNAL( entriesRemoved( QList<QUrl> ) ),
             this, SLOT( slotEntriesRemoved( QList<QUrl> ) ) );
}


void Nepomuk::Query::FolderConnection::close()
{
    kDebug();
    deleteLater();
}


bool Nepomuk::Query::FolderConnection::isListingFinished() const
{
    return m_folder->initialListingDone();
}


QString Nepomuk::Query::FolderConnection::queryString() const
{
    return m_folder->sparqlQuery();
}


QDBusObjectPath Nepomuk::Query::FolderConnection::registerDBusObject( const QString& dbusClient, int id )
{
    // create the query adaptor on this connection
    ( void )new QueryAdaptor( this );

    // build the dbus object path from the id and register the connection as a Query dbus object
    const QString dbusObjectPath = QString( "/nepomukqueryservice/query%1" ).arg( id );
    QDBusConnection::sessionBus().registerObject( dbusObjectPath, this );

    // watch the dbus client for unregistration for auto-cleanup
    m_serviceWatcher = new QDBusServiceWatcher( QString(),
                                                QDBusConnection::sessionBus(),
                                                QDBusServiceWatcher::WatchForUnregistration,
                                                this );
    connect( m_serviceWatcher, SIGNAL(serviceUnregistered(const QString&)),
             this, SLOT(close()) );
    m_serviceWatcher->addWatchedService( dbusClient );

    // finally return the dbus object path this connection can be found on
    return QDBusObjectPath( dbusObjectPath );
}

#include "folderconnection.moc"
