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
#include <KTitleWidget>
#include <KStandardDirs>
#include <KIO/NetAccess>
#include <kio/directorysizejob.h>

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Vocabulary/NFO>

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/Xesam>
#include <Soprano/Util/AsyncQuery>

#include <QtCore/QTimer>


Nepomuk::StatusWidget::StatusWidget( QWidget* parent )
    : KDialog( parent ),
      m_connected( false ),
      m_updatingJobCnt( 0 ),
      m_updateRequested( false )
{
    setupUi( mainWidget() );

    setCaption( m_title->text() );
    setButtons( Close );
    setDefaultButton( Close );

    KIcon icon( "nepomuk" );
    m_title->setPixmap( icon.pixmap( 32, 32 ) );
    setWindowIcon( icon );

    m_updateTimer.setSingleShot( true );
    m_updateTimer.setInterval( 10*1000 ); // do not update multiple times in 10 seconds
    connect( &m_updateTimer, SIGNAL( timeout() ),
             this, SLOT( slotUpdateTimeout() ) );
}


Nepomuk::StatusWidget::~StatusWidget()
{
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
            = Soprano::Util::AsyncQuery::executeQuery( ResourceManager::instance()->mainModel(),
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


void Nepomuk::StatusWidget::showEvent( QShowEvent* event )
{
    if ( !m_connected ) {
        connect( ResourceManager::instance()->mainModel(), SIGNAL( statementsAdded() ),
                 this, SLOT( slotUpdateStoreStatus() ) );
        connect( ResourceManager::instance()->mainModel(), SIGNAL( statementsRemoved() ),
                 this, SLOT( slotUpdateStoreStatus() ) );

        m_connected = true;
    }

    QTimer::singleShot( 0, this, SLOT( slotUpdateStoreStatus() ) );

    KDialog::showEvent( event );
}


void Nepomuk::StatusWidget::hideEvent( QHideEvent* event )
{
    if ( m_connected ) {
        ResourceManager::instance()->mainModel()->disconnect( this );
        m_connected = false;
    }

    KDialog::hideEvent( event );
}

#include "statuswidget.moc"
