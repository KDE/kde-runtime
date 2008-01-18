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

#include <Soprano/Global>

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>

#include <QtDBus/QDBusConnection>


Nepomuk::Server::Server()
    : KDEDModule(),
      m_core( 0 ),
      m_strigiController( new StrigiController( this ) ),
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
        m_strigiController->shutdown();
        updateStrigiConfig();
        m_strigiController->start();
    }
#endif

    NepomukServerSettings::self()->setStartNepomuk( enabled );
}


void Nepomuk::Server::enableStrigi( bool enabled )
{
    kDebug(300002) << enabled;

    if ( enabled ) {
        startStrigi();
    }
    else {
        m_strigiController->shutdown();
    }

    NepomukServerSettings::self()->setStartStrigi( enabled );
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
