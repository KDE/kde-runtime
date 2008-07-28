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

#include "statuswidget.h"
#include "indexscheduler.h"

#include <KCMultiDialog>
#include <KIcon>
#include <KLocale>
#include <KTitleWidget>
#include <KStandardDirs>
#include <KIO/NetAccess>
#include <kio/directorysizejob.h>

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/Xesam>

#include <QtCore/QTimer>


Nepomuk::StatusWidget::StatusWidget( Soprano::Model* model, IndexScheduler* scheduler, QWidget* parent )
    : KDialog( parent ),
      m_model( model ),
      m_indexScheduler( scheduler ),
      m_connected( false ),
      m_updating( false ),
      m_updateRequested( false )
{
    setupUi( mainWidget() );

    setCaption( m_title->text() );
    setButtons( Ok|User1 );
    setDefaultButton( Ok );
    setButtonGuiItem( User1, KGuiItem( i18n( "Configure" ), KIcon( "configure" ) ) );

    m_title->setPixmap( KIcon( "nepomuk" ).pixmap( 32, 32 ) );

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
    bool indexing = m_indexScheduler->isIndexing();
    bool suspended = m_indexScheduler->isSuspended();
    QString folder = m_indexScheduler->currentFolder();

    if ( suspended )
        m_labelStrigiState->setText( i18n( "File indexer is suspended" ) );
    else if ( indexing )
        m_labelStrigiState->setText( i18n( "Strigi is currently indexing files in folder %1", folder ) );
    else
        m_labelStrigiState->setText( i18n( "File indexer is idle" ) );
}


void Nepomuk::StatusWidget::slotUpdateStoreStatus()
{
    if ( !m_updating && !m_updateTimer.isActive() ) {
        m_updating = true;

        // update storage size
        // ========================================
        QString path = KStandardDirs::locateLocal( "data", "nepomuk/repository/main/", false );
        KIO::DirectorySizeJob* job = KIO::directorySize( path );
        if ( KIO::NetAccess::synchronousRun( job, this ) )
            m_labelStoreSize->setText( KIO::convertSize( job->totalSize() ) );
        else
            m_labelStoreSize->setText( i18n( "Calculation failed" ) );


        // update file count
        // ========================================
        Soprano::QueryResultIterator it = m_model->executeQuery( QString( "select distinct ?r where { ?r a <%1> . }" )
                                                                 .arg( Soprano::Vocabulary::Xesam::File().toString() ),
                                                                 Soprano::Query::QueryLanguageSparql );
        int cnt = 0;
        while ( it.next() ) {
            // a bit of hacking to keep the GUI responsive
            // TODO: if we don't get aggregate functions in SPARQL soon, use a thread
            if ( cnt % 100 == 0 )
                QApplication::processEvents();
            ++cnt;
        }
        m_labelFileCount->setText( i18np( "1 files in index", "%1 files in index", cnt ) );

        m_updating = false;

        // start the timer to avoid too many updates
        m_updateTimer.start();
    }
    else {
        m_updateRequested = true;
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
    KCMultiDialog dlg;
    dlg.addModule( "kcm_nepomuk" );
    dlg.exec();
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
        connect( m_indexScheduler, SIGNAL( indexingStarted() ),
                 this, SLOT( slotUpdateStrigiStatus() ) );
        connect( m_indexScheduler, SIGNAL( indexingStopped() ),
                 this, SLOT( slotUpdateStrigiStatus() ) );
        connect( m_indexScheduler, SIGNAL( indexingFolder(QString) ),
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
        m_indexScheduler->disconnect( this );
        m_model->disconnect( this );
        m_connected = false;
    }

    KDialog::hideEvent( event );
}

#include "statuswidget.moc"
