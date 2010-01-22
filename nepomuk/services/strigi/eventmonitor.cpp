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
#include "strigiserviceconfig.h"
#include "indexscheduler.h"

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
      m_pauseState( NotPaused )
{
    // FileSystemWatcher does not catch changes to files, only new and removed files
    // thus, we also do periodic updates of the whole index every two hours
    connect( &m_periodicUpdateTimer, SIGNAL( timeout() ),
             m_indexScheduler, SLOT( updateAll() ) );
    m_periodicUpdateTimer.setInterval( 2*60*60*1000 );

    // monitor the powermanagement to not drain the battery
    connect( Solid::PowerManagement::notifier(), SIGNAL( appShouldConserveResourcesChanged( bool ) ),
             this, SLOT( slotPowerManagementStatusChanged( bool ) ) );

    // setup the avail disk usage monitor
    connect( &m_availSpaceTimer, SIGNAL( timeout() ),
             this, SLOT( slotCheckAvailableSpace() ) );
    m_availSpaceTimer.start( 20*1000 ); // every 20 seconds should be enough

    if ( StrigiServiceConfig::self()->isInitialRun() ) {
        // TODO: add actions to this notification

        m_initialIndexTime.start();

        // inform the user about the initial indexing
        sendEvent( "initialIndexingStarted",
                   i18n( "Indexing files for fast searching. This process may take a while." ),
                   "nepomuk" );

        // connect to get the end of initial indexing
        // (make it a queued connection since KNotification does not like multithreading
        // (mostly because it uses dialogs)
        connect( m_indexScheduler, SIGNAL( indexingStopped() ),
                 this, SLOT( slotIndexingStopped() ),
                 Qt::QueuedConnection );
    }
    else {
        m_periodicUpdateTimer.start();
    }

    slotPowerManagementStatusChanged( Solid::PowerManagement::appShouldConserveResources() );
}


Nepomuk::EventMonitor::~EventMonitor()
{
}


void Nepomuk::EventMonitor::slotPowerManagementStatusChanged( bool conserveResources )
{
    if ( !conserveResources && m_pauseState == PausedDueToPowerManagement ) {
        kDebug() << "Resuming indexer due to power management";
        m_pauseState = NotPaused;
        m_indexScheduler->resume();
        sendEvent( "indexingResumed", i18n("Resuming indexing of files for fast searching."), "battery-charging" );
    }
    else if ( conserveResources &&
              m_indexScheduler->isRunning() &&
              !m_indexScheduler->isSuspended() ) {
        kDebug() << "Pausing indexer due to power management";
        m_pauseState = PausedDueToPowerManagement;
        m_indexScheduler->suspend();
        sendEvent( "indexingSuspended", i18n("Suspending the indexing of files to preserve resources."), "battery-100" );
    }
}


void Nepomuk::EventMonitor::slotCheckAvailableSpace()
{
    KDiskFreeSpaceInfo info = KDiskFreeSpaceInfo::freeSpaceInfo( KStandardDirs::locateLocal( "data", "nepomuk/repository/", false ) );
    if ( info.isValid() ) {
        if ( info.available() <= StrigiServiceConfig::self()->minDiskSpace() ) {
            if ( m_indexScheduler->isRunning() &&
                !m_indexScheduler->isSuspended() ) {
                m_pauseState = PausedDueToAvailSpace;
                m_indexScheduler->suspend();
                sendEvent( "indexingSuspended",
                           i18n("Disk space is running low (%1 left). Suspending indexing of files.",
                                KIO::convertSize( info.available() ) ),
                           "drive-harddisk" );
            }
        }
        else if ( m_pauseState == PausedDueToAvailSpace ) {
            kDebug() << "Resuming indexer due to disk space";
            m_pauseState = NotPaused;
            m_indexScheduler->resume();
            sendEvent( "indexingResumed", i18n("Resuming indexing of files for fast searching."), "drive-harddisk" );
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
        sendEvent( "initialIndexingFinished",
                   i18nc( "@info %1 is a duration formatted using KLocale::prettyFormatDuration",
                          "Initial indexing of files for fast searching finished in %1",
                          KGlobal::locale()->prettyFormatDuration( m_initialIndexTime.elapsed() ) ),
                   "nepomuk" );
        m_indexScheduler->disconnect( this );

        // after this much index work, it makes sense to optimize the full text index in the main model
        QDBusInterface( "org.kde.nepomuk.services.nepomukstorage", "/nepomukstorage", "org.kde.nepomuk.Storage" ).call( "optimize", "main" );


        m_periodicUpdateTimer.start();
    }
}

#include "eventmonitor.moc"
