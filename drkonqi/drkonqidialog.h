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
class GetBacktraceWidget;
class KPushButton;
class AboutBugReportingDialog;
class DrKonqiBugReport;
class KGuiItem;
class KrashConfig;

class DrKonqiDialog: public KDialog
{
    Q_OBJECT
    
    public:
        explicit DrKonqiDialog( QWidget * parent = 0);
        ~DrKonqiDialog();
    
    private Q_SLOTS:
        void aboutBugReporting();
        void toggleMode();
        void reportBugAssistant();
        
        void restartApplication();
        
        //New debugger detected
        void slotNewDebuggingApp( const QString & );
        
        //GUI
        void buildMainWidget();
        void buildAdvancedWidget();
        void buildDialogOptions();
        
    private:
        AboutBugReportingDialog *       m_aboutBugReportingDialog;
        
        QStackedWidget *                m_stackedWidget;
        QWidget *                       m_introWidget;
        QWidget *                       m_advancedWidget;
        GetBacktraceWidget *            m_backtraceWidget;
        
        KPushButton *                   m_aboutBugReportingButton;
        KPushButton *                   m_reportBugButton;

        KGuiItem                        m_guiItemShowIntroduction;
        KGuiItem                        m_guiItemShowAdvanced;
        
        QMenu *                         m_debugMenu;
        QAction *                       m_defaultDebugAction;
        QAction *                       m_customDebugAction;
};

#endif
