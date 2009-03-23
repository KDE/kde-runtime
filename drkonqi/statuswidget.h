/*******************************************************************
* statuswidget.h
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
#ifndef STATUSWIDGET__H
#define STATUSWIDGET__H

#include <QtGui/QStackedWidget>

class QLabel;
class QProgressBar;

class StatusWidget: public QStackedWidget
{
    Q_OBJECT
    public:
        StatusWidget( QWidget * parent = 0);
        
        void setBusy( QString );
        void setIdle( QString );

        void addCustomStatusWidget( QWidget * );
        
        void setStatusLabelWordWrap( bool );
        
    private:
        void setBusyCursor();
        void setIdleCursor();
        
        QLabel *            m_statusLabel;
        
        QProgressBar *      m_progressBar;
        QLabel *            m_busyLabel;
        
        QWidget *           m_statusPage;
        QWidget *           m_busyPage;
};

#endif
