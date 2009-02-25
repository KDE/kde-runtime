/*******************************************************************
* drkonqidialog.h
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

#ifndef DRKONQIDIALOG__H
#define DRKONQIDIALOG__H

#include <kdialog.h>

class QStackedWidget;
class KrashConfig;
class CrashInfo;
class GetBacktraceWidget;
class KPushButton;
class AboutBugReportingDialog;

class DrKonqiDialog: public KDialog
{
    Q_OBJECT
    
    public:
        explicit DrKonqiDialog(KrashConfig * conf, QWidget * parent = 0);
        ~DrKonqiDialog();
    
    private Q_SLOTS:
    
        void aboutBugReporting();
        void toggleBacktrace();
        void reportBugAssistant();
        
        void fin(bool);
    private:
    
        AboutBugReportingDialog *       m_aboutBugReportingDialog;
        
        QStackedWidget *                m_stackedWidget;
        QWidget *                       m_introWidget;
        GetBacktraceWidget *            m_backtraceWidget;
        
        KPushButton *                   m_aboutBugReportingButton;
        CrashInfo *                     m_crashInfo;
    
};

#endif
