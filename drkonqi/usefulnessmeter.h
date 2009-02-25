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

#include "backtraceparser.h"
#include "crashinfo.h"

class QPixmap;

class UsefulnessMeter: public QWidget
{
    Q_OBJECT
    
    public:
        
        UsefulnessMeter(QWidget *);
        void setUsefulness( BacktraceParser::Usefulness );
        void setState( CrashInfo::BacktraceGenState s ) { m_state = s; update(); }
        
    protected:
        
        void paintEvent ( QPaintEvent * event );
        
    private:
    
        CrashInfo::BacktraceGenState    m_state;
        
        bool                            m_star1;
        bool                            m_star2;
        bool                            m_star3;
        
        QPixmap                         m_errorPixmap;
        QPixmap                         m_loadedPixmap;
        QPixmap                         m_loadingPixmap;
        
        QPixmap                         m_starPixmap;
        QPixmap                         m_disabledStarPixmap;
};

#endif
