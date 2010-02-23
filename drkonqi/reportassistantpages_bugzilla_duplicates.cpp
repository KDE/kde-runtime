/*******************************************************************
* reportassistantpages_bugzilla_duplicates.cpp
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

#include "reportassistantpages_bugzilla_duplicates.h"

#include "drkonqi_globals.h"

#include "reportinterface.h"
#include "statuswidget.h"

#include <QtCore/QDate>
#include <QtCore/QTimer>

#include <QtGui/QLabel>
#include <QtGui/QTreeWidgetItem>
#include <QtGui/QHeaderView>

#include <KIcon>
#include <KMessageBox>
#include <KInputDialog>

//BEGIN BugzillaDuplicatesPage

BugzillaDuplicatesPage::BugzillaDuplicatesPage(ReportAssistantDialog * parent):
        ReportAssistantPage(parent),
        m_searching(false)
{
    resetDates();

    connect(bugzillaManager(), SIGNAL(searchFinished(BugMapList)),
             this, SLOT(searchFinished(BugMapList)));
    connect(bugzillaManager(), SIGNAL(searchError(QString)),
             this, SLOT(searchError(QString)));

    ui.setupUi(this);

    connect(ui.m_bugListWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
             this, SLOT(itemClicked(QTreeWidgetItem*, int)));
    connect(ui.m_bugListWidget, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));

    QHeaderView * header = ui.m_bugListWidget->header();
    header->setResizeMode(0, QHeaderView::ResizeToContents);
    header->setResizeMode(1, QHeaderView::Interactive);

    QTreeWidgetItem * customBugItem = new QTreeWidgetItem(
        QStringList() << i18nc("@item:intable custom bug report number", "Custom")
                      << i18nc("@item:intable custom bug report number description", 
                                                            "Manually enter a bug report ID"));
    customBugItem->setData(0, Qt::UserRole, QLatin1String("custom"));
    customBugItem->setIcon(1, KIcon("edit-rename"));

    ui.m_bugListWidget->addTopLevelItem(customBugItem);

    m_searchMoreGuiItem = KGuiItem2(i18nc("@action:button", "Search for more reports"),
                                                   KIcon("edit-find"),
                                                   i18nc("@info:tooltip", "Use this button to "
                                                        "search for more similar bug reports on an "
                                                        "earlier date."));
    ui.m_searchMoreButton->setGuiItem(m_searchMoreGuiItem);
    connect(ui.m_searchMoreButton, SIGNAL(clicked()), this, SLOT(searchMore()));
    
    m_retrySearchGuiItem = KGuiItem2(i18nc("@action:button", "Retry search"),
                                                   KIcon("edit-find"),
                                                   i18nc("@info:tooltip", "Use this button to "
                                                        "retry the search that previously "
                                                        "failed."));

    ui.m_openReportButton->setGuiItem(KGuiItem2(i18nc("@action:button", "Open selected report"),
                                                   KIcon("document-preview"),
                                                   i18nc("@info:tooltip", "Use this button to view "
                                                   "the information of the selected bug report.")));
    connect(ui.m_openReportButton, SIGNAL(clicked()), this, SLOT(openSelectedReport()));

    ui.m_stopSearchButton->setGuiItem(KGuiItem2(i18nc("@action:button", "Stop searching"),
                                                   KIcon("process-stop"),
                                                   i18nc("@info:tooltip", "Use this button to stop "
                                                   "the current search.")));
    ui.m_stopSearchButton->setText(QString()); //FIXME 
    connect(ui.m_stopSearchButton, SIGNAL(clicked()), this, SLOT(stopCurrentSearch()));
    
    //Possible duplicates list and buttons
    connect(ui.m_selectedDuplicatesList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
             this, SLOT(itemClicked(QListWidgetItem*)));
    connect(ui.m_selectedDuplicatesList, SIGNAL(itemSelectionChanged()),
             this, SLOT(possibleDuplicateSelectionChanged()));
             
    ui.m_removeSelectedDuplicateButton->setGuiItem(
        KGuiItem2(i18nc("@action:button remove the selected item from a list", "Remove"),
               KIcon("list-remove"),
               i18nc("@info:tooltip", "Use this button to remove a selected possible duplicate")));
    ui.m_removeSelectedDuplicateButton->setEnabled(false);
    connect(ui.m_removeSelectedDuplicateButton, SIGNAL(clicked()), this, 
                                                                SLOT(removeSelectedDuplicate()));

    ui.m_attachToReportIcon->setPixmap(KIcon("mail-attachment").pixmap(16,16));
    ui.m_attachToReportIcon->setFixedSize(16,16);
    ui.m_attachToReportIcon->setVisible(false);
    ui.m_attachToReportLabel->setVisible(false);
    connect(ui.m_attachToReportLabel, SIGNAL(linkActivated(QString)), this, 
                                                                SLOT(cancelAttachToBugReport()));
    showDuplicatesPanel(false);
}

BugzillaDuplicatesPage::~BugzillaDuplicatesPage()
{
}

void BugzillaDuplicatesPage::aboutToShow()
{
    //Perform initial search if we are not currently searching and if there are no results yet
    if (!m_searching && ui.m_bugListWidget->topLevelItemCount() == 1 && canSearchMore()) {
        searchMore();
    }
}

void BugzillaDuplicatesPage::aboutToHide()
{
    stopCurrentSearch();

    //Save selected possible duplicates by user
    QStringList possibleDuplicates;
    int count = ui.m_selectedDuplicatesList->count();
    for(int i = 0; i<count; i++) {
        possibleDuplicates << ui.m_selectedDuplicatesList->item(i)->text();
    }
    reportInterface()->setPossibleDuplicates(possibleDuplicates);

    //Save possible duplicates by query
    QStringList duplicatesByQuery;
    count = ui.m_bugListWidget->topLevelItemCount();
    for(int i = 1; i<count; i++) {
        duplicatesByQuery << ui.m_bugListWidget->topLevelItem(i)->text(0);
    }
    reportInterface()->setPossibleDuplicatesByQuery(duplicatesByQuery);
}

bool BugzillaDuplicatesPage::isComplete()
{
    return !m_searching;
}

bool BugzillaDuplicatesPage::showNextPage()
{
    //Ask the user to check all the possible duplicates...
    if (ui.m_bugListWidget->topLevelItemCount() != 1 && ui.m_selectedDuplicatesList->count() == 0
        && reportInterface()->attachToBugNumber() == 0) {
        //The user didn't selected any possible duplicate nor a report to attach the new info.
        //Double check this, we need to reduce the duplicate count.
        KGuiItem noDuplicatesButton;
        noDuplicatesButton.setText("There are no real duplicates");
        noDuplicatesButton.setIcon(KIcon("dialog-cancel"));

        KGuiItem letMeCheckMoreReportsButton;
        letMeCheckMoreReportsButton.setText("Let me check more reports");
        letMeCheckMoreReportsButton.setIcon(KIcon("document-preview"));

        if (KMessageBox::questionYesNo(this,
           i18nc("@info","You have not selected any possible duplicates, or a report to which to attach your "
           "crash information. Have you read all the reports, and can you confirm that there are no "
           "real duplicates?"),
           i18nc("@title:window","No selected possible duplicates"), letMeCheckMoreReportsButton,
                               noDuplicatesButton)
                                        == KMessageBox::Yes) {
            return false;
        }
    }
    return true;
}

//BEGIN Search related methods
void BugzillaDuplicatesPage::searchMore()
{
    //1 year back
    m_searchingEndDate = m_startDate;
    m_searchingStartDate = m_searchingEndDate.addYears(-1);

    performSearch();
}

void BugzillaDuplicatesPage::performSearch()
{
    markAsSearching(true);

    QString startDateStr = m_searchingStartDate.toString("yyyy-MM-dd");
    QString endDateStr = m_searchingEndDate.toString("yyyy-MM-dd");

    ui.m_statusWidget->setBusy(i18nc("@info:status","Searching for duplicates (from %1 to %2)...",
                                   startDateStr, endDateStr));
                           
    //Bugzilla will not search on Today bugs if we send the date.
    //we need to send "Now"
    if (m_searchingEndDate == QDate::currentDate()) {
        endDateStr = QLatin1String("Now");
    }

#if true
    BugReport report = reportInterface()->newBugReportTemplate();
    bugzillaManager()->searchBugs(reportInterface()->relatedBugzillaProducts(),
                                  report.bugSeverity(), startDateStr, endDateStr, 
                                  reportInterface()->firstBacktraceFunctions().join(" "));
#else //Test search
    bugzillaManager()->searchBugs(QStringList() << "plasma", "crash", startDateStr, endDateStr,
       "QGraphicsScenePrivate::processDirtyItemsRecursive");
#endif
}

void BugzillaDuplicatesPage::stopCurrentSearch()
{
    if (m_searching) {
        bugzillaManager()->stopCurrentSearch(); 
        
        markAsSearching(false);
        
        if (m_startDate==m_endDate) { //Never searched
            ui.m_statusWidget->setIdle(i18nc("@info:status","Search stopped."));
        } else {
            ui.m_statusWidget->setIdle(i18nc("@info:status","Search stopped. Showing results from "
                                        "%1 to %2", m_startDate.toString("yyyy-MM-dd"),
                                        m_endDate.toString("yyyy-MM-dd")));
        }
    }
}

void BugzillaDuplicatesPage::markAsSearching(bool searching)
{
    m_searching = searching;
    emitCompleteChanged();
    
    ui.m_bugListWidget->setEnabled(!searching);
    ui.m_searchMoreButton->setEnabled(!searching);
    ui.m_searchMoreButton->setVisible(!searching);
    ui.m_stopSearchButton->setEnabled(searching);
    ui.m_stopSearchButton->setVisible(searching);
    
    ui.m_selectedDuplicatesList->setEnabled(!searching);
    ui.m_selectedPossibleDuplicatesLabel->setEnabled(!searching);
    ui.m_removeSelectedDuplicateButton->setEnabled(!searching && 
                                        !ui.m_selectedDuplicatesList->selectedItems().isEmpty());

    ui.m_attachToReportLabel->setEnabled(!searching);

    if (!searching) {
        itemSelectionChanged();
    } else {
        ui.m_openReportButton->setEnabled(false);
    }
}

bool BugzillaDuplicatesPage::canSearchMore()
{
    return (m_startDate.year() >= 2007);
}

void BugzillaDuplicatesPage::searchFinished(const BugMapList & list)
{
    ui.m_searchMoreButton->setGuiItem(m_searchMoreGuiItem);
    m_startDate = m_searchingStartDate;
    
    int results = list.count();
    if (results > 0) {
        markAsSearching(false);
        
        ui.m_statusWidget->setIdle(i18nc("@info:status","Showing results from %1 to %2",
                                     m_startDate.toString("yyyy-MM-dd"),
                                     m_endDate.toString("yyyy-MM-dd")));

        for (int i = 0; i < results; i++) {
            BugMap bug = list.at(i);

            QString title;

            //Generate a non-geek readable status
            QString customStatusString;
            if (bug.value("bug_status") == QLatin1String("UNCONFIRMED")
                || bug.value("bug_status") == QLatin1String("NEW")
                || bug.value("bug_status") == QLatin1String("REOPENED")
                || bug.value("bug_status") == QLatin1String("ASSIGNED")) {
                customStatusString = i18nc("@info/plain bug status", "[Open]");
            } else if (bug.value("bug_status") == QLatin1String("RESOLVED")
                || bug.value("bug_status") == QLatin1String("VERIFIED")
                || bug.value("bug_status") == QLatin1String("CLOSED")) {
                if (bug.value("resolution") == QLatin1String("FIXED")) {
                    customStatusString = i18nc("@info/plain bug resolution", "[Fixed]");
                } else if (bug.value("resolution") == QLatin1String("WORKSFORME")) {
                    customStatusString = i18nc("@info/plain bug resolution", "[Non-reproducible]");
                } else if (bug.value("resolution") == QLatin1String("DUPLICATE")) {
                    customStatusString = i18nc("@info/plain bug resolution", "[Duplicate report]");
                } else if (bug.value("resolution") == QLatin1String("INVALID")) {
                    customStatusString = i18nc("@info/plain bug resolution", "[Invalid]");
                } else if (bug.value("resolution") == QLatin1String("DOWNSTREAM")
                    || bug.value("resolution") == QLatin1String("UPSTREAM")) {
                    customStatusString = i18nc("@info/plain bug resolution", "[External problem]");
                }
            } else if (bug.value("bug_status") == QLatin1String("NEEDSINFO")) {
                customStatusString = i18nc("@info/plain bug status", "[Incomplete]");
            }

            title = customStatusString + ' ' + bug["short_desc"];

            QStringList fields = QStringList() << bug["bug_id"] << title;
            ui.m_bugListWidget->addTopLevelItem(new QTreeWidgetItem(fields));
        }

        ui.m_bugListWidget->sortItems(0 , Qt::DescendingOrder);
        ui.m_bugListWidget->resizeColumnToContents(1);

        if (!canSearchMore()) {
            ui.m_searchMoreButton->setEnabled(false);
        }

    } else {

        if (canSearchMore()) {
            //We don't call markAsSearching(false) to avoid flicker
            //Delayed call to searchMore to avoid unexpected behaviour (signal/slot)
            //because we are in a slot, and searchMore() will be ending calling this slot again
            QTimer::singleShot(0, this, SLOT(searchMore()));
        } else {
            markAsSearching(false);
            ui.m_statusWidget->setIdle(i18nc("@info:status","Search Finished. "
                                                         "No reports found."));
            ui.m_searchMoreButton->setEnabled(false);
            if (ui.m_bugListWidget->topLevelItemCount() == 0) {
                //No reports to mark as possible duplicate
                ui.m_selectedDuplicatesList->setEnabled(false);
            }
        }
    }
}

void BugzillaDuplicatesPage::searchError(QString err)
{
    ui.m_searchMoreButton->setGuiItem(m_retrySearchGuiItem);
    markAsSearching(false);

    ui.m_statusWidget->setIdle(i18nc("@info:status","Error fetching the bug report list"));

    KMessageBox::error(this , i18nc("@info/rich","Error fetching the bug report list<nl/>"
                                                 "<message>%1.</message><nl/>"
                                                 "Please wait some time and try again.", err));
}

void BugzillaDuplicatesPage::resetDates()
{
    m_endDate = QDate::currentDate();
    m_startDate = m_endDate;
}
//END Search related methods

//BEGIN Duplicates list related methods
void BugzillaDuplicatesPage::openSelectedReport()
{
    QList<QTreeWidgetItem*> selected = ui.m_bugListWidget->selectedItems();
    if (selected.count() == 1) {
        itemClicked(selected.at(0), 0);
    }
}

void BugzillaDuplicatesPage::itemClicked(QTreeWidgetItem * item, int col)
{
    Q_UNUSED(col);

    int bugNumber = 0;
    if (item->data(0, Qt::UserRole) == QLatin1String("custom")) {
        bool ok = false;
        bugNumber = KInputDialog::getInteger(
                    i18nc("@title:window", "Enter a custom bug report number"),
                    i18nc("@label", "Enter the number of the bug report you want to check"),
                    0, 0, 1000000, 1, &ok, this);
    } else {
        bugNumber = item->text(0).toInt();
    }
    showReportInformationDialog(bugNumber);
}

void BugzillaDuplicatesPage::itemClicked(QListWidgetItem * item)
{
    showReportInformationDialog(item->text().toInt());
}

void BugzillaDuplicatesPage::showReportInformationDialog(int bugNumber)
{
    if (bugNumber <= 0) {
        return;
    }

    BugzillaReportInformationDialog * infoDialog = new BugzillaReportInformationDialog(this);
    connect(infoDialog, SIGNAL(possibleDuplicateSelected(int)), this, 
                                                            SLOT(addPossibleDuplicateNumber(int)));
    connect(infoDialog, SIGNAL(attachToBugReportSelected(int)), this, SLOT(attachToBugReport(int)));

    infoDialog->showBugReport(bugNumber);
}

void BugzillaDuplicatesPage::itemSelectionChanged()
{
    ui.m_openReportButton->setEnabled(ui.m_bugListWidget->selectedItems().count() == 1);
}
//END Duplicates list related methods

//BEGIN Selected duplicates list related methods
void BugzillaDuplicatesPage::addPossibleDuplicateNumber(int bugNumber)
{
    QString stringNumber = QString::number(bugNumber);
    if (ui.m_selectedDuplicatesList->findItems(stringNumber, Qt::MatchExactly).isEmpty()) {
        ui.m_selectedDuplicatesList->addItem(stringNumber);
    }

    showDuplicatesPanel(true);
}

void BugzillaDuplicatesPage::removeSelectedDuplicate()
{
    QList<QListWidgetItem*> items = ui.m_selectedDuplicatesList->selectedItems();
    if (items.length() > 0) {
        delete ui.m_selectedDuplicatesList->takeItem(ui.m_selectedDuplicatesList->row(items.at(0)));
    }

    if (ui.m_selectedDuplicatesList->count() == 0) {
        showDuplicatesPanel(false);
    }
}

void BugzillaDuplicatesPage::showDuplicatesPanel(bool show)
{
    ui.m_removeSelectedDuplicateButton->setVisible(show);
    ui.m_selectedDuplicatesList->setVisible(show);
    ui.m_selectedPossibleDuplicatesLabel->setVisible(show);
}

void BugzillaDuplicatesPage::possibleDuplicateSelectionChanged()
{
    ui.m_removeSelectedDuplicateButton->setEnabled(
                                        !ui.m_selectedDuplicatesList->selectedItems().isEmpty());
}
//END Selected duplicates list related methods

//BEGIN Attach to bug related methods
void BugzillaDuplicatesPage::attachToBugReport(int bugNumber)
{
    ui.m_attachToReportLabel->setText(i18nc("@label", "The report is going to be "
                            "<strong>attached</strong> to bug <numid>%1</numid>. "
                            "<a href=\"#\">Cancel</a>", bugNumber));
    ui.m_attachToReportLabel->setVisible(true);
    ui.m_attachToReportIcon->setVisible(true);
    reportInterface()->setAttachToBugNumber(bugNumber);
}

void BugzillaDuplicatesPage::cancelAttachToBugReport()
{
    ui.m_attachToReportLabel->setVisible(false);
    ui.m_attachToReportIcon->setVisible(false);
    reportInterface()->setAttachToBugNumber(0);
}
//END Attach to bug related methods

//END BugzillaDuplicatesPage

//BEGIN BugzillaReportInformationDialog

BugzillaReportInformationDialog::BugzillaReportInformationDialog(BugzillaDuplicatesPage * parent) :
        KDialog(parent),
        m_parent(parent),
        m_bugNumber(0),
        m_duplicatesCount(0)
{
    //Create the GUI
    setButtons(KDialog::Close | KDialog::User1 | KDialog::User2);
    setDefaultButton(KDialog::Close);
    setCaption(i18nc("@title:window","Bug Description"));
    
    QWidget * widget = new QWidget(this);
    ui.setupUi(widget);
    setMainWidget(widget);

    ui.m_retryButton->setGuiItem(KGuiItem2(i18nc("@action:button", "Retry..."),
                                  KIcon("view-refresh"),
                                  i18nc("@info:tooltip", "Use this button to retry "
                                                  "loading the bug report.")));
    connect(ui.m_retryButton, SIGNAL(clicked()), this, SLOT(reloadReport()));
    
    setButtonGuiItem(KDialog::User2, 
                KGuiItem2(i18nc("@action:button", "Add as a possible duplicate"),
                    KIcon("list-add"), i18nc("@info:tooltip", "Use this button to mark your "
                    "crash as related to the currently shown bug report. This will help "
                    "the KDE developers to determine whether they are duplicates or not.")));
    connect(this, SIGNAL(user2Clicked()) , this, SLOT(mayBeDuplicateClicked()));

    setButtonGuiItem(KDialog::User1, 
                KGuiItem2(i18nc("@action:button", "Attach to this report (Advanced)"),
                    KIcon("mail-attachment"), i18nc("@info:tooltip", "Use this button to attach "
                    "your crash information to this report; only if you are really sure this is "
                    "the same crash.")));
    connect(this, SIGNAL(user1Clicked()) , this, SLOT(attachToBugReportClicked()));

    connect(ui.m_showOwnBacktraceCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleShowOwnBacktrace(bool)));

    //Connect bugzillalib signals
    connect(m_parent->bugzillaManager(), SIGNAL(bugReportFetched(BugReport, QObject *)),
             this, SLOT(bugFetchFinished(BugReport, QObject *)));
    connect(m_parent->bugzillaManager(), SIGNAL(bugReportError(QString, QObject *)),
             this, SLOT(bugFetchError(QString, QObject *)));
             
    setInitialSize(QSize(800, 600));
    KConfigGroup config(KGlobal::config(), "BugzillaReportInformationDialog");
    restoreDialogSize(config);
}

BugzillaReportInformationDialog::~BugzillaReportInformationDialog()
{
    disconnect(m_parent->bugzillaManager(), SIGNAL(bugReportFetched(BugReport, QObject *)),
             this, SLOT(bugFetchFinished(BugReport, QObject *)));
    disconnect(m_parent->bugzillaManager(), SIGNAL(bugReportError(QString, QObject *)),
             this, SLOT(bugFetchError(QString, QObject *)));

    KConfigGroup config(KGlobal::config(), "BugzillaReportInformationDialog");
    saveDialogSize(config);
}

void BugzillaReportInformationDialog::reloadReport()
{
    showBugReport(m_bugNumber);
}

void BugzillaReportInformationDialog::showBugReport(int bugNumber)
{
    ui.m_retryButton->setVisible(false);

    m_closedStateString.clear();
    m_bugNumber = bugNumber;
    m_parent->bugzillaManager()->fetchBugReport(m_bugNumber, this);

    button(KDialog::User1)->setEnabled(false);
    button(KDialog::User2)->setEnabled(false);

    ui.m_infoBrowser->setText(i18nc("@info:status","Loading..."));
    ui.m_infoBrowser->setEnabled(false);
    
    ui.m_linkLabel->setText(i18nc("@info","<link url='%1'>Report's webpage</link>",
                                    m_parent->bugzillaManager()->urlForBug(m_bugNumber)));

    ui.m_statusWidget->setBusy(i18nc("@info:status","Loading information about bug "
                                                           "<numid>%1</numid> from %2....",
                                            m_bugNumber,
                                            QLatin1String(KDE_BUGZILLA_SHORT_URL)));
                                            
    ui.m_backtraceBrowser->setPlainText(
                    i18nc("@info/plain","Backtrace of the crash I experienced:\n\n") +
                    m_parent->reportInterface()->backtrace());

    KConfigGroup config(KGlobal::config(), "BugzillaReportInformationDialog");
    bool showOwnBacktrace = config.readEntry("ShowOwnBacktrace", false);
    ui.m_showOwnBacktraceCheckBox->setChecked(showOwnBacktrace);
    if (!showOwnBacktrace) { //setChecked(false) will not emit toggled(false)
        toggleShowOwnBacktrace(false);
    }

    show();
}

void BugzillaReportInformationDialog::bugFetchFinished(BugReport report, QObject * jobOwner)
{
    if (jobOwner == this && isVisible()) {
        if (report.isValid()) {

            //Handle duplicate state
            QString duplicate = report.markedAsDuplicateOf();
            if (!duplicate.isEmpty()) {
                bool ok = false;
                int dupId = duplicate.toInt(&ok);
                if (ok && dupId > 0) {
                    ui.m_statusWidget->setIdle(QString());

                    KGuiItem yesItem = KStandardGuiItem::yes();
                    yesItem.setText(i18nc("@action:button let the user to choose to read the "
                    "main report", "Yes, read the main report"));

                    KGuiItem noItem = KStandardGuiItem::no();
                    noItem.setText(i18nc("@action:button let the user choose to read the original "
                    "report", "No, let me read the report I selected"));

                    if (KMessageBox::questionYesNo(this,
                       i18nc("@info","The report you selected (bug <numid>%1</numid>) is already "
                       "marked as duplicate of bug <numid>%2</numid>. "
                       "Do you want to read that report instead? (recommended)",
                       report.bugNumber(), dupId),
                       i18nc("@title:window","Nested duplicate detected"), yesItem, noItem)
                                                    == KMessageBox::Yes) {
                        showBugReport(dupId);
                        return;
                    }
                }
            }

            //Generate html for comments (with proper numbering)
            QString comments;
            QStringList commentList = report.comments();
            for (int i = 0; i < commentList.count(); i++) {
                QString comment = commentList.at(i);
                comment.replace('\n', "<br />");
                comments += i18nc("comment $number to use as subtitle", "<h4>Comment %1:</h4>", (i+1))
                                  + "<p>" + comment + "</p><hr />";
            }

            //Count duplicates (simple check)
            m_duplicatesCount = comments.count("has been marked as a duplicate of this bug.");

            //Generate a non-geek readable status
            QString customStatusString;
            if (report.bugStatus() == QLatin1String("UNCONFIRMED")) {
                customStatusString = i18nc("@info bug status", "Opened (Unconfirmed)");
            } else if (report.bugStatus() == QLatin1String("NEW")
                || report.bugStatus() == QLatin1String("REOPENED")
                || report.bugStatus() == QLatin1String("ASSIGNED")) {
                customStatusString = i18nc("@info bug status", "Opened (Unfixed)");
            } else if (report.bugStatus() == QLatin1String("RESOLVED")
                || report.bugStatus() == QLatin1String("VERIFIED")
                || report.bugStatus() == QLatin1String("CLOSED")) {
                QString customResolutionString;
                if (report.resolution() == QLatin1String("FIXED")) {
                    customResolutionString = i18nc("@info bug resolution", "Fixed");
                    m_closedStateString = i18nc("@info bug resolution", "the bug was fixed by KDE developers");
                } else if (report.resolution() == QLatin1String("WORKSFORME")) {
                    customResolutionString = i18nc("@info bug resolution", "Non-reproducible");
                } else if (report.resolution() == QLatin1String("DUPLICATE")) {
                    customResolutionString = i18nc("@info bug resolution", "Duplicate report "
                                                   "(Already reported before)");
                } else if (report.resolution() == QLatin1String("INVALID")) {
                    customResolutionString = i18nc("@info bug resolution", "Not a valid report/crash");
                } else if (report.resolution() == QLatin1String("DOWNSTREAM") 
                    || report.resolution() == QLatin1String("UPSTREAM")) {
                    customResolutionString = i18nc("@info bug resolution", "Not caused by a problem "
                                                "in the KDE's Applications or libraries");
                    m_closedStateString = i18nc("@info bug resolution", "the bug is caused by a "
                                                "problem in an external application or library, or "
                                                "by a distribution or packaging issue.");
                } else {
                    customResolutionString = report.resolution();
                }
                
                customStatusString = i18nc("@info bug status, %1 is the resolution", "Closed (%1)",
                                           customResolutionString);
            } else if (report.bugStatus() == QLatin1String("NEEDSINFO")) {
                customStatusString = i18nc("@info bug status", "Temporarily closed, because of a lack "
                                            "of information");
            } else { //Fallback to other raw values
                customStatusString = QString("%1 (%2)").arg(report.bugStatus(), report.resolution());
            }

            //Generate notes
            QString notes = i18n("<p><note>The bug report's title is oftenly written by its reporter"
                                 "and it may not reflect the bug nature, root cause or other visible"
                                 "symptoms you could use to compare to your crash. Please read the"
                                 "complete report and all the comments below.</note></p>");

            if (m_duplicatesCount > 10) { //Consider a possible mass duplicate crash
                notes += i18n("<p><note>This bug report has %1 duplicate reports. That means this "
                              "probably is a <strong>common crash</strong>. <i>Please consider only "
                              "adding a comment or a note if you can provide new valuable "
                              "information which wasn't already mentioned.</i></note></p>",
                              m_duplicatesCount);
            }

            QString text =
                i18nc("@info bug report title (quoted)",
                                "<h3>\"%1\"</h3>", report.shortDescription()) +
                notes +
                i18nc("@info bug report status",
                                "<h4>Bug Report Status: %1</h4>", customStatusString) +
                i18nc("@info bug report product and component",
                                "<h4>Affected Component: %1 (%2)</h4>",
                                report.product(), report.component()) +
                i18nc("@info bug report description",
                                "<h3>Description of the bug</h3><p>%1</p>",
                                report.description().replace('\n', "<br />"));

            if (!comments.isEmpty()) {
                text += i18nc("@label:textbox bug report comments (already formatted)", 
                                "<h2>Additional Comments</h2>%1", comments);
            }

            ui.m_infoBrowser->setText(text);
            ui.m_infoBrowser->setEnabled(true);
            
            button(KDialog::User1)->setEnabled(true);
            button(KDialog::User2)->setEnabled(true);

            ui.m_statusWidget->setIdle(i18nc("@info:status", "Showing bug <numid>%1</numid>",
                                                            report.bugNumberAsInt()));
        } else {
            bugFetchError(i18nc("@info", "Invalid report information (malformed data)"), this);
        }
    }
}

int BugzillaReportInformationDialog::promptAboutAlreadyClosedReport()
{
    KGuiItem sameBugDoNotReportButton;
    sameBugDoNotReportButton.setText("It is the same bug. Do not file my report (Close).");
    sameBugDoNotReportButton.setIcon(KIcon("edit-copy"));

    KGuiItem userIsNotSureProceedButton;
    userIsNotSureProceedButton.setText("I cannot be sure. Proceed with reporting the bug.");
    userIsNotSureProceedButton.setIcon(KIcon("go-next"));

    int ret = KMessageBox::questionYesNoCancel(this,
        i18nc("@info messagebox question. %1 is the state explanation","This report is marked as "
        "\"closed\" because %1. If you are sure your crash is the same, then adding further details "
        "or creating a new report will be useless and will waste developers' time. "
        "Can you be sure this is the same as your crash?", m_closedStateString),
        i18nc("@title:window", "This report is already closed"), sameBugDoNotReportButton,
                   userIsNotSureProceedButton, KStandardGuiItem::cancel(), QString(),
                                               KMessageBox::Dangerous | KMessageBox::Notify);

    return ret;
}

void BugzillaReportInformationDialog::warnAboutCommonCrashReport()
{
    KMessageBox::sorry(this,
        i18nc("@info","This bug report has %1 duplicate reports, which means it is probably a "
              "<strong>common crash</strong>, and a lot of different cases' details may have been "
              "provided already. <i>Please consider proceeding only if you can add new information "
              "which was not already mentioned.</i> Otherwise, we suggest you to cancel the bug "
              "reporting process.</note></p>", m_duplicatesCount),
                       i18nc("@title:window", "This is a common crash"));
}

void BugzillaReportInformationDialog::mayBeDuplicateClicked()
{
    if (!m_closedStateString.isEmpty()) {
        int ret = promptAboutAlreadyClosedReport();
        if (ret == KMessageBox::Yes) { // "Same bug report, close"
            //Ask to close the report assistant dialog
            hide();
            m_parent->assistant()->close();
            return;
        } else if (ret == KMessageBox::Cancel) {
            return;
        }
    }

    //Warn about possible common crashes
    if (m_duplicatesCount > 10) {
        warnAboutCommonCrashReport();
    }
    emit possibleDuplicateSelected(m_bugNumber);
    hide();
}

void BugzillaReportInformationDialog::attachToBugReportClicked()
{
    if (!m_closedStateString.isEmpty()) {
        int ret = promptAboutAlreadyClosedReport();
        if (ret == KMessageBox::Yes) { // "Same bug report, close"
            //Ask to close the report assistant dialog
            hide();
            m_parent->assistant()->close();
            return;
        } else if (ret == KMessageBox::Cancel) {
            return;
        }
    } else {
        if (KMessageBox::questionYesNo(this,
               i18nc("@info","If you want to attach new information to an existing bug report you need "
               "to be sure that they refer to the same crash.<nl />Are you sure you want to attach "
               "your report to bug <numid>%1</numid> ?", m_bugNumber),
               i18nc("@title:window","Attach the information to bug <numid>%1</numid>", m_bugNumber))
                                            == KMessageBox::No) {
            return;
        }
    }

    //Warn about possible common crashes
    if (m_duplicatesCount > 10) {
        warnAboutCommonCrashReport();
    }
 
    emit attachToBugReportSelected(m_bugNumber);
    hide();
}

void BugzillaReportInformationDialog::bugFetchError(QString err, QObject * jobOwner)
{
    if (jobOwner == this && isVisible()) {
        KMessageBox::error(this , i18nc("@info/rich","Error fetching the bug report<nl/>"
                                        "<message>%1.</message><nl/>"
                                        "Please wait some time and try again.", err));
        button(KDialog::User1)->setEnabled(false);
        button(KDialog::User2)->setEnabled(false);
        ui.m_infoBrowser->setText(i18nc("@info","Error fetching the bug report"));
        ui.m_statusWidget->setIdle(i18nc("@info:status","Error fetching the bug report"));
        ui.m_retryButton->setVisible(true);
    }
}

void BugzillaReportInformationDialog::toggleShowOwnBacktrace(bool show)
{
    QList<int> sizes;
    if (show) {
        int size = (ui.m_reportSplitter->sizeHint().width()-ui.m_reportSplitter->handleWidth())/2;
        sizes << size << size;
    } else {
        sizes << ui.m_reportSplitter->sizeHint().width() << 0; //Hide backtrace
    }
    ui.m_reportSplitter->setSizes(sizes);

    //Save the current show value
    KConfigGroup config(KGlobal::config(), "BugzillaReportInformationDialog");
    config.writeEntry("ShowOwnBacktrace", show);
}

//END BugzillaReportInformationDialog
