/*******************************************************************
* reportassistantpages_bugzilla.cpp
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

#include "reportassistantpages_bugzilla.h"

#include "reportinterface.h"
#include "systeminformation.h"

#include "statuswidget.h"
#include "drkonqi.h"
#include "drkonqi_globals.h"

#include <QtCore/QDate>
#include <QtCore/QTimer>

#include <QtGui/QLabel>
#include <QtGui/QCheckBox>
#include <QtGui/QTreeWidgetItem>
#include <QtGui/QHeaderView>

#include <KIcon>
#include <KMessageBox>
#include <KToolInvocation>
#include <KWallet/Wallet>

static const char kWalletEntryName[] = "drkonqi_bugzilla";
static const char kWalletEntryUsername[] = "username";
static const char kWalletEntryPassword[] = "password";

static const char konquerorKWalletEntryName[] = "https://bugs.kde.org/index.cgi#login";
static const char konquerorKWalletEntryUsername[] = "Bugzilla_login";
static const char konquerorKWalletEntryPassword[] = "Bugzilla_password";

//BEGIN BugzillaLoginPage

BugzillaLoginPage::BugzillaLoginPage(ReportAssistantDialog * parent) :
        ReportAssistantPage(parent),
        m_wallet(0), m_walletWasOpenedBefore(false)
{
    connect(bugzillaManager(), SIGNAL(loginFinished(bool)), this, SLOT(loginFinished(bool)));
    connect(bugzillaManager(), SIGNAL(loginError(QString)), this, SLOT(loginError(QString)));

    ui.setupUi(this);
    ui.m_statusWidget->setIdle(i18nc("@info:status '1' is replaced with \"bugs.kde.org\"",
                                   "You need to login with your %1 account in order to proceed.",
                                   QLatin1String(KDE_BUGZILLA_SHORT_URL)));

    ui.m_loginButton->setGuiItem(KGuiItem2(i18nc("@action:button", "Login"),
                                              KIcon("network-connect"),
                                              i18nc("@info:tooltip", "Use this button to login "
                                              "to the KDE bug tracking system using the provided "
                                              "username and password.")));
    connect(ui.m_loginButton, SIGNAL(clicked()) , this, SLOT(loginClicked()));

    connect(ui.m_userEdit, SIGNAL(returnPressed()) , this, SLOT(loginClicked()));
    connect(ui.m_passwordEdit, SIGNAL(returnPressed()) , this, SLOT(loginClicked()));

    ui.m_noticeLabel->setText(
                        i18nc("@info/rich","<note>You need a user account on the "
                            "<link url='%1'>KDE bug tracking system</link> in order to "
                            "file a bug report, because we may need to contact you later "
                            "for requesting further information. If you do not have "
                            "one, you can freely <link url='%2'>create one here</link>.</note>",
                            QLatin1String(KDE_BUGZILLA_URL),
                            QLatin1String(KDE_BUGZILLA_CREATE_ACCOUNT_URL)));
}

bool BugzillaLoginPage::isComplete()
{
    return bugzillaManager()->getLogged();
}

void BugzillaLoginPage::loginError(QString err)
{
    loginFinished(false);
    ui.m_statusWidget->setIdle(i18nc("@info:status","Error when trying to login: "
                                                 "<message>%1.</message>", err));
}

void BugzillaLoginPage::aboutToShow()
{
    if (bugzillaManager()->getLogged()) {
        ui.m_loginButton->setEnabled(false);

        ui.m_userEdit->setEnabled(false);
        ui.m_userEdit->clear();
        ui.m_passwordEdit->setEnabled(false);
        ui.m_passwordEdit->clear();

        ui.m_loginButton->setVisible(false);
        ui.m_userEdit->setVisible(false);
        ui.m_passwordEdit->setVisible(false);
        ui.m_userLabel->setVisible(false);
        ui.m_passwordLabel->setVisible(false);

        ui.m_savePasswordCheckBox->setVisible(false);
        
        ui.m_noticeLabel->setVisible(false);
        
        ui.m_statusWidget->setIdle(i18nc("@info:status the user is logged at the bugtracker site "
                                      "as USERNAME",
                                      "Logged in at the KDE bug tracking system (%1) as: %2.",
                                      QLatin1String(KDE_BUGZILLA_SHORT_URL),
                                      bugzillaManager()->getUsername()));
    } else {
        //Try to show wallet dialog once this dialog is shown
        QTimer::singleShot(100, this, SLOT(walletLogin()));
    }
}

bool BugzillaLoginPage::kWalletEntryExists()
{
    return !KWallet::Wallet::keyDoesNotExist(KWallet::Wallet::NetworkWallet(),
                                               KWallet::Wallet::FormDataFolder(),
                                               QLatin1String(kWalletEntryName));
}

void BugzillaLoginPage::openWallet()
{
    //Store if the wallet was previously opened so we can know if we should close it later
    m_walletWasOpenedBefore = KWallet::Wallet::isOpen(KWallet::Wallet::NetworkWallet());
    //Request open the wallet
    m_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 
                                    static_cast<QWidget*>(this->parent())->winId());
}

void BugzillaLoginPage::walletLogin()
{
    if (!m_wallet) {
        if (kWalletEntryExists()) {  //Key exists!
            openWallet();
            ui.m_savePasswordCheckBox->setCheckState(Qt::Checked);
            //Was the wallet opened?
            if (m_wallet) {
                m_wallet->setFolder(KWallet::Wallet::FormDataFolder());

                //Use wallet data to try login
                QMap<QString, QString> values;
                m_wallet->readMap(QLatin1String(kWalletEntryName), values);
                QString username = values.value(QLatin1String(kWalletEntryUsername));
                QString password = values.value(QLatin1String(kWalletEntryPassword));

                if (!username.isEmpty() && !password.isEmpty()) {
                    ui.m_userEdit->setText(username);
                    ui.m_passwordEdit->setText(password);
                }
            }
        } else if (!KWallet::Wallet::keyDoesNotExist(KWallet::Wallet::NetworkWallet(),
                                               KWallet::Wallet::FormDataFolder(),
                                               QLatin1String(konquerorKWalletEntryName))) {
            //If the DrKonqi entry is empty, but a Konqueror entry exists, use and copy it.
            openWallet();
            if (m_wallet) {
                m_wallet->setFolder(KWallet::Wallet::FormDataFolder());

                //Fetch Konqueror data
                QMap<QString, QString> values;
                m_wallet->readMap(QLatin1String(konquerorKWalletEntryName), values);
                QString username = values.value(QLatin1String(konquerorKWalletEntryUsername));
                QString password = values.value(QLatin1String(konquerorKWalletEntryPassword));
                
                if (!username.isEmpty() && !password.isEmpty()) {
                    //Copy to DrKonqi own entries
                    values.clear();
                    values.insert(QLatin1String(kWalletEntryUsername), username);
                    values.insert(QLatin1String(kWalletEntryPassword), password);
                    m_wallet->writeMap(QLatin1String(kWalletEntryName), values);

                    ui.m_savePasswordCheckBox->setCheckState(Qt::Checked);
                    
                    ui.m_userEdit->setText(username);
                    ui.m_passwordEdit->setText(password);
                }
            }
            
        }
    }
}

void BugzillaLoginPage::loginClicked()
{
    if (!(ui.m_userEdit->text().isEmpty() || ui.m_passwordEdit->text().isEmpty())) {
        ui.m_loginButton->setEnabled(false);

        ui.m_userEdit->setEnabled(false);
        ui.m_passwordEdit->setEnabled(false);
        ui.m_savePasswordCheckBox->setEnabled(false);
        
        if (ui.m_savePasswordCheckBox->checkState()==Qt::Checked) { //Wants to save data
            if (!m_wallet) {
                openWallet();
            }
            //Got wallet open ?
            if (m_wallet) {
                m_wallet->setFolder(KWallet::Wallet::FormDataFolder());
                
                QMap<QString, QString> values;
                values.insert(QLatin1String(kWalletEntryUsername), ui.m_userEdit->text());
                values.insert(QLatin1String(kWalletEntryPassword), ui.m_passwordEdit->text());
                m_wallet->writeMap(QLatin1String(kWalletEntryName), values);
            }
            
        } else { //User doesn't want to save or wants to remove.
            if (kWalletEntryExists()) {
                if (!m_wallet) {
                    openWallet();
                }
                //Got wallet open ?
                if (m_wallet) {
                    m_wallet->setFolder(KWallet::Wallet::FormDataFolder());
                    m_wallet->removeEntry(QLatin1String(kWalletEntryName));
                }
            }
        }

        ui.m_statusWidget->setBusy(i18nc("@info:status '1' is a url, '2' the username",
                                      "Performing login at %1 as %2...",
                                      QLatin1String(KDE_BUGZILLA_SHORT_URL), ui.m_userEdit->text()));

        bugzillaManager()->setLoginData(ui.m_userEdit->text(), ui.m_passwordEdit->text());
        bugzillaManager()->tryLogin();
    } else {
        loginFinished(false);
    }
}

void BugzillaLoginPage::loginFinished(bool logged)
{
    if (logged) {
        emitCompleteChanged();

        aboutToShow();
        if (m_wallet) {
            if (m_wallet->isOpen() && !m_walletWasOpenedBefore) {
                m_wallet->lockWallet();
            }
        }
        
        emit loggedTurnToNextPage();
    } else {
        ui.m_statusWidget->setIdle(i18nc("@info:status/rich","<b>Error: Invalid username or "
                                                                                  "password</b>"));

        ui.m_loginButton->setEnabled(true);

        ui.m_userEdit->setEnabled(true);
        ui.m_passwordEdit->setEnabled(true);
        ui.m_savePasswordCheckBox->setEnabled(true);
        
        ui.m_userEdit->setFocus(Qt::OtherFocusReason);
    }
}

BugzillaLoginPage::~BugzillaLoginPage()
{
    //Close wallet if we close the assistant in this step
    if (m_wallet) {
        if (m_wallet->isOpen() && !m_walletWasOpenedBefore) { 
            m_wallet->lockWallet();
        }
        delete m_wallet;
    }
}

//END BugzillaLoginPage

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
    connect(ui.m_stopSearchButton, SIGNAL(clicked()), this, SLOT(stopCurrentSearch()));
    
    //Possible duplicates list and buttons
    connect(ui.m_selectedDuplicatesList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
             this, SLOT(itemClicked(QListWidgetItem*)));
             
    ui.m_removeSelectedDuplicateButton->setIcon(KIcon("list-remove"));
    connect(ui.m_removeSelectedDuplicateButton, SIGNAL(clicked()), this, 
                                                                SLOT(removeSelectedDuplicate()));
}

BugzillaDuplicatesPage::~BugzillaDuplicatesPage()
{
}

void BugzillaDuplicatesPage::itemSelectionChanged()
{
    ui.m_openReportButton->setEnabled(ui.m_bugListWidget->selectedItems().count() == 1);
}

void BugzillaDuplicatesPage::openSelectedReport()
{
    QList<QTreeWidgetItem*> selected = ui.m_bugListWidget->selectedItems();
    if (selected.count() == 1) {
        itemClicked(selected.at(0), 0);
    }
}

void BugzillaDuplicatesPage::markAsSearching(bool searching)
{
    m_searching = searching;
    
    ui.m_bugListWidget->setEnabled(!searching);
    ui.m_searchMoreButton->setEnabled(!searching);
    ui.m_searchMoreButton->setVisible(!searching);
    ui.m_stopSearchButton->setEnabled(searching);
    ui.m_stopSearchButton->setVisible(searching);
    
    ui.m_selectedDuplicatesList->setEnabled(!searching);
    ui.m_removeSelectedDuplicateButton->setEnabled(!searching);
    
    if (!searching) {
        itemSelectionChanged();
    } else {
        ui.m_openReportButton->setEnabled(false);
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

void BugzillaDuplicatesPage::itemClicked(QTreeWidgetItem * item, int col)
{
    Q_UNUSED(col);
    showReportInformationDialog(item->text(0).toInt());
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
    infoDialog->showBugReport(bugNumber);
}

void BugzillaDuplicatesPage::addPossibleDuplicateNumber(int bugNumber)
{
    QString stringNumber = QString::number(bugNumber);
    if (ui.m_selectedDuplicatesList->findItems(stringNumber, Qt::MatchExactly).isEmpty()) {
        ui.m_selectedDuplicatesList->addItem(stringNumber);
    }
}

void BugzillaDuplicatesPage::removeSelectedDuplicate()
{
    QList<QListWidgetItem*> items = ui.m_selectedDuplicatesList->selectedItems();
    if (items.length() > 0) {
        delete ui.m_selectedDuplicatesList->takeItem(ui.m_selectedDuplicatesList->row(items.at(0)));
    }
}


bool BugzillaDuplicatesPage::canSearchMore()
{
    return (m_startDate.year() >= 2002);
}

void BugzillaDuplicatesPage::resetDates()
{
    m_endDate = QDate::currentDate();
    m_startDate = m_endDate;
}

void BugzillaDuplicatesPage::aboutToShow()
{
    //Perform initial search if we are not currently searching and if there are no results yet
    if (!m_searching && ui.m_bugListWidget->topLevelItemCount() == 0 && canSearchMore()) {
        searchMore();
    }
}

void BugzillaDuplicatesPage::aboutToHide()
{
    stopCurrentSearch();
    
    QStringList possibleDuplicates;
    int count = ui.m_selectedDuplicatesList->count();
    for(int i = 0; i<count; i++) {
        possibleDuplicates << ui.m_selectedDuplicatesList->item(i)->text();
    }
    reportInterface()->setPossibleDuplicates(possibleDuplicates);
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
    
    BugReport report = reportInterface()->newBugReportTemplate();
    bugzillaManager()->searchBugs(QString(), reportInterface()->relatedBugzillaProducts(), 
                                  report.bugSeverity(), startDateStr, endDateStr, 
                                  reportInterface()->firstBacktraceFunctions().join(" "));
    
    //Test search
    /*
    bugzillaManager()->searchBugs( QString(), QStringList() << "konqueror", "crash",  
                startDateStr, endDateStr , "caret" );
    */
}

