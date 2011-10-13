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
#include "statuswidget.h"

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

#include <QApplication>
#include <QDesktopWidget>


Nepomuk::SystemTray::SystemTray( QObject* parent )
    : KStatusNotifierItem( parent ),
      m_service( 0 ),
      m_suspendedManually( false ),
      m_statusWidget( 0 )
{
    setCategory( SystemServices );
    setIconByName( "nepomuk" );
    setTitle( i18n( "Desktop Search File Indexing" ) );

    // the status widget
    connect(this, SIGNAL(activateRequested(bool,QPoint)), this, SLOT(slotActivateRequested()));

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

    // connect to the file indexer service
    m_service = new org::kde::nepomuk::FileIndexer( QLatin1String("org.kde.nepomuk.services.nepomukfileindexer"),
                                                    QLatin1String("/nepomukfileindexer"),
                                                    QDBusConnection::sessionBus(),
                                                    this );
    m_serviceControl = new org::kde::nepomuk::ServiceControl( QLatin1String("org.kde.nepomuk.services.nepomukfileindexer"),
                                                              QLatin1String("/servicecontrol"),
                                                              QDBusConnection::sessionBus(),
                                                              this );
    connect( m_service, SIGNAL( statusChanged() ), this, SLOT( slotUpdateFileIndexerStatus() ) );

    // watch for the file indexer service to come up and go down
    QDBusServiceWatcher* dbusServiceWatcher = new QDBusServiceWatcher( m_service->service(),
                                                                       QDBusConnection::sessionBus(),
                                                                       QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
                                                                       this );
    connect( dbusServiceWatcher, SIGNAL( serviceRegistered( QString ) ),
             this, SLOT( slotUpdateFileIndexerStatus()) );
    connect( dbusServiceWatcher, SIGNAL( serviceUnregistered( QString ) ),
             this, SLOT( slotUpdateFileIndexerStatus()) );

    connect( &m_updateTimer, SIGNAL( timeout() ),
             this, SLOT( slotActiveStatusTimeout()) );

    slotUpdateFileIndexerStatus();
    m_updateTimer.setSingleShot(true);

}


Nepomuk::SystemTray::~SystemTray()
{
}


void Nepomuk::SystemTray::slotUpdateFileIndexerStatus()
{
    ItemStatus newStatus = status();

    // make sure we do not update the systray icon all the time
    const bool fileIndexerInitialized =
            QDBusConnection::sessionBus().interface()->isServiceRegistered(m_service->service()) &&
            m_serviceControl->isInitialized();

    // a manually suspended service should not be passive
    if ( fileIndexerInitialized ) {
        if ( m_service->isIndexing() || m_suspendedManually) {
            if (!m_updateTimer.isActive()) {
                m_updateTimer.start(3000);
            }
        }
        else {
            m_updateTimer.stop();
            newStatus = Passive;
        }
    }
    else
        newStatus = Passive;
    if ( newStatus != status() ) {
        setStatus( newStatus );
    }

    QString statusString;
    if( fileIndexerInitialized )
        statusString = m_service->userStatusString();
    else
        statusString = i18n("File indexing service not running");
    if ( statusString != m_prevStatus ) {
        m_prevStatus = statusString;
        setToolTip("nepomuk", i18n("Search Service"), statusString );
    }

    m_suspendResumeAction->setEnabled( fileIndexerInitialized );
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


// from kdialog.cpp since KDialog::centerOnScreen will simply do nothing on X11!
static QRect screenRect( QWidget *widget, int screen )
{
    QDesktopWidget *desktop = QApplication::desktop();
    KConfig gc( "kdeglobals", KConfig::NoGlobals );
    KConfigGroup cg(&gc, "Windows" );
    if ( desktop->isVirtualDesktop() &&
         cg.readEntry( "XineramaEnabled", true ) &&
         cg.readEntry( "XineramaPlacementEnabled", true ) ) {

        if ( screen < 0 || screen >= desktop->numScreens() ) {
            if ( screen == -1 )
                screen = desktop->primaryScreen();
            else if ( screen == -3 )
                screen = desktop->screenNumber( QCursor::pos() );
            else
                screen = desktop->screenNumber( widget );
        }

        return desktop->availableGeometry( screen );
    } else
        return desktop->geometry();
}

void Nepomuk::SystemTray::slotActivateRequested()
{
    if(!m_statusWidget)
        m_statusWidget = new Nepomuk::StatusWidget();

    if(!m_statusWidget->isVisible()) {
        m_statusWidget->show();

        const QRect rect = screenRect( 0, -3 );
        m_statusWidget->move( rect.center().x() - m_statusWidget->width() / 2,
                              rect.center().y() - m_statusWidget->height() / 2 );
    }
    else {
        m_statusWidget->hide();
    }
}

void Nepomuk::SystemTray::slotActiveStatusTimeout()
{
    setStatus(Active);
} 
#include "systray.moc"
