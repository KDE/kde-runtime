/*******************************************************************
* reportassistantpages_base.h
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

#ifndef REPORTASSISTANTPAGES__H
#define REPORTASSISTANTPAGES__H

#include "backtracewidget.h"
#include "reportassistantdialog.h"

#include "ui_assistantpage_bugawareness.h"
#include "ui_assistantpage_conclusions.h"
#include "ui_assistantpage_conclusions_dialog.h"

#include <QtCore/QPointer>

/** BASE interface which implements some signals, and
**  aboutTo(Show|Hide) functions (also reimplements QWizard behaviour) **/
class ReportAssistantPage: public QWidget
{
    Q_OBJECT

public:
    ReportAssistantPage(ReportAssistantDialog * parent) :
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
    ReportAssistantDialog * assistant() const {
        return m_assistant;
    }

public Q_SLOTS:
    void emitCompleteChanged() {
        emit completeChanged(this, isComplete());
    }

Q_SIGNALS:
    /** Tells the KAssistantDialog that the isComplete function changed value **/
    void completeChanged(ReportAssistantPage*, bool);

private:
    ReportAssistantDialog * const m_assistant;
};

/** Backtrace page **/
class CrashInformationPage: public ReportAssistantPage
{
    Q_OBJECT

public:
    CrashInformationPage(ReportAssistantDialog *);

    void aboutToShow();
    void aboutToHide();
    bool isComplete();
    bool showNextPage();

private:
    BacktraceWidget *        m_backtraceWidget;
};

/** Bug Awareness page **/
class BugAwarenessPage: public ReportAssistantPage
{
    Q_OBJECT

public:
    BugAwarenessPage(ReportAssistantDialog *);

    void aboutToHide();

private:
    Ui::AssistantPageBugAwareness   ui;
};

/** Conclusions page **/
class ConclusionPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    ConclusionPage(ReportAssistantDialog *);
    
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
