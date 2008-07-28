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

#include "eventmonitor.h"
#include "config.h"
#include "indexscheduler.h"

#include <KDirWatch>
#include <KDebug>
#include <KPassivePopup>
#include <KLocale>
#include <KDiskFreeSpaceInfo>
#include <KStandardDirs>
#include <KNotification>
#include <KIcon>

#include <Solid/PowerManagement>

#include <QtDBus/QDBusInterface>


namespace {
    void sendEvent( const QString& event, const QString& text, const QString& iconName ) {
        KNotification::event( event, text, KIcon( iconName ).pixmap( 32, 32 ) );
    }
}


Nepomuk::EventMonitor::EventMonitor( IndexScheduler* scheduler, QObject* parent )
    : QObject( parent ),
      m_indexScheduler( scheduler ),
      m_pauseState( NotPaused ),
      m_watchedRecursively( false )
{
    // setup the dirwatch
    m_dirWatch = new KDirWatch( this );
    connect( m_dirWatch, SIGNAL( dirty( QString ) ),
             m_indexScheduler, SLOT( updateDir( QString ) ) );
    connect( m_dirWatch, SIGNAL( deleted( QString ) ),
             m_indexScheduler, SLOT( updateDir( QString ) ) );

    // update the watches if the config changes
    connect( Config::self(), SIGNAL( configChanged() ),
             this, SLOT( updateWatches() ) );

    // monitor the powermanagement to not drain the battery
    connect( Solid::PowerManagement::notifier(), SIGNAL( appShouldConserveResourcesChanged( bool ) ),
             this, SLOT( slotPowerManagementStatusChanged( bool ) ) );

    // start watching the index folders
    updateWatches();

    // setup the avail disk usage monitor
    connect( &m_availSpaceTimer, SIGNAL( timeout() ),
             this, SLOT( slotCheckAvailableSpace() ) );
    m_availSpaceTimer.start( 20*1000 ); // every 20 seconds should be enough

    if ( Config::self()->isInitialRun() ) {
        // TODO: add actions to this notification

        m_initialIndexTime.start();

        // inform the user about the initial indexing
        sendEvent( "initialIndexingStarted",
                   i18n( "Strigi file indexing started. Indexing all files for fast desktop searches may take a while." ),
                   "nepomuk" );

        // connect to get the end of initial indexing
        connect( m_indexScheduler, SIGNAL( indexingStopped() ),
                 this, SLOT( slotIndexingStopped() ) );
    }
}


Nepomuk::EventMonitor::~EventMonitor()
{
}


void Nepomuk::EventMonitor::updateWatches()
{
    // the hard way since the KDirWatch API is too simple
    QStringList folders = Config::self()->folders();
    if ( folders != m_watchedFolders ||
         Config::self()->recursive() != m_watchedRecursively ) {
        foreach( const QString& folder, m_watchedFolders ) {
            m_dirWatch->removeDir( folder );
        }
        m_watchedFolders = folders;
        m_watchedRecursively = Config::self()->recursive();
        foreach( const QString& folder, m_watchedFolders ) {
            m_dirWatch->addDir( folder, m_watchedRecursively ? KDirWatch::WatchSubDirs : KDirWatch::WatchDirOnly );
        }
    }
}


void Nepomuk::EventMonitor::slotPowerManagementStatusChanged( bool conserveResources )
{
    if ( !conserveResources && m_pauseState == PausedDueToPowerManagement ) {
        kDebug() << "Resuming indexer due to power management";
        m_pauseState = NotPaused;
        m_indexScheduler->resume();
        sendEvent( "indexingResumed", i18n("Resuming Strigi file indexing."), "solid" );
    }
    else if ( m_indexScheduler->isRunning() &&
              !m_indexScheduler->isSuspended() ) {
        kDebug() << "Pausing indexer due to power management";
        m_pauseState = PausedDueToPowerManagement;
        m_indexScheduler->suspend();
        sendEvent( "indexingSuspended", i18n("Suspending Strigi file indexing to preserve resources."), "solid" );
    }
}


void Nepomuk::EventMonitor::slotCheckAvailableSpace()
{
    KDiskFreeSpaceInfo info = KDiskFreeSpaceInfo::freeSpaceInfo( KStandardDirs::locateLocal( "data", "nepomuk/repository/", false ) );
    if ( info.isValid() ) {
        if ( info.available() <= Config::self()->minDiskSpace() ) {
            if ( m_indexScheduler->isRunning() &&
                !m_indexScheduler->isSuspended() ) {
                m_pauseState = PausedDueToAvailSpace;
                m_indexScheduler->suspend();
                sendEvent( "indexingSuspended",
                           i18n("Local disk space is running low (%1 left). Suspending Strigi file indexing.",
                                KIO::convertSize( info.available() ) ),
                           "drive-harddisk" );
            }
        }
        else if ( m_pauseState == PausedDueToAvailSpace ) {
            kDebug() << "Resuming indexer due to disk space";
            m_pauseState = NotPaused;
            m_indexScheduler->resume();
            sendEvent( "indexingResumed", i18n("Resuming Strigi file indexing."), "drive-harddisk" );
        }
    }
    else {
        // if it does not work once, it will probably never work
        m_availSpaceTimer.stop();
    }
}


void Nepomuk::EventMonitor::slotIndexingStopped()
{
    // inform the user about the end of initial indexing. This will only be called once
    if ( !m_indexScheduler->isSuspended() ) {
        kDebug() << "initial indexing took" << m_initialIndexTime.elapsed();
        sendEvent( "initialIndexingFinished", i18n( "Strigi file indexing finished. Elapsed time: %1", m_initialIndexTime.toString() ), "nepomuk" );
        m_indexScheduler->disconnect( this );

        // after this much index work, it makes sense to optimize the full text index in the main model
        QDBusInterface( "org.kde.nepomuk.services.nepomukstorage", "/nepomukstorage", "org.kde.nepomuk.Storage" ).call( "optimize", "main" );
    }
}

#include "eventmonitor.moc"
