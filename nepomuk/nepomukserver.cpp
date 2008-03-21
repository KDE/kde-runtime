/* This file is part of the KDE Project
   Copyright (c) 2007 Sebastian Trueg <trueg@kde.org>

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

#include "nepomukserver-config.h"

#include "nepomukserver.h"
#include "nepomukcore.h"
#include "strigicontroller.h"
#include "strigiconfigfile.h"
#include "nepomukserveradaptor.h"
#include "nepomukserversettings.h"
#include "ontologyloader.h"

#include <Soprano/Global>

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KStandardDirs>

#include <QtDBus/QDBusConnection>


Nepomuk::Server* Nepomuk::Server::s_self = 0;

Nepomuk::Server::Server()
    : KDEDModule(),
      m_core( 0 ),
      m_strigiController( new StrigiController( this ) ),
      m_restartStrigiAfterInitialization( false )
{
    s_self = this;

    m_config = KSharedConfig::openConfig( "nepomukserverrc" );

    // we want to be accessible through our own nice nepomuk service,
    // not only kded
    QDBusConnection::sessionBus().registerService( "org.kde.NepomukServer" );

    // register the nepomuk server adaptor
    new NepomukServerAdaptor( this );

    // initialize according to config
    init();
}


Nepomuk::Server::~Server()
{
    NepomukServerSettings::self()->writeConfig();
    QDBusConnection::sessionBus().unregisterService( "org.kde.NepomukServer" );
}


void Nepomuk::Server::init()
{
    enableNepomuk( NepomukServerSettings::self()->startNepomuk() );
    enableStrigi( NepomukServerSettings::self()->startStrigi() );
}


void Nepomuk::Server::startStrigi()
{
    // make sure we have a proper Strigi config
    updateStrigiConfig();

    // start Strigi
    if ( !StrigiController::isRunning() ) {
        m_strigiController->start();
    }
}


void Nepomuk::Server::startNepomuk()
{
    if ( Repository::activeSopranoBackend() && !m_core ) {
        m_core = new Core( this );
        connect( m_core, SIGNAL( initializationDone(bool) ),
                 this, SLOT( slotNepomukCoreInitialized(bool) ) );
        m_core->init();
    }
}


void Nepomuk::Server::enableNepomuk( bool enabled )
{
    kDebug(300002) << "enableNepomuk" << enabled;
    bool needToRestartStrigi = ( NepomukServerSettings::self()->startStrigi() &&
                                 enabled != NepomukServerSettings::self()->startNepomuk() );
    m_restartStrigiAfterInitialization = enabled && needToRestartStrigi;

    NepomukServerSettings::self()->setStartNepomuk( enabled );

    // 1. restart strigi if necessary
    //    We do this before starting or stopping Nepomuk for 2 reasons:
    //    * Let Strigi go down with a running Nepomuk -> no errors
    //    * NepomukCore might emit the initializationDone signal before
    //      this method is done
    // =================================================================

    // FIXME: make this a runtime descision
#ifdef HAVE_STRIGI_SOPRANO_BACKEND
    if ( needToRestartStrigi ) {
        m_strigiController->shutdown();
        updateStrigiConfig();
        // if nepomuk is enabled wait for it to be initialized
        if ( !enabled ) {
            m_strigiController->start();
        }
    }
#endif

    // 2. Start/Stop Nepomuk
    // =====================
    if ( enabled && !m_core ) {
        startNepomuk();
    }
    else if ( !enabled ) {
        delete m_core;
        m_core = 0;
    }
}


void Nepomuk::Server::enableStrigi( bool enabled )
{
    kDebug(300002) << enabled;

    NepomukServerSettings::self()->setStartStrigi( enabled );

    if ( enabled ) {
        startStrigi();
    }
    else {
        m_strigiController->shutdown();
    }
}


void Nepomuk::Server::updateStrigiConfig()
{
    StrigiConfigFile strigiConfig ( StrigiConfigFile::defaultStrigiConfigFilePath() );
    strigiConfig.load();
    if ( NepomukServerSettings::self()->startNepomuk() ) {
        strigiConfig.defaultRepository().setType( "sopranobackend" );
    }
    else {
        strigiConfig.defaultRepository().setType( "clucene" );
    }
    strigiConfig.save();
}


bool Nepomuk::Server::isNepomukEnabled() const
{
    kDebug(300002) << "m_core=" << m_core;
    return m_core != 0;
}


bool Nepomuk::Server::isStrigiEnabled() const
{
    kDebug(300002 );
    return NepomukServerSettings::self()->startStrigi();
}


QString Nepomuk::Server::defaultRepository() const
{
    return "main";
}


void Nepomuk::Server::reconfigure()
{
    NepomukServerSettings::self()->config()->sync();
    NepomukServerSettings::self()->readConfig();
    init();
}


KSharedConfig::Ptr Nepomuk::Server::config() const
{
    return m_config;
}


Nepomuk::Server* Nepomuk::Server::self()
{
    return s_self;
}


void Nepomuk::Server::slotNepomukCoreInitialized( bool success )
{
    if ( success ) {
        kDebug() << "Successfully initialized nepomuk core";

        // the core is initialized. Export it to the clients.
        // the D-Bus interface
        m_core->registerAsDBusObject();

        // the faster local socket interface
        m_core->start( KGlobal::dirs()->locateLocal( "data", "nepomuk/socket" ) );

        if ( m_restartStrigiAfterInitialization ) {
            m_strigiController->start();
        }

        // FIXME: move this into an extra module once we have proper dependancy handling
        m_ontologyLoader = new OntologyLoader( m_core->model( "main" ), this );
        QTimer::singleShot( 0, m_ontologyLoader, SLOT( update() ) );
    }
    else {
        kDebug() << "Failed to initialize nepomuk core";
    }
}


extern "C"
{
    KDE_EXPORT KDEDModule *create_nepomukserver()
    {
        return new Nepomuk::Server();
    }
}

#include "nepomukserver.moc"
