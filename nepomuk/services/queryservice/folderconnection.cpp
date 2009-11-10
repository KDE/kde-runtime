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

#include "folderconnection.h"
#include "folder.h"

#include <QtCore/QStringList>

#include <KDebug>

Nepomuk::Query::FolderConnection::FolderConnection( Folder* folder )
    : QObject( folder ),
      m_folder( folder )
{
    connect( m_folder, SIGNAL( newEntries( QList<Nepomuk::Query::Result> ) ),
             this, SIGNAL( newEntries( QList<Nepomuk::Query::Result> ) ) );
    connect( m_folder, SIGNAL( entriesRemoved( QList<QUrl> ) ),
             this, SLOT( slotEntriesRemoved( QList<QUrl> ) ) );
    connect( m_folder, SIGNAL( finishedListing() ),
             this, SIGNAL( finishedListing() ) );

    m_folder->addConnection( this );
}


Nepomuk::Query::FolderConnection::~FolderConnection()
{
    m_folder->removeConnection( this );
}


void Nepomuk::Query::FolderConnection::list()
{
    kDebug();
    if ( !m_folder->entries().isEmpty() ) {
        emit newEntries( m_folder->entries() );
    }
    if ( m_folder->initialListingDone() ) {
        emit finishedListing();
    }
    else {
        // make sure the search has actually been started
        m_folder->update();
    }
}


void Nepomuk::Query::FolderConnection::slotEntriesRemoved( QList<QUrl> entries )
{
    QStringList uris;
    for ( int i = 0; i < entries.count(); ++i ) {
        uris.append( entries[i].toString() );
    }
    emit entriesRemoved( uris );
}


void Nepomuk::Query::FolderConnection::close()
{
    kDebug();
    deleteLater();
}

#include "folderconnection.moc"