void BugzillaDuplicatesPage::searchMore()
{
    //1 year back
    m_searchingEndDate = m_startDate;
    m_searchingStartDate = m_searchingEndDate.addYears(-1);

    performSearch();
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

            QStringList fields = QStringList() << bug["bug_id"] << bug["short_desc"];
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

//END BugzillaDuplicatesPage

//BEGIN BugzillaReportInformationDialog

BugzillaReportInformationDialog::BugzillaReportInformationDialog(BugzillaDuplicatesPage * parent) :
        KDialog(parent),
        m_bugNumber(0)
{
    m_parent = parent;
    
    //Create the GUI
    setButtons(KDialog::Close | KDialog::User1);
    setDefaultButton(KDialog::Close);
    setCaption(i18nc("@title:window","Bug Description"));
    
    QWidget * widget = new QWidget(this);
    ui.setupUi(widget);
    setMainWidget(widget);
        
    setButtonGuiItem(KDialog::User1, 
                KGuiItem2(i18nc("@action:button", "My crash may be a duplicate of this report"),
                    KIcon("document-import"), i18nc("@info:tooltip", "Use this button to mark your "
                    "crash as related to the currently shown bug report. This will help "
                    "the KDE developers to determine whether they are duplicates or not.")));
    connect(this, SIGNAL(user1Clicked()) , this, SLOT(mayBeDuplicateClicked()));

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

void BugzillaReportInformationDialog::showBugReport(int bugNumber)
{
    m_bugNumber = bugNumber;
    m_parent->bugzillaManager()->fetchBugReport(m_bugNumber, this);

    button(KDialog::User1)->setEnabled(false);

    ui.m_infoBrowser->setText(i18nc("@info:status","Loading..."));
    ui.m_infoBrowser->setEnabled(false);
    
    ui.m_linkLabel->setText(i18nc("@info","<link url='%1'>Bug report page at the KDE bug "
                                              "tracking system</link>",
                                    m_parent->bugzillaManager()->urlForBug(m_bugNumber)));

    ui.m_statusWidget->setBusy(i18nc("@info:status","Loading information about bug "
                                                           "<numid>%1</numid> from %2....",
                                            m_bugNumber,
                                            QLatin1String(KDE_BUGZILLA_SHORT_URL)));
                                            
    ui.m_backtraceBrowser->setPlainText(m_parent->reportInterface()->backtrace());
    show();
}

void BugzillaReportInformationDialog::bugFetchFinished(BugReport report, QObject * jobOwner)
{
    if (jobOwner == this && isVisible()) {
        if (report.isValid()) {
            //Generate html for comments
            QString comments;
            QStringList commentList = report.comments();
            for (int i = 0; i < commentList.count(); i++) {
                QString comment = commentList.at(i);
                comment.replace('\n', "<br />");
                comments += "<br /><strong>----</strong><br />" + comment;
            }

            //Generate a non-geek readable status
            QString customStatusString;
            if (report.bugStatus() == QLatin1String("UNCONFIRMED")
                || report.bugStatus() == QLatin1String("NEW")
                || report.bugStatus() == QLatin1String("REOPENED")
                || report.bugStatus() == QLatin1String("ASSIGNED")) {
                customStatusString = i18nc("@info bug status", "Open");
            } else if (report.bugStatus() == QLatin1String("RESOLVED")
                || report.bugStatus() == QLatin1String("VERIFIED")
                || report.bugStatus() == QLatin1String("CLOSED")) {
                
                QString customResolutionString;
                if (report.resolution() == QLatin1String("FIXED")) {
                    customResolutionString = i18nc("@info bug resolution", "Fixed");
                } else if (report.resolution() == QLatin1String("WORKSFORME")) {
                    customResolutionString = i18nc("@info bug resolution", "Non-reproducible");
                } else if (report.resolution() == QLatin1String("DUPLICATE")) {
                    customResolutionString = i18nc("@info bug resolution", "Already reported");
                } else if (report.resolution() == QLatin1String("INVALID")) {
                    customResolutionString = i18nc("@info bug resolution", "Not a valid report/crash");
                } else if (report.resolution() == QLatin1String("DOWNSTREAM") 
                    || report.resolution() == QLatin1String("UPSTREAM")) {
                    customResolutionString = i18nc("@info bug resolution", "Not a KDE bug");
                } else {
                    customResolutionString = report.resolution();
                }
                
                customStatusString = i18nc("@info bug status, %1 is the resolutin", "Closed (%1)",
                                           customResolutionString);
            } else if (report.bugStatus() == QLatin1String("NEEDSINFO")) {
                customStatusString = i18nc("@info bug status", "Temporary closed because the lack "
                                            "of information");
            } else {
                customStatusString = QString("%1 (%2)").arg(report.bugStatus(), report.resolution());
            }
            
            QString text =
                i18nc("@label:textbox bug report label and value", 
                                "<strong>Product:</strong> %1 (%2)<br />", 
                                report.product(), report.component()) +
                i18nc("@label:textbox bug report label and value", 
                                "<strong>Title:</strong> %1<br />", report.shortDescription()) +
                i18nc("@label:textbox bug report label and value", 
                                "<strong>Status:</strong> %1<br />", customStatusString) +
                i18nc("@label:textbox bug report label and value", 
                                "<strong>Full Description:</strong><br />%1", 
                                report.description().replace('\n', "<br />"));
                                                
            if (!comments.isEmpty()) {
                text += i18nc("@label:textbox bug report label and value", 
                                "<br /><br /><strong>Comments:</strong> %1", comments);
            }

            ui.m_infoBrowser->setText(text);
            ui.m_infoBrowser->setEnabled(true);
            
            button(KDialog::User1)->setEnabled(true);

            ui.m_statusWidget->setIdle(i18nc("@info:status", "Showing report <numid>%1</numid>",
                                                            report.bugNumberAsInt()));
        } else {
            bugFetchError(i18nc("@info","Invalid report data"), this);
        }
    }
}

void BugzillaReportInformationDialog::mayBeDuplicateClicked()
{
    m_parent->addPossibleDuplicateNumber(m_bugNumber);
    hide();
}

void BugzillaReportInformationDialog::bugFetchError(QString err, QObject * jobOwner)
{
    if (jobOwner == this && isVisible()) {
        KMessageBox::error(this , i18nc("@info/rich","Error fetching the bug report<nl/>"
                                        "<message>%1.</message><nl/>"
                                        "Please wait some time and try again.", err));
        button(KDialog::User1)->setEnabled(false);
        ui.m_infoBrowser->setText(i18nc("@info","Error fetching the bug report"));
        ui.m_statusWidget->setIdle(i18nc("@info:status","Error fetching the bug report"));
    }
}

//END BugzillaReportInformationDialog

//BEGIN BugzillaInformationPage

BugzillaInformationPage::BugzillaInformationPage(ReportAssistantDialog * parent)
        : ReportAssistantPage(parent),
        m_textsOK(false), m_distributionComboSetup(false), m_distroComboVisible(false)
{
    ui.setupUi(this);
    
    connect(ui.m_titleEdit, SIGNAL(textChanged(QString)), this, SLOT(checkTexts()));
    connect(ui.m_detailsEdit, SIGNAL(textChanged()), this, SLOT(checkTexts()));

    ui.m_compiledSourcesCheckBox->setChecked(
                                    DrKonqi::instance()->systemInformation()->compiledSources());
    
    checkTexts();
}

void BugzillaInformationPage::aboutToShow()
{
    if (!m_distributionComboSetup) {
        //Autodetecting distro failed ?
        if (DrKonqi::instance()->systemInformation()->bugzillaPlatform() == 
                                                                    QLatin1String("unspecified")) { 
            m_distroComboVisible = true;
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Unspecified"),"unspecified");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Debian stable"), "Debian stable");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Debian testing"), "Debian testing");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Debian unstable"), "Debian unstable");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Exherbo"), "Exherbo Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Gentoo"), "Gentoo Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Mandriva"), "Mandriva RPMs");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Slackware"), "Slackware Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "SuSE/OpenSUSE"), "SuSE RPMs");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "RedHat"), "RedHat RPMs");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Fedora"), "Fedora RPMs");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Kubuntu/Ubuntu (and derivates)"), 
                                                   "Ubuntu Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Pardus"), "Pardus Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Archlinux"), "Archlinux Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "FreeBSD (Ports)"), "FreeBSD Ports");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "NetBSD (pkgsrc)"), "NetBSD pkgsrc");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "OpenBSD"), "OpenBSD Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Mac OS X"), "MacPorts Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Solaris"), "Solaris Packages");
            
            //Restore previously selected bugzilla platform (distribution)
            KConfigGroup config(KGlobal::config(), "BugzillaInformationPage");
            QString entry = config.readEntry("BugzillaPlatform","unspecified");
            int index = ui.m_distroChooserCombo->findData(entry);
            if ( index == -1 ) index = 0;
            ui.m_distroChooserCombo->setCurrentIndex(index);
        } else {
            ui.m_distroChooserCombo->setVisible(false);
        }
        m_distributionComboSetup = true;
    }

    bool showDetails = reportInterface()->userCanDetail();
    ui.m_detailsLabel->setVisible(showDetails);
    ui.m_detailsEdit->setVisible(showDetails);

    checkTexts(); //May be the options (canDetail) changed and we need to recheck
}

