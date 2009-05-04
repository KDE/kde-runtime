/*******************************************************************
* drkonqiassistantpages_base.h
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

#ifndef DRKONQIASSISTANTPAGES__H
#define DRKONQIASSISTANTPAGES__H

#include "getbacktracewidget.h"
#include "drkonqibugreport.h"

#include "ui_assistantpage_introduction.h"
#include "ui_assistantpage_bugawareness.h"
#include "ui_assistantpage_conclusions.h"
#include "ui_assistantpage_conclusions_dialog.h"

#include <QtCore/QPointer>

/** BASE interface which implements some signals, and
**  aboutTo(Show|Hide) functions (also reimplements QWizard behaviour) **/
class DrKonqiAssistantPage: public QWidget
{
    Q_OBJECT

public:
    DrKonqiAssistantPage(DrKonqiBugReport * parent) :
            QWidget(parent), m_assistant(parent) {}

    /** Load the widget data if empty **/
    virtual void aboutToShow() {}
    /** Save the widget data to ReportInfo **/
    virtual void aboutToHide() {}
    /** Tells the KAssistantDialog to enable the Next button **/
    virtual bool isComplete() {
        return true;
    }
    /** Last time checks to see if you can turn the page **/
    virtual bool showNextPage() {
        return true;
    }
    
    ReportInfo * reportInfo() const {
        return m_assistant->reportInfo();
    }
    BugzillaManager *bugzillaManager() const {
        return m_assistant->bugzillaManager();
    }
    DrKonqiBugReport * assistant() const {
        return m_assistant;
    }

public Q_SLOTS:
    void emitCompleteChanged() {
        emit completeChanged(this, isComplete());
    }

Q_SIGNALS:
    /** Tells the KAssistantDialog that the isComplete function changed value **/
    void completeChanged(DrKonqiAssistantPage*, bool);

private:
    DrKonqiBugReport * const m_assistant;
};

/** Dummy Introduction page **/
class IntroductionPage: public DrKonqiAssistantPage
{
    Q_OBJECT

public:
    IntroductionPage(DrKonqiBugReport *);
    
private:
    Ui::AssistantPageIntroduction   ui;
};

/** Backtrace page **/
class CrashInformationPage: public DrKonqiAssistantPage
{
    Q_OBJECT

public:
    CrashInformationPage(DrKonqiBugReport *);

    void aboutToShow();
    void aboutToHide();
    bool isComplete();
    bool showNextPage();

private:
    GetBacktraceWidget *        m_backtraceWidget;
};

/** Bug Awareness page **/
class BugAwarenessPage: public DrKonqiAssistantPage
{
    Q_OBJECT

public:
    BugAwarenessPage(DrKonqiBugReport *);

    void aboutToHide();

private:
    Ui::AssistantPageBugAwareness   ui;
};

/** Conclusions page **/
class ConclusionPage : public DrKonqiAssistantPage
{
    Q_OBJECT

public:
    ConclusionPage(DrKonqiBugReport *);
    
    void aboutToShow();
    void aboutToHide();

    bool isComplete();

private Q_SLOTS:
    void finishClicked();
    
    void openReportInformation();

private:
    Ui::AssistantPageConclusions            ui;
    
    QPointer<KDialog>                       m_infoDialog;
    
    bool                                    isBKO;
    bool                                    needToReport;

Q_SIGNALS:
    void finished(bool);
};

class ReportInformationDialog : public KDialog
{
    Q_OBJECT
public:
    ReportInformationDialog(const QString & reportText);
    ~ReportInformationDialog();

private Q_SLOTS:
    void saveReport();

private:
    Ui::AssistantPageConclusionsDialog ui;
};

#endif
