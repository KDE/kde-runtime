/* This file is part of the KDE libraries
   Copyright (C) 2007-2010 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kinotify.h"

#include <QtCore/QSocketNotifier>
#include <QtCore/QHash>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QQueue>

#include <kdebug.h>

#include <sys/inotify.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>


namespace {
    const int EVENT_STRUCT_SIZE = sizeof( struct inotify_event );

    // we need one event to fit into the buffer, the problem is that the name
    // is a variable length array
    const int EVENT_BUFFER_SIZE = EVENT_STRUCT_SIZE + 1024*16;

    QByteArray normalizePath( const QByteArray& path ) {
        QByteArray p( path );
        if ( p.endsWith( '/' ) )
            p.truncate( p.length()-1 );
        return p;
    }
}

class KInotify::Private
{
public:
    Private( KInotify* parent )
        : watchHiddenFolders( false ),
          m_inotifyFd( -1 ),
          m_notifier( 0 ),
          q( parent) {
    }

    ~Private() {
        close();
    }

    QHash<int, QString> cookies;
    QHash<int, QByteArray> pathHash;

    /// queue of paths to install watches for
    QQueue<QByteArray> pathsToWatch;

    unsigned char eventBuffer[EVENT_BUFFER_SIZE];

    // FIXME: only stored from the last addWatch call
    WatchEvents mode;
    WatchFlags flags;

    bool watchHiddenFolders;

    int inotify() {
        if ( m_inotifyFd < 0 ) {
            open();
        }
        return m_inotifyFd;
    }

    void close() {
        kDebug();
        delete m_notifier;
        m_notifier = 0;

        ::close( m_inotifyFd );
        m_inotifyFd = -1;
    }

    bool addWatch( const QByteArray& path ) {
        // we always need the unmount event to maintain our path hash
        WatchEvents newMode = mode;
        WatchFlags newFlags = flags;

        if( !q->filterWatch( path, newMode, newFlags ) ) {
            return true;
        }
        const int mask = newMode|newFlags|EventUnmount;

        int wd = inotify_add_watch( inotify(), path.data(), mask );
        if ( wd > 0 ) {
//            kDebug() << "Successfully added watch for" << path << pathHash.count();
            pathHash.insert( wd, normalizePath( path ) );
            return true;
        }
        else {
            kDebug() << "Failed to create watch for" << path;
            static bool userLimitReachedSignaled = false;
            if ( !userLimitReachedSignaled && errno == ENOSPC ) {
                kDebug() << "User limit reached. Please raise the inotify user watch limit.";
                userLimitReachedSignaled = true;
                emit q->watchUserLimitReached();
            }
            return false;
        }
    }

    bool addWatchesRecursively( const QByteArray& path )
    {
        if ( !addWatch( path ) )
            return false;

        int len = offsetof(struct dirent, d_name) +
                  pathconf(path.data(), _PC_NAME_MAX) + 1;
        struct dirent* entry = ( struct dirent* )new char[len];

        DIR* dir = opendir( path.data() );
        if ( dir ) {
            struct dirent *result = 0;
            while ( !readdir_r( dir, entry, &result ) ) {

                if ( !result ) {
                    // end of folder
                    break;
                }

                if ( ( entry->d_type == DT_UNKNOWN ||
                       entry->d_type == DT_DIR ) &&
                     ( watchHiddenFolders ||
                       qstrncmp( entry->d_name, ".", 1 ) ) &&
                     qstrcmp( entry->d_name, "." ) &&
                     qstrcmp( entry->d_name, ".." ) ) {
                    bool isDir = true;
                    QByteArray subDir = path + '/' + QByteArray::fromRawData( entry->d_name, qstrlen( entry->d_name ) );
                    if ( entry->d_type == DT_UNKNOWN ) {
                        struct stat buf;
                        lstat( subDir.data(), &buf );
                        isDir = S_ISDIR( buf.st_mode );
                    }

                    if ( isDir ) {
                        pathsToWatch.enqueue( subDir );
                    }
                }
            }

            closedir( dir );
            delete [] entry;

            return true;
        }
        else {
            kDebug() << "Could not open dir" << path;
            return false;
        }
    }

    void removeWatch( int wd ) {
        pathHash.remove( wd );
        inotify_rm_watch( inotify(), wd );
    }

    void _k_addWatches() {
        // add the next batch of paths
        for ( int i = 0; i < 100; ++i ) {
            if ( pathsToWatch.isEmpty() ||
                 !addWatchesRecursively( pathsToWatch.dequeue() ) ) {
                return;
            }
        }

        // asyncroneously add the next batch
        if ( !pathsToWatch.isEmpty() ) {
            QMetaObject::invokeMethod( q, "_k_addWatches", Qt::QueuedConnection );
        }
    }

private:
    void open() {
        kDebug();
        m_inotifyFd = inotify_init();
        delete m_notifier;
        if ( m_inotifyFd > 0 ) {
            fcntl( m_inotifyFd, F_SETFD, FD_CLOEXEC );
            kDebug() << "Successfully opened connection to inotify:" << m_inotifyFd;
            m_notifier = new QSocketNotifier( m_inotifyFd, QSocketNotifier::Read );
            connect( m_notifier, SIGNAL( activated( int ) ), q, SLOT( slotEvent( int ) ) );
        }
    }

    int m_inotifyFd;
    QSocketNotifier* m_notifier;

    KInotify* q;
};


KInotify::KInotify( QObject* parent )
    : QObject( parent ),
      d( new Private( this ) )
{
}


KInotify::~KInotify()
{
    delete d;
}


bool KInotify::available() const
{
    if( d->inotify() > 0 ) {
        // trueg: Copied from KDirWatch.
        struct utsname uts;
        int major, minor, patch;
        if ( uname(&uts) < 0 ) {
            return false; // *shrug*
        }
        else if ( sscanf( uts.release, "%d.%d.%d", &major, &minor, &patch) != 3 ) {
            return false; // *shrug*
        }
        else if( major * 1000000 + minor * 1000 + patch < 2006014 ) { // <2.6.14
            kDebug(7001) << "Can't use INotify, Linux kernel too old";
            return false;
        }

        return true;
    }
    else {
        return false;
    }
}


bool KInotify::watchingPath( const QString& path ) const
{
    QByteArray p( normalizePath( QFile::encodeName( path ) ) );
    QHash<int, QByteArray>::const_iterator end = d->pathHash.constEnd();
    for ( QHash<int, QByteArray>::const_iterator it = d->pathHash.constBegin();
          it != end; ++it ) {
        if ( it.value() == p )
            return true;
    }
    return false;
}


bool KInotify::addWatch( const QString& path, WatchEvents mode, WatchFlags flags )
{
    kDebug() << path;

    d->mode = mode;
    d->flags = flags;
    d->pathsToWatch.append( QFile::encodeName( path ) );
    d->_k_addWatches();
    return true;
}


// TODO: do this more efficiently
bool KInotify::removeWatch( const QString& path )
{
    QByteArray encodedPath = QFile::encodeName( path );
    QHash<int, QByteArray>::iterator it = d->pathHash.begin();
    while ( it != d->pathHash.end() ) {
        if ( it.value().startsWith( encodedPath ) ) {
            inotify_rm_watch( d->inotify(), it.key() );
            it = d->pathHash.erase( it );
        }
        else {
            ++it;
        }
    }
    return true;
}


bool KInotify::filterWatch( const QString & path, WatchEvents & modes, WatchFlags & flags )
{
    Q_UNUSED( path );
    Q_UNUSED( modes );
    Q_UNUSED( flags );
    return true;
}

void KInotify::slotEvent( int socket )
{
    // read at least one event
    int len = read( socket, d->eventBuffer, EVENT_BUFFER_SIZE );
    int i = 0;
    while ( i < len && len-i >= EVENT_STRUCT_SIZE  ) {
        struct inotify_event* event = ( struct inotify_event* )&d->eventBuffer[i];

        QByteArray encodedPath = QByteArray::fromRawData( event->name, event->len );

        if ( encodedPath[0] != '/' ) {
            encodedPath = d->pathHash.value( event->wd ) + '/' + encodedPath;
        }

        QString path = QFile::decodeName( encodedPath );

        // now signal the event
        if ( event->mask & EventAccess) {
//            kDebug() << path << "EventAccess";
            emit accessed( path );
        }
        if ( event->mask & EventAttributeChange ) {
//            kDebug() << path << "EventAttributeChange";
            emit attributeChanged( path );
        }
        if ( event->mask & EventCloseWrite ) {
//            kDebug() << path << "EventCloseWrite";
            emit closedWrite( path );
        }
        if ( event->mask & EventCloseRead ) {
//            kDebug() << path << "EventCloseRead";
            emit closedRead( path );
        }
        if ( event->mask & EventCreate ) {
//            kDebug() << path << "EventCreate";
            if ( event->mask & IN_ISDIR ) {
                // FIXME: store the mode and flags somewhere
                addWatch( encodedPath, d->mode, d->flags );
            }
            emit created( path );
        }
        if ( event->mask & EventDelete ) {
//            kDebug() << path << "EventDelete";
            bool isDir = false;
            if ( event->mask & IN_ISDIR ) {
                isDir = true;
                d->removeWatch( event->wd );
            }
            emit deleted( path, isDir );
        }
        if ( event->mask & EventDeleteSelf ) {
//            kDebug() << path << "EventDeleteSelf";
            bool isDir = false;
            if ( event->mask & IN_ISDIR ) {
                isDir = true;
                d->removeWatch( event->wd );
            }
            emit deleted( path, isDir );
        }
        if ( event->mask & EventModify ) {
//            kDebug() << path << "EventModify";
            emit modified( path );
        }
        if ( event->mask & EventMoveSelf ) {
//            kDebug() << path << "EventMoveSelf";
        }
        if ( event->mask & EventMoveFrom ) {
//            kDebug() << path << "EventMoveFrom";
            d->cookies[event->cookie] = path;
        }
        if ( event->mask & EventMoveTo ) {
            // check if we have a cookie for this one
            if ( d->cookies.contains( event->cookie ) ) {
                QString oldPath = d->cookies[event->cookie];
                d->cookies.remove( event->cookie );
//                kDebug() << oldPath << "EventMoveTo" << path;
                emit moved( oldPath, path );
            }
            else {
                kDebug() << "No cookie for move information of" << path;
            }
        }
        if ( event->mask & EventOpen ) {
//            kDebug() << path << "EventOpen";
            emit opened( path );
        }
        if ( event->mask & EventUnmount ) {
//            kDebug() << path << "EventUnmount. removing from path hash";
            if ( event->mask & IN_ISDIR ) {
                d->removeWatch( event->wd );
            }
            emit unmounted( path );
        }
        if ( event->mask & EventQueueOverflow ) {
            // This should not happen since we grab all events as soon as they arrive
            kDebug() << path << "EventQueueOverflow";
//            emit queueOverflow();
        }
        if ( event->mask & EventIgnored ) {
            kDebug() << path << "EventIgnored";
        }

        i += EVENT_STRUCT_SIZE + event->len;
    }

    if ( len < 0 ) {
        kDebug() << "Failed to read event.";
    }
}

#include "kinotify.moc"
