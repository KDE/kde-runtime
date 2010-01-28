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
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QMetaType>

#include <KDebug>

Q_DECLARE_METATYPE( FileSystemWatcher::Status )

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

class FileSystemWatcher::Private : public QThread
{
public:
    Private( FileSystemWatcher* parent )
        : recursive( true ),
          interval( 10*60 ),
          status( FileSystemWatcher::Idle ),
          m_suspended( false ),
          q( parent ) {
    }

    QStringList folders;
    QHash<QString, FolderEntry> cache;
    bool recursive;
    int interval;

    FileSystemWatcher::Status status;

    void start( const QDateTime& startTime );
    void stop();
    void suspend( bool );
    void run();

    void buildFolderCache( uint mTime );
    void checkFolders();

private:
    void updateChildrenCache( const QString& parentPath, FolderEntry& parentEntry, bool signalNewEntries );
    void checkFolder( const QString& path, FolderEntry& folder );

    // waits if suspended, returns false if stopped
    bool continueChecking();

    QDateTime m_startTime;
    QWaitCondition m_updateWaiter;
    QMutex m_statusMutex;
    QWaitCondition m_statusWaiter;
    bool m_stopped;
    bool m_suspended;

    FileSystemWatcher* q;
};


void FileSystemWatcher::Private::start( const QDateTime& startTime )
{
    m_stopped = false;
    m_startTime = startTime;
    QThread::start();
}


void FileSystemWatcher::Private::stop()
{
    QMutexLocker lock( &m_statusMutex );
    m_stopped = true;
    m_updateWaiter.wakeAll();
    m_statusWaiter.wakeAll();
}


void FileSystemWatcher::Private::suspend( bool suspend )
{
    if ( suspend != m_suspended ) {
        kDebug() << suspend;
        QMutexLocker lock( &m_statusMutex );
        m_suspended = suspend;
        if ( !suspend )
            m_statusWaiter.wakeAll();
    }
}


bool FileSystemWatcher::Private::continueChecking()
{
    QMutexLocker lock( &m_statusMutex );
    if ( m_suspended && !m_stopped ) {
        kDebug() << "waiting";
        m_statusWaiter.wait( &m_statusMutex );
    }
    return !m_stopped;
}


void FileSystemWatcher::Private::run()
{
    buildFolderCache( m_startTime.toTime_t() );

    while ( 1 ) {
        // wait for the next update or stop
        QMutex mutex;
        mutex.lock();
        if ( m_updateWaiter.wait( &mutex, interval*1000 ) ) {
            // canceled
            return;
        }

        kDebug() << "woke up";

        // check if we have been stopped
        if ( !continueChecking() )
            return;

        // check all folders
        status = Checking;
        emit q->statusChanged( Checking );
        checkFolders();
        status = Idle;
        emit q->statusChanged( Idle );

        // check if we have been stopped
        if ( !continueChecking() )
            return;
    }
}


void FileSystemWatcher::Private::buildFolderCache( uint mTime )
{
    cache.clear();

    foreach( QString folder, folders ) { // krazy:exclude=foreach
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
    kDebug();
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

            // wait in case we are suspended
            if ( !continueChecking() )
                return;
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
    qRegisterMetaType<FileSystemWatcher::Status>();
}


FileSystemWatcher::~FileSystemWatcher()
{
    stop();
    delete d;
}


void FileSystemWatcher::start( const QDateTime& startTime )
{
    stop();
    d->start( startTime );
}


void FileSystemWatcher::stop()
{
    d->stop();
    d->wait();
}


void FileSystemWatcher::suspend()
{
    d->suspend( true );
}


void FileSystemWatcher::resume()
{
    d->suspend( false );
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


FileSystemWatcher::Status FileSystemWatcher::status() const
{
    return d->status;
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
