/* This file is part of the KDE Project
   Copyright (c) 2008-2010 Sebastian Trueg <trueg@kde.org>

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

#include "statuswidget.h"

#include <KIcon>
#include <KLocale>
#include <KStandardDirs>
#include <KToolInvocation>
#include <KIO/NetAccess>
#include <kio/directorysizejob.h>

#include <Nepomuk2/ResourceManager>
#include <Nepomuk2/Vocabulary/NFO>

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/Xesam>
#include <Soprano/Util/AsyncQuery>

#include <QtCore/QTimer>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusServiceWatcher>

Nepomuk2::StatusWidget::StatusWidget( QWidget* parent )
        : KDialog( parent ),
        m_connected( false ),
        m_updateRunning( false ),
        m_updateRequested( false ),
        m_fileIndexerService(0)
{
    KGlobal::locale()->insertCatalog("kcm_nepomuk");
    setupUi( mainWidget() );
    mainWidget()->layout()->setContentsMargins( 0, 0, 0, 0 );

    //setCaption( m_labelTitle->text() );
    setButtons( Close );
    setDefaultButton( Close );

    KIcon icon( "nepomuk" );
    iconLabel->setPixmap( icon.pixmap( 48, 48 ) );
    setWindowIcon( icon );

    m_configureButton->setIcon( KIcon("configure") );

    m_updateTimer.setSingleShot( true );
    m_updateTimer.setInterval( 10*1000 ); // do not update multiple times in 10 seconds
    connect( &m_updateTimer, SIGNAL( timeout() ),
             this, SLOT( slotUpdateTimeout() ) );

    // connect to the file indexer service
    m_fileIndexerService = new org::kde::nepomuk::FileIndexer( QLatin1String("org.kde.nepomuk.services.nepomukfileindexer"),
            QLatin1String("/nepomukfileindexer"),
            QDBusConnection::sessionBus(),
            this );
    m_fileIndexerServiceControl = new org::kde::nepomuk::ServiceControl( QLatin1String("org.kde.nepomuk.services.nepomukfileindexer"),
            QLatin1String("/servicecontrol"),
            QDBusConnection::sessionBus(),
            this );
    connect( m_fileIndexerService, SIGNAL( statusChanged() ), this, SLOT( slotUpdateStatus() ) );

    // watch for the file indexer service to come up and go down
    QDBusServiceWatcher* dbusServiceWatcher = new QDBusServiceWatcher( m_fileIndexerService->service(),
            QDBusConnection::sessionBus(),
            QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
            this );
    connect( dbusServiceWatcher, SIGNAL( serviceRegistered( QString ) ),
             this, SLOT( slotUpdateStatus()) );
    connect( dbusServiceWatcher, SIGNAL( serviceUnregistered( QString ) ),
             this, SLOT( slotUpdateStatus()) );

    slotUpdateStatus();

    connect( m_suspendResumeButton, SIGNAL(clicked()),
             this, SLOT(slotSuspendResume()) );
    connect( m_configureButton, SIGNAL(clicked()),
             this, SLOT(slotConfigure()) );

    updateSuspendResumeButtonText( m_fileIndexerService->isSuspended() );
}

Nepomuk2::StatusWidget::~StatusWidget()
{
}


void Nepomuk2::StatusWidget::slotUpdateStoreStatus()
{
    if ( !m_updateRunning && !m_updateTimer.isActive() ) {
        m_updateRunning = true;

        // update file count
        // ========================================
        QLatin1String queryStr("select count(distinct ?r) where { ?r a nfo:FileDataObject ;"
                               " kext:indexingLevel ?l . }");

        Soprano::Model* model = ResourceManager::instance()->mainModel();
        Soprano::Util::AsyncQuery* query
        = Soprano::Util::AsyncQuery::executeQuery( model, queryStr, Soprano::Query::QueryLanguageSparql );

        connect( query, SIGNAL( nextReady(Soprano::Util::AsyncQuery*) ),
                 this, SLOT( slotFileCountFinished(Soprano::Util::AsyncQuery*) ) );
    }
    else {
        m_updateRequested = true;
    }
}

void Nepomuk2::StatusWidget::slotFileCountFinished( Soprano::Util::AsyncQuery* query )
{
    m_labelFileCount->setText( i18np( "1 file in index", "%1 files in index", query->binding( 0 ).literal().toInt() ) );
    query->deleteLater();

    // start the timer to avoid too many updates
    m_updateTimer.start();
    m_updateRunning = false;
}


void Nepomuk2::StatusWidget::slotUpdateTimeout()
{
    if ( m_updateRequested ) {
        m_updateRequested = false;
        slotUpdateStoreStatus();
    }
}

void Nepomuk2::StatusWidget::slotUpdateStatus()
{
    const bool fileIndexerInitialized =
        QDBusConnection::sessionBus().interface()->isServiceRegistered(m_fileIndexerService->service()) &&
        m_fileIndexerServiceControl->isInitialized();

    QString statusString;
    if ( fileIndexerInitialized )
        statusString = m_fileIndexerService->userStatusString();
    else
        statusString = i18n("File indexing service not running");

    if ( statusString != m_labelFileIndexing->text() ) {
        m_labelFileIndexing->setText(statusString);
    }
    m_suspendResumeButton->setEnabled( fileIndexerInitialized );

    updateSuspendResumeButtonText( m_fileIndexerService->isSuspended() );
}

void Nepomuk2::StatusWidget::slotSuspendResume()
{
    if ( m_fileIndexerService->isSuspended()) {
        m_fileIndexerService->resume();
        updateSuspendResumeButtonText( false );
    }
    else {
        m_fileIndexerService->suspend();
        updateSuspendResumeButtonText( true );
    }
}

void Nepomuk2::StatusWidget::slotConfigure()
{
    KToolInvocation::kdeinitExec("kcmshell4", QStringList() << "kcm_nepomuk");
}

void Nepomuk2::StatusWidget::updateSuspendResumeButtonText(bool suspended)
{
    if (!suspended) {
        m_suspendResumeButton->setText( i18nc("Suspends the Nepomuk file indexing service.","Suspend File Indexing") );
        m_suspendResumeButton->setIcon( KIcon("media-playback-pause") );
    }
    else {
        m_suspendResumeButton->setText( i18nc("Resumes the Nepomuk file indexing service.","Resume File Indexing") );
        m_suspendResumeButton->setIcon( KIcon("media-playback-start") );
    }
}

void Nepomuk2::StatusWidget::showEvent( QShowEvent* event )
{
    if ( !m_connected ) {
        connect( ResourceManager::instance()->mainModel(), SIGNAL( statementsAdded() ),
                 this, SLOT( slotUpdateStoreStatus() ) );
        connect( ResourceManager::instance()->mainModel(), SIGNAL( statementsRemoved() ),
                 this, SLOT( slotUpdateStoreStatus() ) );

        m_connected = true;
    }

    if ( QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.internal.KSettingsWidget-kcm_nepomuk") ) {
        // hide Configure button when being run from inside the KCM
        m_configureButton->hide();
    } else {
        m_configureButton->show();
    }

    QTimer::singleShot( 0, this, SLOT( slotUpdateStoreStatus() ) );

    KDialog::showEvent( event );
}


void Nepomuk2::StatusWidget::hideEvent( QHideEvent* event )
{
    if ( m_connected ) {
        ResourceManager::instance()->mainModel()->disconnect( this );
        m_connected = false;
    }

    KDialog::hideEvent( event );
}

#include "statuswidget.moc"
