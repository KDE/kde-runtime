/* This file is part of the KDE Project
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

#include "filesystemwatcher.h"

#include <QtCore/QTimer>
#include <QtCore/QHash>
#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>

#include <KDebug>


namespace {
    // small class to keep mem usage low
    class FolderEntry
    {
    public:
        FolderEntry() {
        }

        FolderEntry( int m )
            : mTime( m ) {
        }

        uint mTime;
        QHash<QString, FolderEntry> children;
    };
}

class FileSystemWatcher::Private
{
public:
    Private( FileSystemWatcher* parent )
        : recursive( true ),
          interval( 10*60 ),
          q( parent ) {
    }

    QStringList folders;
    QHash<QString, FolderEntry> cache;
    bool recursive;
    int interval;

    QTimer timer;

    void buildFolderCache( uint mTime );
    void checkFolders();

private:
    void updateChildrenCache( const QString& parentPath, FolderEntry& parentEntry, bool signalNewEntries );
    void checkFolder( const QString& path, FolderEntry& folder );

    FileSystemWatcher* q;
};


void FileSystemWatcher::Private::buildFolderCache( uint mTime )
{
    cache.clear();

    foreach( QString folder, folders ) {
        if ( folder.endsWith( '/' ) )
            folder.truncate( folder.length()-1 );
        FolderEntry entry( mTime );
        if ( recursive ) {
            updateChildrenCache( folder, entry, false );
        }
        cache.insert( folder, entry );
    }
}


void FileSystemWatcher::Private::updateChildrenCache( const QString& parentPath, FolderEntry& parentEntry, bool signalNewEntries )
{
    QDirIterator dirIt( parentPath, QDir::NoDotAndDotDot|QDir::Readable|QDir::Dirs|QDir::NoSymLinks );
    while ( dirIt.hasNext() ) {
        dirIt.next();
        if ( !parentEntry.children.contains( dirIt.fileName() ) ) {
            FolderEntry entry( parentEntry.mTime );
            parentEntry.children.insert( dirIt.fileName(), entry );
            if ( signalNewEntries ) {
                emit q->dirty( dirIt.filePath() );
            }
        }
    }

    for( QHash<QString, FolderEntry>::iterator it = parentEntry.children.begin();
         it != parentEntry.children.end(); ++it ) {
        updateChildrenCache( parentPath + '/' + it.key(), it.value(), signalNewEntries );
    }
}


void FileSystemWatcher::Private::checkFolders()
{
    for( QHash<QString, FolderEntry>::iterator it = cache.begin();
         it != cache.end(); ++it ) {
        checkFolder( it.key(), it.value() );
    }
}


void FileSystemWatcher::Private::checkFolder( const QString& path, FolderEntry& entry )
{
    QFileInfo info( path );
    if ( info.exists() ) {
        // check if anything changed in the folder
        bool dirty = false;
        if ( info.lastModified().toTime_t() > entry.mTime ) {
            entry.mTime = info.lastModified().toTime_t();
            emit q->dirty( path );
            dirty = true;
        }

        // check if any subfolder changed
        for( QHash<QString, FolderEntry>::iterator it = entry.children.begin();
             it != entry.children.end(); ++it ) {
            checkFolder( path + '/' + it.key(), it.value() );
        }

        // update in case folders have been created
        if ( dirty ) {
            updateChildrenCache( path, entry, true );
        }
    }
    // else -> FIXME: do we need to signal this or is it handled by the parent folder
}


FileSystemWatcher::FileSystemWatcher( QObject* parent )
    : QObject( parent ),
      d( new Private( this ) )
{
    connect( &d->timer, SIGNAL( timeout() ),
             this, SLOT( checkFolders() ) );
}


FileSystemWatcher::~FileSystemWatcher()
{
    delete d;
}


void FileSystemWatcher::start( const QDateTime& startTime )
{
    stop();
    d->buildFolderCache( startTime.toTime_t() );
    d->timer.start( d->interval*1000 );
}


void FileSystemWatcher::stop()
{
    d->timer.stop();
}


QStringList FileSystemWatcher::folders() const
{
    return d->folders;
}


bool FileSystemWatcher::watchRecursively() const
{
    return d->recursive;
}


int FileSystemWatcher::interval() const
{
    return d->interval;
}


void FileSystemWatcher::setFolders( const QStringList& folders )
{
    d->folders = folders;
}


void FileSystemWatcher::setWatchRecursively( bool r )
{
    d->recursive = r;
}


void FileSystemWatcher::setInterval( int seconds )
{
    d->interval = seconds;
}

#include "filesystemwatcher.moc"
