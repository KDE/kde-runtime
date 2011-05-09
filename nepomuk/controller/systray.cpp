/* This file is part of the KDE Project
   Copyright (c) 2008-2011 Sebastian Trueg <trueg@kde.org>

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

#include "systray.h"

#include <KMenu>
#include <KToggleAction>
#include <KLocale>
#include <KIcon>
#include <KToolInvocation>
#include <KActionCollection>
#include <KStandardAction>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusServiceWatcher>


Nepomuk::SystemTray::SystemTray( QObject* parent )
    : KStatusNotifierItem( parent ),
      m_service( 0 ),
      m_suspendedManually( false )
{
    setCategory( SystemServices );
    setIconByName( "nepomuk" );
    setTitle( i18n( "Nepomuk File Indexing" ) );

    m_suspendResumeAction = new KToggleAction( i18n( "Suspend File Indexing" ), actionCollection() );
    m_suspendResumeAction->setCheckedState( KGuiItem( i18n( "Suspend File Indexing" ) ) );
    m_suspendResumeAction->setToolTip( i18n( "Suspend or resume the file indexer manually" ) );
    connect( m_suspendResumeAction, SIGNAL( toggled( bool ) ),
             this, SLOT( slotSuspend( bool ) ) );

    KAction* configAction = new KAction( actionCollection() );
    configAction->setText( i18n( "Configure File Indexing" ) );
    configAction->setIcon( KIcon( "configure" ) );
    connect( configAction, SIGNAL( triggered() ),
             this, SLOT( slotConfigure() ) );

    contextMenu()->addAction( m_suspendResumeAction );
    contextMenu()->addAction( configAction );

    // connect to the strigi service
    m_service = new org::kde::nepomuk::Strigi( QLatin1String("org.kde.nepomuk.services.nepomukstrigiservice"),
                                               QLatin1String("/nepomukstrigiservice"),
                                               QDBusConnection::sessionBus(),
                                               this );
    m_serviceControl = new org::kde::nepomuk::ServiceControl( QLatin1String("org.kde.nepomuk.services.nepomukstrigiservice"),
                                                              QLatin1String("/servicecontrol"),
                                                              QDBusConnection::sessionBus(),
                                                              this );
    connect( m_service, SIGNAL( statusChanged() ), this, SLOT( slotUpdateStrigiStatus() ) );

    // watch for the strigi service to come up and go down
    QDBusServiceWatcher* dbusServiceWatcher = new QDBusServiceWatcher( m_service->service(),
                                                                       QDBusConnection::sessionBus(),
                                                                       QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
                                                                       this );
    connect( dbusServiceWatcher, SIGNAL( serviceRegistered( QString ) ),
             this, SLOT( slotUpdateStrigiStatus()) );
    connect( dbusServiceWatcher, SIGNAL( serviceUnregistered( QString ) ),
             this, SLOT( slotUpdateStrigiStatus()) );

    slotUpdateStrigiStatus();
}


Nepomuk::SystemTray::~SystemTray()
{
}


void Nepomuk::SystemTray::slotUpdateStrigiStatus()
{
    // make sure we do not update the systray icon all the time

    ItemStatus newStatus = status();
    const bool strigiServiceInitialized =
            QDBusConnection::sessionBus().interface()->isServiceRegistered(m_service->service()) &&
            m_serviceControl->isInitialized();

    // a manually suspended service should not be passive
    if( strigiServiceInitialized )
        newStatus = m_service->isIndexing() || m_suspendedManually ? Active : Passive;
    else
        newStatus = Passive;
    if ( newStatus != status() ) {
        setStatus( newStatus );
    }

    QString statusString;
    if( strigiServiceInitialized )
        statusString = m_service->userStatusString();
    else
        statusString = i18n("Nepomuk File Indexing Service not running");
    if ( statusString != m_prevStatus ) {
        m_prevStatus = statusString;
        setToolTip("nepomuk", i18n("Search Service"), statusString );
    }

    m_suspendResumeAction->setEnabled( strigiServiceInitialized );
    m_suspendResumeAction->setChecked( m_service->isSuspended() );
}


void Nepomuk::SystemTray::slotConfigure()
{
    QStringList args;
    args << "kcm_nepomuk";
    KToolInvocation::kdeinitExec("kcmshell4", args);
}


void Nepomuk::SystemTray::slotSuspend( bool suspended )
{
    m_suspendedManually = suspended;
    if( suspended )
        m_service->suspend();
    else
        m_service->resume();
}

#include "systray.moc"
