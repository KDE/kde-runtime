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
#include "indexscheduler.h"
#include "strigiservice.h"
#include "nfo.h"

#include <KToolInvocation>
#include <KIcon>
#include <KLocale>
#include <KTitleWidget>
#include <KStandardDirs>
#include <KIO/NetAccess>
#include <kio/directorysizejob.h>

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/Xesam>
#include <Soprano/Util/AsyncQuery>

#include <QtCore/QTimer>


Nepomuk::StatusWidget::StatusWidget( Soprano::Model* model, StrigiService* service, QWidget* parent )
    : KDialog( parent ),
      m_model( model ),
      m_service( service ),
      m_connected( false ),
      m_updatingJobCnt( 0 ),
      m_updateRequested( false )
{
    setupUi( mainWidget() );

    setCaption( m_title->text() );
    setButtons( Ok|User1 );
    setDefaultButton( Ok );
    setButtonGuiItem( User1, KGuiItem( i18n( "Configure" ), KIcon( "configure" ) ) );

    KIcon icon( "nepomuk" );
    m_title->setPixmap( icon.pixmap( 32, 32 ) );
    setWindowIcon( icon );
    
    m_updateTimer.setSingleShot( true );
    m_updateTimer.setInterval( 10*1000 ); // do not update multiple times in 10 seconds
    connect( &m_updateTimer, SIGNAL( timeout() ),
             this, SLOT( slotUpdateTimeout() ) );

    connect( this, SIGNAL( user1Clicked() ),
             this, SLOT( slotConfigure() ) );
}


Nepomuk::StatusWidget::~StatusWidget()
{
}


void Nepomuk::StatusWidget::slotUpdateStrigiStatus()
{
    m_labelStrigiState->setText( m_service->userStatusString() );
}


void Nepomuk::StatusWidget::slotUpdateStoreStatus()
{
    if ( !m_updatingJobCnt && !m_updateTimer.isActive() ) {
        m_updatingJobCnt = 2;

        // update storage size
        // ========================================
        QString path = KStandardDirs::locateLocal( "data", "nepomuk/repository/main/", false );
        KIO::DirectorySizeJob* job = KIO::directorySize( path );
        connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotStoreSizeCalculated( KJob* ) ) );
        job->start();

        // update file count
        // ========================================
        Soprano::Util::AsyncQuery* query
            = Soprano::Util::AsyncQuery::executeQuery( m_model,
                                                       QString::fromLatin1( "select count(distinct ?r) where { ?r a %1 . }" )
                                                       .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NFO::FileDataObject() ) ),
                                                       Soprano::Query::QueryLanguageSparql );
        connect( query, SIGNAL( nextReady(Soprano::Util::AsyncQuery*) ), this, SLOT( slotFileCountFinished(Soprano::Util::AsyncQuery*) ) );
    }
    else {
        m_updateRequested = true;
    }
}


void Nepomuk::StatusWidget::slotStoreSizeCalculated( KJob* job )
{
    KIO::DirectorySizeJob* dirJob = static_cast<KIO::DirectorySizeJob*>( job );
    if ( !job->error() )
        m_labelStoreSize->setText( KIO::convertSize( dirJob->totalSize() ) );
    else
        m_labelStoreSize->setText( i18n( "Calculation failed" ) );

    if ( !--m_updatingJobCnt ) {
        // start the timer to avoid too many updates
        m_updateTimer.start();
    }
}


void Nepomuk::StatusWidget::slotFileCountFinished( Soprano::Util::AsyncQuery* query )
{
    m_labelFileCount->setText( i18np( "1 file in index", "%1 files in index", query->binding( 0 ).literal().toInt() ) );
    query->deleteLater();

    if ( !--m_updatingJobCnt ) {
        // start the timer to avoid too many updates
        m_updateTimer.start();
    }
}


void Nepomuk::StatusWidget::slotUpdateTimeout()
{
    if ( m_updateRequested ) {
        m_updateRequested = false;
        slotUpdateStoreStatus();
    }
}


void Nepomuk::StatusWidget::slotConfigure()
{
    QStringList args;
    args << "kcm_nepomuk";
    KToolInvocation::kdeinitExec("kcmshell4", args);
}

#include <QApplication>
#include <QDesktopWidget>

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

void Nepomuk::StatusWidget::showEvent( QShowEvent* event )
{
    if ( !m_connected ) {
        connect( m_service, SIGNAL( statusStringChanged() ),
                 this, SLOT( slotUpdateStrigiStatus() ) );

        connect( m_model, SIGNAL( statementsAdded() ),
                 this, SLOT( slotUpdateStoreStatus() ) );
        connect( m_model, SIGNAL( statementsRemoved() ),
                 this, SLOT( slotUpdateStoreStatus() ) );

        m_connected = true;
    }

    QTimer::singleShot( 0, this, SLOT( slotUpdateStoreStatus() ) );
    QTimer::singleShot( 0, this, SLOT( slotUpdateStrigiStatus() ) );

    KDialog::showEvent( event );

    QRect rect = screenRect( this, -1 );
    move( rect.center().x() - width() / 2,
          rect.center().y() - height() / 2 );
    //KDialog::centerOnScreen( this );
}


void Nepomuk::StatusWidget::hideEvent( QHideEvent* event )
{
    if ( m_connected ) {
        m_service->disconnect( this );
        m_model->disconnect( this );
        m_connected = false;
    }

    KDialog::hideEvent( event );
}

#include "statuswidget.moc"
