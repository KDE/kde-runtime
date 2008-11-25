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
#include "config.h"
#include "statuswidget.h"
#include "nepomukstorageinterface.h"

#include <KDebug>

#include <strigi/indexpluginloader.h>
#include <strigi/indexmanager.h>

#include <nepomuk/resourcemanager.h>


Nepomuk::StrigiService::StrigiService( QObject* parent, const QList<QVariant>& )
    : Service( parent, true )
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

            ( void )new EventMonitor( m_indexScheduler, this );
            ( void )new StrigiServiceAdaptor( m_indexScheduler, this );
            StatusWidget* sw = new StatusWidget( mainModel(), m_indexScheduler );
            ( new SystemTray( m_indexScheduler, sw ) )->show();

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


#include <kpluginfactory.h>
#include <kpluginloader.h>

NEPOMUK_EXPORT_SERVICE( Nepomuk::StrigiService, "nepomukstrigiservice" )

#include "strigiservice.moc"

