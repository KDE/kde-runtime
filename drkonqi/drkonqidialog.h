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

#include <QtCore/QPointer>
#include <QtCore/QHash>
#include <KDialog>

#include "ui_maindialog.h"

class BacktraceWidget;
class AboutBugReportingDialog;
class KTabWidget;
class AbstractDebuggerLauncher;
class KMenu;

class DrKonqiDialog: public KDialog
{
    Q_OBJECT

public:
    explicit DrKonqiDialog(QWidget * parent = 0);
    ~DrKonqiDialog();

private Q_SLOTS:
    void aboutBugReporting();
    void reportBugAssistant();

    void applicationRestarted();

    void addDebugger(AbstractDebuggerLauncher *launcher);
    void removeDebugger(AbstractDebuggerLauncher *launcher);
    void enableDebugMenu(bool);

    //GUI
    void buildMainWidget();
    void buildDialogOptions();

    void tabIndexChanged(int);

private:
    KTabWidget *                        m_tabWidget;

    QPointer<AboutBugReportingDialog>   m_aboutBugReportingDialog;

    QWidget *                           m_introWidget;
    Ui::MainWidget                      ui;
    
    BacktraceWidget *                   m_backtraceWidget;

    KMenu *m_debugMenu;
    QHash<AbstractDebuggerLauncher*, QAction*> m_debugMenuActions;
};

#endif
