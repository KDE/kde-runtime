/* This file is part of the KDE Project
   Copyright (c) 2010 Sebastian Trueg <trueg@kde.org>

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

#include "useractivitymonitor.h"

#include <QtCore/QTimer>
#include <QtGui/QCursor>


Nepomuk::UserActivityMonitor::UserActivityMonitor( QObject* parent )
    : QObject( parent ),
      m_userActiveAtLastCheck( false )
{
    m_wakeUpTimer = new QTimer( this );
    m_wakeUpTimer->setInterval( 10*1000 );
    connect( m_wakeUpTimer, SIGNAL( timeout() ), SLOT( checkUserActivity() ) );
}


Nepomuk::UserActivityMonitor::~UserActivityMonitor()
{
}


void Nepomuk::UserActivityMonitor::start()
{
    m_cursorPos = QCursor::pos();
    m_wakeUpTimer->start();
}


void Nepomuk::UserActivityMonitor::checkUserActivity()
{
    const QPoint cursorPos = QCursor::pos();
    if ( QPoint( cursorPos - m_cursorPos ).manhattanLength() > 10 ) {
        if ( !m_userActiveAtLastCheck ) {
            m_userActiveAtLastCheck = true;

            // use a longer check time of 2 minutes before next check
            // to not toggle between active and non-active all the time
//            m_wakeUpTimer->setInterval( 2*60*1000 );

            emit userActive( true );
        }
    }
    else {
        if ( m_userActiveAtLastCheck ) {
            m_userActiveAtLastCheck = false;

            // use a shorter interval during indexing to allow a fast
            // stopping after a user interaction
            m_wakeUpTimer->setInterval( 10*1000 );

            emit userActive( false );
        }
    }
}

#include "useractivitymonitor.moc"
