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
      m_fileIndexerServiceName( "nepomukfileindexer" ),
      m_currentState(StateDisabled)
{
    s_self = this;

    m_config = KSharedConfig::openConfig( "nepomukserverrc" );

    QDBusConnection::sessionBus().registerService( "org.kde.NepomukServer" );

    // register the nepomuk server adaptor
    (void)new NepomukServerAdaptor( this );
    QDBusConnection::sessionBus().registerObject( "/nepomukserver", this );

    // create the service manager.
    m_serviceManager = new ServiceManager( this );
    connect(m_serviceManager, SIGNAL(serviceInitialized(QString)),
            this, SLOT(slotServiceInitialized(QString)));
    connect(m_serviceManager, SIGNAL(serviceStopped(QString)),
            this, SLOT(slotServiceStopped(QString)));
    (void)new ServiceManagerAdaptor( m_serviceManager );

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
    // no need to start the file indexer explicetely. it is done in enableNepomuk
    enableNepomuk( NepomukServerSettings::self()->startNepomuk() );
}


void Nepomuk::Server::enableNepomuk( bool enabled )
{
    kDebug() << "enableNepomuk" << enabled;
    if ( enabled != isNepomukEnabled() ) {
        if ( enabled ) {
            // update the server state
            m_currentState = StateEnabling;

            // start all autostart services
            m_serviceManager->startAllServices();

            // register the service manager interface
            QDBusConnection::sessionBus().registerObject( "/servicemanager", m_serviceManager );
        }
        else {
            // update the server state
            m_currentState = StateDisabling;

            // stop all running services
            m_serviceManager->stopAllServices();

            // unregister the service manager interface
            QDBusConnection::sessionBus().unregisterObject( "/servicemanager" );
        }
    }
}


void Nepomuk::Server::enableFileIndexer( bool enabled )
{
    kDebug() << enabled;
    if ( isNepomukEnabled() ) {
        if ( enabled ) {
            m_serviceManager->startService( m_fileIndexerServiceName );
        }
        else {
            m_serviceManager->stopService( m_fileIndexerServiceName );
        }
    }
}


bool Nepomuk::Server::isNepomukEnabled() const
{
    return m_currentState == StateEnabled || m_currentState == StateEnabling;
}


bool Nepomuk::Server::isFileIndexerEnabled() const
{
    return m_serviceManager->runningServices().contains( m_fileIndexerServiceName );
}


QString Nepomuk::Server::defaultRepository() const
{
    return QLatin1String("main");
}


void Nepomuk::Server::reconfigure()
{
    NepomukServerSettings::self()->config()->sync();
    NepomukServerSettings::self()->readConfig();
    init();
}


void Nepomuk::Server::quit()
{
    if( isNepomukEnabled() &&
        !m_serviceManager->runningServices().isEmpty() ) {
        connect(this, SIGNAL(nepomukDisabled()),
                qApp, SLOT(quit()));
        enableNepomuk(false);
    }
    else {
        QCoreApplication::instance()->quit();
    }
}


KSharedConfig::Ptr Nepomuk::Server::config() const
{
    return m_config;
}


Nepomuk::Server* Nepomuk::Server::self()
{
    return s_self;
}

void Nepomuk::Server::slotServiceStopped(const QString &name)
{
    Q_UNUSED(name);

    kDebug() << name;

    if(m_currentState == StateDisabling &&
       m_serviceManager->runningServices().isEmpty()) {
        m_currentState = StateDisabled;
        emit nepomukDisabled();
    }
    else {
        kDebug() << "Services still running:" << m_serviceManager->runningServices();
    }
}

void Nepomuk::Server::slotServiceInitialized(const QString &name)
{
    Q_UNUSED(name);

    if(m_currentState == StateEnabling &&
       m_serviceManager->pendingServices().isEmpty()) {
        m_currentState = StateEnabled;
        emit nepomukEnabled();
    }
}

#include "nepomukserver.moc"
