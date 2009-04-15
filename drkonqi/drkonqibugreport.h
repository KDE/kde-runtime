/*******************************************************************
* drkonqibugreport.h
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

#ifndef DRKONQIBUGREPORT__H
#define DRKONQIBUGREPORT__H

#include <KAssistantDialog>

class DrKonqiAssistantPage;
class AboutBugReportingDialog;
class ReportInfo;
class QCloseEvent;

class DrKonqiBugReport: public KAssistantDialog
{
    Q_OBJECT

public:
    explicit DrKonqiBugReport(QWidget * parent = 0);
    ~DrKonqiBugReport();

    ReportInfo *reportInfo() const {
        return m_reportInfo;
    }

private Q_SLOTS:
    void currentPageChanged_slot(KPageWidgetItem *, KPageWidgetItem *);

    void completeChanged(DrKonqiAssistantPage*, bool);
    void assistantFinished(bool);

    void enableNextButton(bool);
    void enableBackButton(bool);

    void showHelp();

    void next();

    //Override default reject method
    void reject();
private:
    //void
    void connectSignals(DrKonqiAssistantPage *);
    void closeEvent(QCloseEvent*);

    AboutBugReportingDialog *   m_aboutBugReportingDialog;
    ReportInfo *                m_reportInfo;

    bool                        m_canClose;
    
    //KGuiItem                    m_finishButtonItem;
    //KGuiItem                    m_closeButtonItem;
};

#endif