void BugzillaInformationPage::checkTexts()
{
    bool detailsNotOk = ui.m_detailsEdit->isVisible() ? ui.m_detailsEdit->toPlainText().isEmpty() : false;
    bool ok = !(ui.m_titleEdit->text().isEmpty() || detailsNotOk);

    if (ok != m_textsOK) {
        m_textsOK = ok;
        emitCompleteChanged();
    }
}

bool BugzillaInformationPage::showNextPage()
{
    checkTexts();
    if (m_textsOK) { //not empty
        bool titleShort = ui.m_titleEdit->text().size() < 25;
        bool detailsShort = ui.m_detailsEdit->isVisible() ?
                                (ui.m_detailsEdit->toPlainText().size() < 100) : false;

        if (titleShort || detailsShort) {
            QString message;

            if (titleShort && !detailsShort) {
                message = i18nc("@info","The title does not provide enough information.");
            } else if (detailsShort && !titleShort) {
                message = i18nc("@info","The description about the crash details does not provide "
                                        "enough information.");
            } else {
                message = i18nc("@info","Both the title and the description about the crash "
                                        "details do not provide enough information.");
            }

            message += ' ' + i18nc("@info","Can you tell us more?");

            //The user input is less than we want.... encourage to write more
            if (KMessageBox::questionYesNo(this, message,
                                           i18nc("@title:window","We need more information"))
                                            == KMessageBox::No) {
                return true; //Allow to continue
            }
        } else {
            return true;
        }
    }
    
    return false;
}

