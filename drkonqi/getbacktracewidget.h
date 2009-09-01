/*******************************************************************
* getbacktracewidget.h
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

#ifndef GETBACKTRACEWIDGET__H
#define GETBACKTRACEWIDGET__H

#include <QtGui/QWidget>

#include "debugpackageinstaller.h"
#include "ui_getbacktracewidget.h"

class KTextBrowser;
class QLabel;
class KPushButton;
class UsefulnessMeter;
class BacktraceGenerator;

class GetBacktraceWidget: public QWidget
{
    Q_OBJECT

public:
    explicit GetBacktraceWidget(BacktraceGenerator *generator, QWidget *parent = 0);

public Q_SLOTS:
    void generateBacktrace();
    void hilightExtraDetailsLabel(bool hilight);
    void focusImproveBacktraceButton();

Q_SIGNALS:
    void stateChanged();

private:
    BacktraceGenerator * m_btGenerator;
    Ui::Form    ui;
    UsefulnessMeter *   m_usefulnessMeter;

    DebugPackageInstaller * m_debugPackageInstaller;
    
private Q_SLOTS:
    void loadData();
    void backtraceNewLine(const QString &);

    void setAsLoading();
    void regenerateBacktrace();

    void saveClicked();
    void copyClicked();

    void anotherDebuggerRunning();
    
    void installDebugPackages();
    void debugPackageError(const QString &);
    void debugPackageCanceled();
};

#endif
