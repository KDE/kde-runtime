/*******************************************************************
* usefulnessmeter.h
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

#ifndef USEFULNESSMETER__H
#define USEFULNESSMETER__H

#include <QtGui/QWidget>

#include "backtraceinfo.h"
#include "crashinfo.h"

class QPixmap;

class UsefulnessMeter: public QWidget
{
    Q_OBJECT
    
    public:
        
        UsefulnessMeter(QWidget *);
        void setUsefulness( BacktraceInfo::Usefulness );
        void setState( CrashInfo::BacktraceGenState s ) { state = s; update(); }
        
    protected:
        
        void paintEvent ( QPaintEvent * event );
        
    private:
    
        CrashInfo::BacktraceGenState state;
        
        bool star1;
        bool star2;
        bool star3;
        
        QPixmap errorPixmap;
        QPixmap loadedPixmap;
        QPixmap loadingPixmap;
        
        QPixmap starPixmap;
        QPixmap disabledStarPixmap;
};

#endif
