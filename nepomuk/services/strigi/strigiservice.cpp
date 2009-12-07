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

#include "strigiservice.h"
#include "strigiserviceadaptor.h"
#include "priority.h"
#include "indexscheduler.h"
#include "eventmonitor.h"
#include "systray.h"
#include "strigiserviceconfig.h"
#include "statuswidget.h"
#include "nepomukstorageinterface.h"
#include "filesystemwatcher.h"

#include <KDebug>

#include <strigi/indexpluginloader.h>
#include <strigi/indexmanager.h>

#include <nepomuk/resourcemanager.h>


Nepomuk::StrigiService::StrigiService( QObject* parent, const QList<QVariant>& )
    : Service( parent, true ),
      m_indexManager( 0 )
{
    // only so ResourceManager won't open yet another connection to the nepomuk server
    ResourceManager::instance()->setOverrideMainModel( mainModel() );


    // lower process priority - we do not want to spoil KDE usage
    // ==============================================================
    if ( !lowerPriority() )
        kDebug() << "Failed to lower priority.";
    if ( !lowerSchedulingPriority() )
        kDebug() << "Failed to lower scheduling priority.";
    if ( !lowerIOPriority() )
        kDebug() << "Failed to lower io priority.";


    // Using Strigi with the redland backend is torture.
    // Thus we simply fail initialization if it is used
    // ==============================================================
    if ( org::kde::nepomuk::Storage( "org.kde.NepomukStorage",
                                     "/nepomukstorage",
                                     QDBusConnection::sessionBus() )
         .usedSopranoBackend().value() != QString::fromLatin1( "redland" ) ) {

        // setup the actual index scheduler including strigi stuff
        // ==============================================================
        if ( ( m_indexManager = Strigi::IndexPluginLoader::createIndexManager( "sopranobackend", 0 ) ) ) {
            m_indexScheduler = new IndexScheduler( m_indexManager, this );

            // monitor all kinds of events
            ( void )new EventMonitor( m_indexScheduler, this );

            // monitor the file system
            m_fsWatcher = new FileSystemWatcher( this );
            m_fsWatcher->setWatchRecursively( true );
            connect( m_fsWatcher, SIGNAL( dirty( QString ) ),
                     this, SLOT( slotDirDirty( QString ) ) );

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
            connect( m_fsWatcher, SIGNAL( statusChanged(FileSystemWatcher::Status) ),
                     this, SIGNAL( statusStringChanged() ) );


            // start watching the index folders
            updateWatches();

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
    else {
        kDebug() << "Will not start when using redland Soprano backend due to horrible performance.";
        setServiceInitialized( false );
    }
}


Nepomuk::StrigiService::~StrigiService()
{
    if ( m_indexManager ) {
        m_indexScheduler->stop();
        m_indexScheduler->wait();
        Strigi::IndexPluginLoader::deleteIndexManager( m_indexManager );
    }
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


QString Nepomuk::StrigiService::userStatusString() const
{
    bool indexing = m_indexScheduler->isIndexing();
    bool suspended = m_indexScheduler->isSuspended();
    QString folder = m_indexScheduler->currentFolder();

    if ( suspended )
        return i18nc( "@info:status", "File indexer is suspended" );
    else if ( indexing )
        return i18nc( "@info:status", "Strigi is currently indexing files in folder %1", folder );
    else if ( m_fsWatcher->status() == FileSystemWatcher::Checking )
        return i18nc( "@info:status", "Checking file system for new files" );
    else
        return i18nc( "@info:status", "File indexer is idle" );
}


void Nepomuk::StrigiService::setSuspended( bool suspend )
{
    if ( suspend ) {
        m_indexScheduler->suspend();
        m_fsWatcher->suspend();
    }
    else {
        m_indexScheduler->resume();
        m_fsWatcher->resume();
    }
}


#include <kpluginfactory.h>
#include <kpluginloader.h>

NEPOMUK_EXPORT_SERVICE( Nepomuk::StrigiService, "nepomukstrigiservice" )

#include "strigiservice.moc"