bool BugzillaInformationPage::isComplete()
{
    return m_textsOK;
}

void BugzillaInformationPage::aboutToHide()
{
    //Save fields data
    reportInterface()->setTitle(ui.m_titleEdit->text());
    reportInterface()->setDetailText(ui.m_detailsEdit->toPlainText());
    
    if (m_distroComboVisible) {
        //Save bugzilla platform (distribution)
        QString bugzillaPlatform = ui.m_distroChooserCombo->itemData(
                                        ui.m_distroChooserCombo->currentIndex()).toString();
        KConfigGroup config(KGlobal::config(), "BugzillaInformationPage");
        config.writeEntry("BugzillaPlatform", bugzillaPlatform);
        DrKonqi::instance()->systemInformation()->setBugzillaPlatform(bugzillaPlatform);
    }
    bool compiledFromSources = ui.m_compiledSourcesCheckBox->checkState() == Qt::Checked;
    DrKonqi::instance()->systemInformation()->setCompiledSources(compiledFromSources);
    
}

//END BugzillaInformationPage

//BEGIN BugzillaPreviewPage

BugzillaPreviewPage::BugzillaPreviewPage(ReportAssistantDialog * parent)
        : ReportAssistantPage(parent)
{
    ui.setupUi(this);
}

void BugzillaPreviewPage::aboutToShow()
{
    ui.m_previewEdit->setText(reportInterface()->generateReport(true));
}

