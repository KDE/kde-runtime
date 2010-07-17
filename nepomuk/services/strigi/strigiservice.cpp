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
#include "filewatchserviceinterface.h"

#include <KDebug>
#include <KDirNotify>
#include <KIdleTime>

#include <strigi/indexpluginloader.h>
#include <strigi/indexmanager.h>

#include <nepomuk/resourcemanager.h>

#include <QtCore/QTimer>


Nepomuk::StrigiService::StrigiService( QObject* parent, const QList<QVariant>& )
    : Service( parent, true ),
      m_indexManager( 0 )
{
    // lower process priority - we do not want to spoil KDE usage
    // ==============================================================
    if ( !lowerPriority() )
        kDebug() << "Failed to lower priority.";
    if ( !lowerSchedulingPriority() )
        kDebug() << "Failed to lower scheduling priority.";
    if ( !lowerIOPriority() )
        kDebug() << "Failed to lower io priority.";


    // setup the actual index scheduler including strigi stuff
    // ==============================================================
    if ( ( m_indexManager = Strigi::IndexPluginLoader::createIndexManager( "nepomukbackend", 0 ) ) ) {
        m_indexScheduler = new IndexScheduler( m_indexManager, this );

        // monitor all kinds of events
        ( void )new EventMonitor( m_indexScheduler, this );

        // update the watches if the config changes
        connect( StrigiServiceConfig::self(), SIGNAL( configChanged() ),
                 this, SLOT( updateWatches() ) );

        // export on dbus
        ( void )new StrigiServiceAdaptor( this );

        // create the status widget (hidden)
        StatusWidget* sw = new StatusWidget( mainModel(), this );

        // create the systray
        new SystemTray( this, sw );

        // setup status connections
        connect( m_indexScheduler, SIGNAL( indexingStarted() ),
                 this, SIGNAL( statusStringChanged() ) );
        connect( m_indexScheduler, SIGNAL( indexingStopped() ),
                 this, SIGNAL( statusStringChanged() ) );
        connect( m_indexScheduler, SIGNAL( indexingFolder(QString) ),
                 this, SIGNAL( statusStringChanged() ) );
        connect( m_indexScheduler, SIGNAL( indexingSuspended(bool) ),
                 this, SIGNAL( statusStringChanged() ) );

        // setup the indexer to index at snail speed for the first two minutes
        // this is done for KDE startup - to not slow that down too much
        m_indexScheduler->setIndexingSpeed( IndexScheduler::SnailPace );

        // delayed init for the rest which uses IO and CPU
        QTimer::singleShot( 2*60*1000, this, SLOT( finishInitialization() ) );

        // start the actual indexing
        m_indexScheduler->start();
    }
    else {
        kDebug() << "Failed to load sopranobackend Strigi index manager.";
    }


    // service initialization done if creating a strigi index manager was successful
    // ==============================================================
    setServiceInitialized( m_indexManager != 0 );
}


Nepomuk::StrigiService::~StrigiService()
{
    if ( m_indexManager ) {
        m_indexScheduler->stop();
        m_indexScheduler->wait();
        Strigi::IndexPluginLoader::deleteIndexManager( m_indexManager );
    }
}


void Nepomuk::StrigiService::finishInitialization()
{
    // slow down on user activity (start also only after 2 minutes)
    KIdleTime * idleTime = KIdleTime::instance();
    idleTime->addIdleTimeout( 1000 * 60 * 2 ); // 2 min

    connect( idleTime, SIGNAL(timeoutReached(int)), this, SLOT(slotIdleTimerPause()) );
    connect( idleTime, SIGNAL(resumingFromIdle()), this, SLOT(slotIdleTimerResume()) );

    // full speed until the user is active
    m_indexScheduler->setIndexingSpeed( IndexScheduler::FullSpeed );

    updateWatches();
}

void Nepomuk::StrigiService::slotIdleTimerPause()
{
    m_indexScheduler->setIndexingSpeed( IndexScheduler::ReducedSpeed );
}

void Nepomuk::StrigiService::slotIdleTimerResume()
{
    m_indexScheduler->setIndexingSpeed( IndexScheduler::FullSpeed );
}


void Nepomuk::StrigiService::updateWatches()
{
    org::kde::nepomuk::FileWatch filewatch( "org.kde.nepomuk.services.nepomukfilewatch",
                                            "/nepomukfilewatch",
                                            QDBusConnection::sessionBus() );
    foreach( const QString& folder, StrigiServiceConfig::self()->includeFolders() ) {
        filewatch.watchFolder( folder );
    }
}


QString Nepomuk::StrigiService::userStatusString() const
{
    return userStatusString( false );
}


QString Nepomuk::StrigiService::simpleUserStatusString() const
{
    return userStatusString( true );
}


QString Nepomuk::StrigiService::userStatusString( bool simple ) const
{
    bool indexing = m_indexScheduler->isIndexing();
    bool suspended = m_indexScheduler->isSuspended();
    QString folder = m_indexScheduler->currentFolder();

    if ( suspended ) {
        return i18nc( "@info:status", "File indexer is suspended" );
    }
    else if ( indexing ) {
        if ( folder.isEmpty() || simple )
            return i18nc( "@info:status", "Strigi is currently indexing files" );
        else
            return i18nc( "@info:status", "Strigi is currently indexing files in folder %1", folder );
    }
    else {
        return i18nc( "@info:status", "File indexer is idle" );
    }
}


bool Nepomuk::StrigiService::isIdle() const
{
    return ( !m_indexScheduler->isIndexing() );
}


void Nepomuk::StrigiService::setSuspended( bool suspend )
{
    if ( suspend ) {
        m_indexScheduler->suspend();
    }
    else {
        m_indexScheduler->resume();
    }
}


bool Nepomuk::StrigiService::isSuspended() const
{
    return m_indexScheduler->isSuspended();
}

#include <kpluginfactory.h>
#include <kpluginloader.h>

NEPOMUK_EXPORT_SERVICE( Nepomuk::StrigiService, "nepomukstrigiservice" )

#include "strigiservice.moc"

