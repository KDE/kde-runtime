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

#include "nepomukserver.h"
#include "nepomukserveradaptor.h"
#include "nepomukserversettings.h"
#include "servicemanager.h"
#include "servicemanageradaptor.h"

#include <Soprano/Global>

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KStandardDirs>

#include <QtDBus/QDBusConnection>


Nepomuk::Server* Nepomuk::Server::s_self = 0;

Nepomuk::Server::Server( QObject* parent )
    : QObject( parent ),
      m_enabled( false ),
      m_strigiServiceName( "nepomukstrigiservice" )
{
    s_self = this;

    m_config = KSharedConfig::openConfig( "nepomukserverrc" );

    QDBusConnection::sessionBus().registerService( "org.kde.NepomukServer" );

    // register the nepomuk server adaptor
    (void)new NepomukServerAdaptor( this );
    QDBusConnection::sessionBus().registerObject( "/nepomukserver", this );

    // create the service manager.
    m_serviceManager = new ServiceManager( this );
    (void)new ServiceManagerAdaptor( m_serviceManager );

    // initialize according to config
    init();
}


Nepomuk::Server::~Server()
{
    m_serviceManager->stopAllServices();
    NepomukServerSettings::self()->writeConfig();
    QDBusConnection::sessionBus().unregisterService( "org.kde.NepomukServer" );
}


void Nepomuk::Server::init()
{
    // no need to start strigi explicetely. it is done in enableNepomuk
    enableNepomuk( NepomukServerSettings::self()->startNepomuk() );
}


void Nepomuk::Server::enableNepomuk( bool enabled )
{
    kDebug() << "enableNepomuk" << enabled;
    if ( enabled != m_enabled ) {
        if ( enabled ) {
            // start all autostart services
            m_serviceManager->startAllServices();

            // register the service manager interface
            QDBusConnection::sessionBus().registerObject( "/servicemanager", m_serviceManager );

            // now nepomuk is enabled
            m_enabled = true;
        }
        else {
            // stop all running services
            m_serviceManager->stopAllServices();

            // unregister the service manager interface
            QDBusConnection::sessionBus().unregisterObject( "/servicemanager" );

            // nepomuk is disabled
            m_enabled = false;
        }
    }
}


void Nepomuk::Server::enableStrigi( bool enabled )
{
    kDebug() << enabled;
    if ( isNepomukEnabled() ) {
        if ( enabled ) {
            m_serviceManager->startService( m_strigiServiceName );
        }
        else {
            m_serviceManager->stopService( m_strigiServiceName );
        }
    }

    KConfigGroup config( m_config, QString("Service-%1").arg(m_strigiServiceName) );
    config.writeEntry( "autostart", enabled );
}


bool Nepomuk::Server::isNepomukEnabled() const
{
    return m_enabled;
}


bool Nepomuk::Server::isStrigiEnabled() const
{
    return m_serviceManager->runningServices().contains( m_strigiServiceName );
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


void Nepomuk::Server::quit()
{
    QCoreApplication::instance()->quit();
}


KSharedConfig::Ptr Nepomuk::Server::config() const
{
    return m_config;
}


Nepomuk::Server* Nepomuk::Server::self()
{
    return s_self;
}

#include "nepomukserver.moc"
