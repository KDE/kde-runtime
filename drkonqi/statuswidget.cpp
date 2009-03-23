/*******************************************************************
* statuswidget.cpp
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************/
#include "statuswidget.h"

#include <QtGui/QApplication>
#include <QtGui/QSizePolicy>
#include <QtGui/QProgressBar>
#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>

#include <klocale.h>

StatusWidget::StatusWidget( QWidget * parent ) :
    QStackedWidget( parent )
{
    //Main layout
    m_statusPage = new QWidget( this );
    m_busyPage = new QWidget( this );
    
    insertWidget( 0, m_statusPage );
    insertWidget( 1, m_busyPage );
    
    //Status widget
    m_statusLabel = new QLabel();
    m_statusLabel->setOpenExternalLinks( true );
    m_statusLabel->setTextFormat( Qt::RichText );
    
    QHBoxLayout * statusLayout = new QHBoxLayout();
    statusLayout->setContentsMargins( 0,0,0,0 );
    statusLayout->setSpacing( 2 );
    m_statusPage->setLayout( statusLayout );
    
    statusLayout->addWidget( m_statusLabel );
    
    //Busy widget
    m_progressBar = new QProgressBar();
    m_progressBar->setRange( 0, 0 );
    
    m_busyLabel = new QLabel();
    
    QHBoxLayout * busyLayout = new QHBoxLayout();
    busyLayout->setContentsMargins( 0,0,0,0 );
    busyLayout->setSpacing( 2 );
    m_busyPage->setLayout( busyLayout );
    
    busyLayout->addWidget( m_busyLabel );
    busyLayout->addStretch();
    busyLayout->addWidget( m_progressBar );
    
    setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Maximum ) );
}

void StatusWidget::setBusy( QString busyMessage )
{
    m_statusLabel->setText( QString() );
    m_busyLabel->setText( busyMessage );
    setCurrentIndex( 1 );
    setBusyCursor();
}

void StatusWidget::setIdle( QString idleMessage )
{
    m_busyLabel->setText( QString() );
    m_statusLabel->setText( idleMessage );
    setCurrentIndex( 0 );
    setIdleCursor();
}

void StatusWidget::addCustomStatusWidget( QWidget * widget )
{
    QVBoxLayout * statusLayout = static_cast<QVBoxLayout*>(m_statusPage->layout());
    
    statusLayout->addStretch();
    statusLayout->addWidget( widget );
}

void StatusWidget::setStatusLabelWordWrap( bool ww )
{
    m_statusLabel->setWordWrap( ww );
}

void StatusWidget::setBusyCursor()
{
    QApplication::setOverrideCursor( Qt::WaitCursor );
}

void StatusWidget::setIdleCursor()
{
    while( QApplication::overrideCursor() )
        QApplication::restoreOverrideCursor();
}
