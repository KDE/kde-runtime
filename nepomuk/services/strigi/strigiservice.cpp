/* This file is part of the KDE Project
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

#include "strigiservice.h"
#include "strigiserviceadaptor.h"
#include "priority.h"
#include "indexscheduler.h"
#include "eventmonitor.h"
#include "systray.h"
#include "strigiserviceconfig.h"
#include "statuswidget.h"
#include "filesystemwatcher.h"

#include <KDebug>
#include <KDirNotify>

#include <strigi/indexpluginloader.h>
#include <strigi/indexmanager.h>

#include <nepomuk/resourcemanager.h>


Nepomuk::StrigiService::StrigiService( QObject* parent, const QList<QVariant>& )
    : Service( parent, true ),
      m_indexManager( 0 ),
      m_indexScheduler( 0 ),
      m_fsWatcher( 0 ),
      m_wakeUpTimer( 0 ),
      m_cursorPos(),
      m_suspendRequested( false ),
      m_internallySuspended( false )
{
    // only so ResourceManager won't open yet another connection to the nepomuk server
    ResourceManager::instance()->setOverrideMainModel( mainModel() );


    // Lower process priority - we do not want to spoil KDE usage
    // ==============================================================
    if ( !lowerPriority() )
        kDebug() << "Failed to lower priority.";
    if ( !lowerSchedulingPriority() )
        kDebug() << "Failed to lower scheduling priority.";
    if ( !lowerIOPriority() )
        kDebug() << "Failed to lower io priority.";


    // Start indexing with a delay of 2 minutes to let KDE startup
    // in peace. TODO: how about only doing this when we are actually
    // in KDE startup
    // ==============================================================
    if ( ( m_indexManager = Strigi::IndexPluginLoader::createIndexManager( "nepomukbackend", 0 ) ) ) {
        // setup the actual index scheduler including strigi stuff
        m_indexScheduler = new IndexScheduler( m_indexManager, this );

        // setup status connections
        connect( m_indexScheduler, SIGNAL( indexingStarted() ),
                 this, SIGNAL( statusStringChanged() ) );
        connect( m_indexScheduler, SIGNAL( indexingStopped() ),
                 this, SIGNAL( statusStringChanged() ) );
        connect( m_indexScheduler, SIGNAL( indexingFolder(QString) ),
                 this, SIGNAL( statusStringChanged() ) );
        connect( m_indexScheduler, SIGNAL( indexingSuspended(bool) ),
                 this, SIGNAL( statusStringChanged() ) );

        // export on dbus
        ( void )new StrigiServiceAdaptor( this );

        // create the status widget (hidden)
        StatusWidget* sw = new StatusWidget( mainModel(), this );

        // create the systray
        new SystemTray( this, sw );

        // delayed start of initial indexing
        QTimer::singleShot( 2*60*1000, this, SLOT( delayedInit() ) );
    }
    else {
        kDebug() << "Failed to load nepomukbackend Strigi index manager.";
    }

    // service initialization done if creating a strigi index manager was successful
    // ==============================================================
    setServiceInitialized( m_indexManager != 0 );
}


void Nepomuk::StrigiService::delayedInit()
{
    // monitor all kinds of events
    ( void )new EventMonitor( m_indexScheduler, this );

    // monitor the file system
    m_fsWatcher = new FileSystemWatcher( this );
    m_fsWatcher->setWatchRecursively( true );
    connect( m_fsWatcher, SIGNAL( dirty( QString ) ),
             this, SLOT( slotDirDirty( QString ) ) );
    connect( m_fsWatcher, SIGNAL( statusChanged(FileSystemWatcher::Status) ),
             this, SIGNAL( statusStringChanged() ) );

    // monitor all KDE-ish changes for quick updates
    connect( new org::kde::KDirNotify( QString(), QString(), QDBusConnection::sessionBus(), this ),
             SIGNAL( FilesAdded( QString ) ),
             this, SLOT( slotDirDirty( const QString& ) ) );

    // update the watches if the config changes
    connect( StrigiServiceConfig::self(), SIGNAL( configChanged() ),
             this, SLOT( updateWatches() ) );

    // start watching the index folders
    updateWatches();

    // start the actual indexing
    m_indexScheduler->start();

    // in case suspend was called before we were initialized
    if ( m_suspendRequested ) {
        m_indexScheduler->suspend();
    }

    // start checking for user activity
    m_wakeUpTimer = new QTimer( this );
    m_wakeUpTimer->setSingleShot( true );
    m_wakeUpTimer->setInterval( 10*1000 );
    connect( m_wakeUpTimer, SIGNAL( timeout() ), SLOT( wakeUp() ) );
    m_wakeUpTimer->start();

    m_cursorPos = QCursor::pos();
}


Nepomuk::StrigiService::~StrigiService()
{
    if ( m_indexManager ) {
        m_indexScheduler->stop();
        m_indexScheduler->wait();
        Strigi::IndexPluginLoader::deleteIndexManager( m_indexManager );
    }
}

bool Nepomuk::StrigiService::isSuspended() const
{
    return m_indexScheduler && m_indexScheduler->isSuspended() && !m_internallySuspended;
}

void Nepomuk::StrigiService::updateWatches()
{
    // the hard way since the KDirWatch API is too simple
    QStringList folders = StrigiServiceConfig::self()->folders();
    if ( folders != m_fsWatcher->folders() ) {
        m_fsWatcher->setFolders( StrigiServiceConfig::self()->folders() );
        m_fsWatcher->setInterval( 2*60 ); // check every 2 minutes
        m_fsWatcher->start();
    }
}


void Nepomuk::StrigiService::slotDirDirty( const QString& path )
{
    if ( StrigiServiceConfig::self()->shouldFolderBeIndexed( path ) ) {
        m_indexScheduler->updateDir( path );
    }
}


void Nepomuk::StrigiService::wakeUp()
{
    if ( m_suspendRequested ) {
        m_wakeUpTimer->stop();
    }
    else {
        Q_ASSERT( m_indexScheduler );

        const QPoint cursorPos = QCursor::pos();
        if ( QPoint( cursorPos - m_cursorPos ).manhattanLength() > 10 ) {
            // The mouse has been moved. Suspend the indexing until
            // no mouse moving has been done for the given time period.
            m_cursorPos = cursorPos;

            m_internallySuspended = true;
            m_indexScheduler->suspend();
            if ( m_fsWatcher )
                m_fsWatcher->suspend();

            m_wakeUpTimer->setInterval( 2*60*1000 );
        }
        else {
            // the mouse has not been moved
            m_internallySuspended = false;
            m_indexScheduler->resume();
            if ( m_fsWatcher )
                m_fsWatcher->resume();

            // use a shorter interval during indexing to allow a fast
            // stopping after a user interaction
            m_wakeUpTimer->setInterval( 10*1000 );
        }
        m_wakeUpTimer->start();
    }
}


QString Nepomuk::StrigiService::userStatusString() const
{
    if ( m_indexScheduler && !m_internallySuspended ) {
        bool indexing = m_indexScheduler->isIndexing();
        bool suspended = m_indexScheduler->isSuspended();
        QString folder = m_indexScheduler->currentFolder();

        if ( suspended )
            return i18nc( "@info:status", "File indexer is suspended" );
        else if ( indexing )
            return i18nc( "@info:status", "Strigi is currently indexing files in folder %1", folder );
        else if ( m_fsWatcher && m_fsWatcher->status() == FileSystemWatcher::Checking )
            return i18nc( "@info:status", "Checking file system for new files" );
    }
    else if ( m_suspendRequested ) {
        return i18nc( "@info:status", "File indexer is suspended" );
    }

    return i18nc( "@info:status", "File indexer is idle" );
}


void Nepomuk::StrigiService::setSuspended( bool suspend )
{
    m_suspendRequested = suspend;
    m_internallySuspended = false;

    if ( m_indexScheduler ) {
        if ( suspend ) {
            m_indexScheduler->suspend();
            if ( m_fsWatcher )
                m_fsWatcher->suspend();
        }
        else {
            m_indexScheduler->resume();
            if ( m_fsWatcher )
                m_fsWatcher->resume();
        }

        Q_ASSERT( m_wakeUpTimer );
        if ( suspend )
            m_wakeUpTimer->stop();
        else
            m_wakeUpTimer->start();
    }
}


#include <kpluginfactory.h>
#include <kpluginloader.h>

NEPOMUK_EXPORT_SERVICE( Nepomuk::StrigiService, "nepomukstrigiservice" )

#include "strigiservice.moc"

