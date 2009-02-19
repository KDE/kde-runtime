/*******************************************************************
* usefulnessmeter.cpp
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

#include "usefulnessmeter.h"

#include <QtGui/QPainter>

#include <kicon.h>

UsefulnessMeter::UsefulnessMeter(QWidget * parent) : 
    QWidget(parent),
    state(CrashInfo::NonLoaded),
    star1(false),
    star2(false),
    star3(false)
{
    setMinimumSize( 105, 24 );
    
    starPixmap = KIcon("favorites").pixmap( QSize(22,22) );
    disabledStarPixmap = KIcon("favorites").pixmap( QSize(22,22), QIcon::Disabled );
    
    errorPixmap = KIcon("dialog-error").pixmap( QSize(22,22) );
    loadedPixmap = KIcon("dialog-ok-apply").pixmap( QSize(22,22) );
    loadingPixmap = KIcon("chronometer").pixmap( QSize(22,22) ); //user-away
}

void UsefulnessMeter::setUsefulness( BacktraceInfo::Usefulness usefulness )
{
    switch( usefulness )
    {
        case BacktraceInfo::ReallyUseful:
        {
            star1 = true; 
            star2 = true; 
            star3 = true; 
            break;
        }
        case BacktraceInfo::MayBeUseful:
        {
            star1 = true; 
            star2 = true; 
            star3 = false; 
            break;
        }
        case BacktraceInfo::ProbablyUnuseful:
        {
            star1 = true; 
            star2 = false; 
            star3 = false; 
            break;
        }
        case BacktraceInfo::Unuseful:
        {
            star1 = false; 
            star2 = false; 
            star3 = false; 
            break;
        }
    }
    
    update();
}

void UsefulnessMeter::paintEvent( QPaintEvent * event )
{
    Q_UNUSED( event );
    
    QPainter p(this);

    p.drawPixmap( QPoint(30,1) , star1 ? starPixmap: disabledStarPixmap );
    p.drawPixmap( QPoint(55,1) , star2 ? starPixmap: disabledStarPixmap );
    p.drawPixmap( QPoint(80,1) , star3 ? starPixmap: disabledStarPixmap );

    switch( state )
    {
        case CrashInfo::Loaded:
        {
            if( star1 && star2 && star3 )
                p.drawPixmap( QPoint(0,1) , loadedPixmap );
            break;
        }
        case CrashInfo::Failed:
        case CrashInfo::DebuggerFailed:
        {
            p.drawPixmap( QPoint(0,1) , errorPixmap );
            break;
        }
        case CrashInfo::Loading:
        {
            p.drawPixmap( QPoint(0,1) , loadingPixmap );
        }
        default: 
            break;
    }
    
    p.end();
}