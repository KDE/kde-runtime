/* This file is part of the KDE Project
   Copyright (c) 2008-2010 Sebastian Trueg <trueg@kde.org>
   Copyright (c) 2010 Vishesh Handa <handa.vish@gmail.com>

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
#include "strigiadaptor.h"
#include "indexscheduler.h"
#include "eventmonitor.h"
#include "strigiserviceconfig.h"
#include "filewatchserviceinterface.h"
#include "util.h"

#include <KDebug>
#include <KDirNotify>
#include <KIdleTime>
#include <KLocale>

#include <nepomuk/resourcemanager.h>

#include <QtCore/QTimer>


Nepomuk::StrigiService::StrigiService( QObject* parent, const QList<QVariant>& )
    : Service( parent )
{
    // setup the actual index scheduler including strigi stuff
    // ==============================================================
    m_indexScheduler = new IndexScheduler( this );

    // monitor all kinds of events
    ( void )new EventMonitor( m_indexScheduler, this );

    // update the watches if the config changes
    connect( StrigiServiceConfig::self(), SIGNAL( configChanged() ),
             this, SLOT( updateWatches() ) );

    // export on dbus
    ( void )new StrigiAdaptor( this );

    // setup status connections
    connect( m_indexScheduler, SIGNAL( indexingStarted() ),
             this, SIGNAL( statusStringChanged() ) );
    connect( m_indexScheduler, SIGNAL( indexingStopped() ),
             this, SIGNAL( statusStringChanged() ) );
    connect( m_indexScheduler, SIGNAL( indexingFolder(QString) ),
             this, SIGNAL( statusStringChanged() ) );
    connect( m_indexScheduler, SIGNAL( indexingFile(QString) ),
             this, SIGNAL( statusStringChanged() ) );
    connect( m_indexScheduler, SIGNAL( indexingSuspended(bool) ),
             this, SIGNAL( statusStringChanged() ) );

    // setup the indexer to index at snail speed for the first two minutes
    // this is done for KDE startup - to not slow that down too much
    m_indexScheduler->setIndexingSpeed( IndexScheduler::SnailPace );

    // delayed init for the rest which uses IO and CPU
    // FIXME: do not use a random delay value but wait for KDE to be started completely (using the session manager)
    QTimer::singleShot( 2*60*1000, this, SLOT( finishInitialization() ) );

    // start the actual indexing
    m_indexScheduler->start();

    // Connect some signals used in the DBus interface
    connect( this, SIGNAL( statusStringChanged() ),
             this, SIGNAL( statusChanged() ) );
    connect( m_indexScheduler, SIGNAL( indexingStarted() ),
             this, SIGNAL( indexingStarted() ) );
    connect( m_indexScheduler, SIGNAL( indexingStopped() ),
             this, SIGNAL( indexingStopped() ) );
    connect( m_indexScheduler, SIGNAL( indexingFolder(QString) ),
             this, SIGNAL( indexingFolder(QString) ) );
}


Nepomuk::StrigiService::~StrigiService()
{
    m_indexScheduler->stop();
    m_indexScheduler->wait();
}


void Nepomuk::StrigiService::finishInitialization()
{
    // slow down on user activity (start also only after 2 minutes)
    KIdleTime* idleTime = KIdleTime::instance();
    idleTime->addIdleTimeout( 1000 * 60 * 2 ); // 2 min

    connect( idleTime, SIGNAL(timeoutReached(int)), this, SLOT(slotIdleTimeoutReached()) );
    connect( idleTime, SIGNAL(resumingFromIdle()), this, SLOT(slotIdleTimerResume()) );

    // start out with reduced speed until the user is idle for 2 min
    m_indexScheduler->setIndexingSpeed( IndexScheduler::ReducedSpeed );

    // Creation of watches is a memory intensive process as a large number of
    // watch file descriptors need to be created ( one for each directory )
    updateWatches();
}

void Nepomuk::StrigiService::slotIdleTimeoutReached()
{
    m_indexScheduler->setIndexingSpeed( IndexScheduler::FullSpeed );
    KIdleTime::instance()->catchNextResumeEvent();
}

void Nepomuk::StrigiService::slotIdleTimerResume()
{
    m_indexScheduler->setIndexingSpeed( IndexScheduler::ReducedSpeed );
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

    if ( suspended ) {
        return i18nc( "@info:status", "File indexer is suspended" );
    }
    else if ( indexing ) {
        QString folder = m_indexScheduler->currentFolder();

        if ( folder.isEmpty() || simple )
            return i18nc( "@info:status", "Strigi is currently indexing files" );
        else {
            QString file = KUrl( m_indexScheduler->currentFile() ).fileName();
            if( file.isEmpty() )
                return i18nc( "@info:status", "Strigi is currently indexing files in folder %1", folder );
            else
                return i18nc( "@info:status", "Strigi is currently indexing files in folder %1 (%2)", folder, file );
        }
    }
    else {
        return i18nc( "@info:status", "File indexer is idle" );
    }
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


bool Nepomuk::StrigiService::isIndexing() const
{
    return m_indexScheduler->isIndexing();
}


void Nepomuk::StrigiService::suspend() const
{
    m_indexScheduler->suspend();
}


void Nepomuk::StrigiService::resume() const
{
    m_indexScheduler->resume();
}


QString Nepomuk::StrigiService::currentFile() const
{
   return m_indexScheduler->currentFile();
}


QString Nepomuk::StrigiService::currentFolder() const
{
    return m_indexScheduler->currentFolder();
}


void Nepomuk::StrigiService::updateFolder(const QString& path, bool recursive, bool forced)
{
    kDebug() << "Called with path: " << path;
    QFileInfo info( path );
    if ( info.exists() ) {
        QString dirPath;
        if ( info.isDir() )
            dirPath = info.absoluteFilePath();
        else
            dirPath = info.absolutePath();

        if ( StrigiServiceConfig::self()->shouldFolderBeIndexed( dirPath ) ) {
            indexFolder(path, recursive, forced);
        }
    }
}


void Nepomuk::StrigiService::updateAllFolders(bool forced)
{
    m_indexScheduler->updateAll( forced );
}


void Nepomuk::StrigiService::indexFile(const QString& path)
{
    m_indexScheduler->analyzeFile( path );
}


void Nepomuk::StrigiService::indexFolder(const QString& path, bool recursive, bool forced)
{
    QFileInfo info( path );
    if ( info.exists() ) {
        QString dirPath;
        if ( info.isDir() )
            dirPath = info.absoluteFilePath();
        else
            dirPath = info.absolutePath();

        kDebug() << "Updating : " << dirPath;

        Nepomuk::IndexScheduler::UpdateDirFlags flags;
        if(recursive)
            flags |= Nepomuk::IndexScheduler::UpdateRecursive;
        if(forced)
            flags |= Nepomuk::IndexScheduler::ForceUpdate;

        m_indexScheduler->updateDir( dirPath, flags );
    }
}



#include <kpluginfactory.h>
#include <kpluginloader.h>

NEPOMUK_EXPORT_SERVICE( Nepomuk::StrigiService, "nepomukstrigiservice" )

#include "strigiservice.moc"