bool BugzillaPreviewPage::showNextPage()
{
    return (KMessageBox::questionYesNo(this, i18nc("@info question","Are you ready to submit this "
                                            "report?") , i18nc("@title:window","Are you sure?"))
                                            == KMessageBox::Yes);
}

//END BugzillaPreviewPage

//BEGIN BugzillaSendPage

BugzillaSendPage::BugzillaSendPage(ReportAssistantDialog * parent)
        : ReportAssistantPage(parent),
        m_contentsDialog(0)
{
    connect(reportInterface(), SIGNAL(reportSent(int)), this, SLOT(sent(int)));
    connect(reportInterface(), SIGNAL(sendReportError(QString)), this, SLOT(sendError(QString)));

    ui.setupUi(this);
    
    ui.m_retryButton->setGuiItem(KGuiItem2(i18nc("@action:button", "Retry..."),
                                              KIcon("view-refresh"),
                                              i18nc("@info:tooltip", "Use this button to retry "
                                                  "sending the crash report if it failed before.")));
    
    ui.m_showReportContentsButton->setGuiItem(
                    KGuiItem2(i18nc("@action:button", "Sho&w Contents of the Report"),
                            KIcon("document-preview"),
                            i18nc("@info:tooltip", "Use this button to show the generated "
                            "report information about this crash.")));
    connect(ui.m_showReportContentsButton, SIGNAL(clicked()), this, SLOT(openReportContents()));
                                                  
    ui.m_retryButton->setVisible(false);
    connect(ui.m_retryButton, SIGNAL(clicked()), this , SLOT(retryClicked()));
    
    ui.m_launchPageOnFinish->setVisible(false);
    ui.m_restartAppOnFinish->setVisible(false);
    
    connect(assistant(), SIGNAL(user1Clicked()), this, SLOT(finishClicked()));
}

