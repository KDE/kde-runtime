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
#include "nepomukserveradaptor.h"
#include "nepomukserversettings.h"

#include <strigi/qtdbus/strigiclient.h>

#include <Soprano/Global>

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>

#include <QtDBus/QDBusConnection>


Nepomuk::Server::Server()
    : KDEDModule(),
      m_core( 0 ),
      m_strigiController( 0 ),
      m_strigi( new StrigiClient() ),
      m_backend( 0 )
{
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
    delete m_strigi;
}


void Nepomuk::Server::init()
{
    enableNepomuk( NepomukServerSettings::self()->startNepomuk() );
    enableStrigi( NepomukServerSettings::self()->startStrigi() );
}


void Nepomuk::Server::startStrigi()
{
    if ( !StrigiController::isRunning() ) {
        if ( !m_strigiController ) {
            m_strigiController = new StrigiController( this );
        }
        m_strigiController->start(
#ifdef HAVE_STRIGI_SOPRANO_BACKEND
            NepomukServerSettings::self()->startNepomuk()
#else
            false
#endif
            );

        m_strigi->startIndexing();
    }
}


void Nepomuk::Server::startNepomuk()
{
    m_backend = findBackend();

    if ( m_backend && !m_core ) {
        Soprano::setUsedBackend( m_backend );
        m_core = new Core( this );
    }
}


void Nepomuk::Server::enableNepomuk( bool enabled )
{
    kDebug(300002) << "enableNepomuk" << enabled;
    bool needToRestartStrigi = ( NepomukServerSettings::self()->startStrigi() &&
                                 enabled != NepomukServerSettings::self()->startNepomuk() );
    if ( enabled && !m_core ) {
        startNepomuk();
    }
    else if ( !enabled ) {
        delete m_core;
        m_core = 0;
    }

    // FIXME: make this a runtime descision
#ifdef HAVE_STRIGI_SOPRANO_BACKEND
    if ( needToRestartStrigi ) {
        // we use the dbus interface to also shut it down in case we did not start it ourselves
        m_strigi->stopDaemon();
        m_strigiController->start( enabled );
    }
#endif

    NepomukServerSettings::self()->setStartNepomuk( enabled );
}


void Nepomuk::Server::enableStrigi( bool enabled )
{
    kDebug(300002) << "enableStrigi" << enabled;
    if ( enabled ) {
        startStrigi();
        m_strigi->startIndexing();
    }
    else {
        if ( m_strigiController && m_strigiController->state() == StrigiController::Running ) {
            m_strigiController->shutdown();
        }
        else if ( StrigiController::isRunning() ) {
            m_strigi->stopDaemon();
        }
    }

    NepomukServerSettings::self()->setStartStrigi( enabled );
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

    // FIXME: This is completely useless since it always returns "idling", regardless of its actual state (indexing or not indexing)
//     kDebug(300002) << "strigi status=" << m_strigi->getStatus()["Status"];
//     return m_strigi->getStatus()["Status"] != "stopping";
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


const Soprano::Backend* Nepomuk::Server::findBackend() const
{
    QString backendName = NepomukServerSettings::self()->sopranoBackend();
    const Soprano::Backend* backend = ::Soprano::discoverBackendByName( backendName );
    if ( !backend ) {
        kDebug(300002) << "(Nepomuk::Core::Core) could not find backend" << backendName << ". Falling back to default.";
        backend = ::Soprano::usedBackend();
    }
    if ( !backend ) {
        kDebug(300002) << "(Nepomuk::Core::Core) could not find a backend.";
    }
    return backend;
}


extern "C"
{
    KDE_EXPORT KDEDModule *create_nepomukserver()
    {
        return new Nepomuk::Server();
    }
}

#include "nepomukserver.moc"
