/*******************************************************************
* reportassistantpages_bugzilla_duplicates.h
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

#ifndef REPORTASSISTANTPAGES__BUGZILLA__DUPLICATES_H
#define REPORTASSISTANTPAGES__BUGZILLA__DUPLICATES_H

#include "reportassistantpage.h"

#include "reportassistantpages_base.h"
#include "bugzillalib.h"

#include "ui_assistantpage_bugzilla_duplicates.h"
#include "ui_assistantpage_bugzilla_duplicates_dialog.h"

class QDate;
class QTreeWidgetItem;

class KGuiItem;

class BugzillaReportInformationDialog;

/** Searching for duplicates and showing report information page**/
class BugzillaDuplicatesPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    BugzillaDuplicatesPage(ReportAssistantDialog *);
    ~BugzillaDuplicatesPage();

    void aboutToShow();
    void aboutToHide();

    bool isComplete();
    bool showNextPage();

private Q_SLOTS:
    /* Search related methods */
    void searchMore();
    void performSearch();
    void stopCurrentSearch();

    void markAsSearching(bool);
    
    bool canSearchMore();

    void searchFinished(const BugMapList&);
    void searchError(QString);

    void resetDates();
    
    /* Duplicates list related methods */
    void openSelectedReport();
    void itemClicked(QTreeWidgetItem *, int);
    void itemClicked(QListWidgetItem *);
    void showReportInformationDialog(int);
    void itemSelectionChanged();

    /* Selected duplicates list related methods */
    void addPossibleDuplicateNumber(int);
    void removeSelectedDuplicate();

    void showDuplicatesPanel(bool);

    void possibleDuplicateSelectionChanged();

    /* Attach to bug related methods */
    void attachToBugReport(int);
    void cancelAttachToBugReport();

private:
    bool                                        m_searching;

    Ui::AssistantPageBugzillaDuplicates         ui;
    
    //Dates of current Results
    QDate                                       m_startDate;
    QDate                                       m_endDate;
    //Dates of searching process
    QDate                                       m_searchingStartDate;
    QDate                                       m_searchingEndDate;
    
    KGuiItem                                    m_searchMoreGuiItem;
    KGuiItem                                    m_retrySearchGuiItem;
};

/** Internal bug-info dialog **/
class BugzillaReportInformationDialog : public KDialog
{
    Q_OBJECT

public:
    BugzillaReportInformationDialog(BugzillaDuplicatesPage*parent=0);
    ~BugzillaReportInformationDialog();
    
    void showBugReport(int bugNumber);

private Q_SLOTS:
    void bugFetchFinished(BugReport,QObject *);
    void bugFetchError(QString, QObject *);

    void reloadReport();

    int promptAboutAlreadyClosedReport();
    void mayBeDuplicateClicked();
    void attachToBugReportClicked();

    void toggleShowOwnBacktrace(bool);
    
Q_SIGNALS:
    void possibleDuplicateSelected(int);
    void attachToBugReportSelected(int);

private:
    Ui::AssistantPageBugzillaDuplicatesDialog   ui;
    BugzillaDuplicatesPage *                    m_parent;
    int                                         m_bugNumber;
    QString                                     m_closedStateString;

    int                                         m_duplicatesCount;
};

#endif