void BugzillaSendPage::retryClicked()
{
    ui.m_retryButton->setEnabled(false);
    aboutToShow();
}

void BugzillaSendPage::aboutToShow()
{
    ui.m_statusWidget->setBusy(i18nc("@info:status","Sending crash report... (please wait)"));
    reportInterface()->sendBugReport(bugzillaManager());
}

void BugzillaSendPage::sent(int bug_id)
{
    ui.m_statusWidget->setVisible(false);
    ui.m_retryButton->setEnabled(false);
    ui.m_retryButton->setVisible(false);
    
    ui.m_showReportContentsButton->setVisible(false);
    
    ui.m_launchPageOnFinish->setVisible(true);
    ui.m_restartAppOnFinish->setVisible(!DrKonqi::instance()->appRestarted());
    ui.m_restartAppOnFinish->setChecked(false);
    
    reportUrl = bugzillaManager()->urlForBug(bug_id);
    ui.m_finishedLabel->setText(i18nc("@info/rich","Crash report sent.<nl/>"
                                             "URL: <link>%1</link><nl/>"
                                             "Thanks for contributing to KDE. "
                                             "You can now close this window.", reportUrl));
    
    emit finished(false);
}

void BugzillaSendPage::sendError(QString errorString)
{
    ui.m_statusWidget->setIdle(i18nc("@info:status","Error sending the crash report:  "
                                  "<message>%1.</message>", errorString));

    ui.m_retryButton->setEnabled(true);
    ui.m_retryButton->setVisible(true);
}

void BugzillaSendPage::finishClicked()
{
    if (ui.m_launchPageOnFinish->isChecked() && !reportUrl.isEmpty()) {
        KToolInvocation::invokeBrowser(reportUrl);
    }
    if (ui.m_restartAppOnFinish->isChecked()) {
        DrKonqi::instance()->restartCrashedApplication();
    }
}

void BugzillaSendPage::openReportContents()
{
    if (!m_contentsDialog)
    {
        QString report = reportInterface()->generateReport(false) + QLatin1Char('\n') +
                            i18nc("@info/plain","Report to %1", QLatin1String(KDE_BUGZILLA_URL));
        m_contentsDialog = new ReportInformationDialog(report);
    }
    m_contentsDialog->show();
    m_contentsDialog->raise();
    m_contentsDialog->activateWindow();
}

//END BugzillaSendPage
