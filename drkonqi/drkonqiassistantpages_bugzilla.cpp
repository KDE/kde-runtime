/*******************************************************************
* drkonqiassistantpages_bugzilla.cpp
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

#include "drkonqiassistantpages_bugzilla.h"
#include "reportinfo.h"
#include "statuswidget.h"
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

//BEGIN BugzillaLoginPage

BugzillaLoginPage::BugzillaLoginPage(DrKonqiBugReport * parent) :
        DrKonqiAssistantPage(parent),
        m_wallet(0)
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
                            "for requesting further information. <nl />If you do not have "
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
        
        ui.m_statusWidget->setIdle(i18nc("@info:status the user is logged at the bugtracker site as USERNAME",
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
    m_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 
                                    static_cast<QWidget*>(this->parent())->winId());
}

void BugzillaLoginPage::walletLogin()
{
    if (!m_wallet) {
        if (kWalletEntryExists()) {  //Key exists!
            ui.m_savePasswordCheckBox->setCheckState(Qt::Checked);
            openWallet();
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
            if (m_wallet->isOpen()) {
                m_wallet->lockWallet();
            }
        }
    } else {
        ui.m_statusWidget->setIdle(i18nc("@info:status","Invalid username or password"));

        ui.m_loginButton->setEnabled(true);

        ui.m_userEdit->setEnabled(true);
        ui.m_passwordEdit->setEnabled(true);
        ui.m_savePasswordCheckBox->setEnabled(true);
        
        ui.m_userEdit->setFocus(Qt::OtherFocusReason);
    }
}

BugzillaLoginPage::~BugzillaLoginPage()
{
    if (m_wallet) {
        if (m_wallet->isOpen()) { //Close wallet if we close the assistant in this step
            m_wallet->lockWallet();
        }
        delete m_wallet;
    }
}

//END BugzillaLoginPage

//BEGIN BugzillaKeywordsPage

BugzillaKeywordsPage::BugzillaKeywordsPage(DrKonqiBugReport * parent) :
        DrKonqiAssistantPage(parent),
        m_keywordsOK(false)
{
    ui.setupUi(this);
    
    connect(ui.m_keywordsEdit, SIGNAL(textEdited(QString)), this, SLOT(textEdited(QString)));
}

void BugzillaKeywordsPage::textEdited(QString newText)
{
    QStringList list = newText.split(' ', QString::SkipEmptyParts);

    bool ok = (list.count() >= 4); //At least four (valid) words

    //Check words size
    /* FIXME, some words can be short, like version numbers
    if ( ok )
    {
        Q_FOREACH( const QString & word, list)
        {
            if( word.size() <= 3 )
                ok = false;
        }
    }
    */

    if (ok != m_keywordsOK) {
        m_keywordsOK = ok;
        emitCompleteChanged();
    }
}

bool BugzillaKeywordsPage::isComplete()
{
    return m_keywordsOK;
}

void BugzillaKeywordsPage::aboutToShow()
{
    textEdited(ui.m_keywordsEdit->text());
}

void BugzillaKeywordsPage::aboutToHide()
{
    //Save keywords (as short description of the future report)
    reportInfo()->setReportKeywords(ui.m_keywordsEdit->text());
}

//END BugzillaKeywordsPage

//BEGIN BugzillaDuplicatesPage

BugzillaDuplicatesPage::BugzillaDuplicatesPage(DrKonqiBugReport * parent):
        DrKonqiAssistantPage(parent),
        m_searching(false),
        m_infoDialog(0),
        m_possibleDuplicateBugNumber(0)
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
                                                   
    connect(ui.m_foundDuplicateCheckBox , SIGNAL(toggled(bool)), this, SLOT(checkBoxChanged(bool)));
    ui.m_foundDuplicateCheckBox->setVisible(false);
}

BugzillaDuplicatesPage::~BugzillaDuplicatesPage()
{
    delete m_infoDialog;
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
    
    ui.m_foundDuplicateCheckBox->setEnabled(!searching);
    
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

void BugzillaDuplicatesPage::checkBoxChanged(bool checked)
{
    if (!checked) {
        m_possibleDuplicateBugNumber = 0;
        ui.m_foundDuplicateCheckBox->setVisible(false);
    }
}

void BugzillaDuplicatesPage::itemClicked(QTreeWidgetItem * item, int col)
{
    Q_UNUSED(col);
    
    int bugNumber = item->text(0).toInt();
    if (bugNumber <= 0) {
        return;
    }
    if (!m_infoDialog) {
        m_infoDialog = new BugzillaReportInformationDialog(this);
    }
    m_infoDialog->showBugReport(bugNumber);
}

void BugzillaDuplicatesPage::setPossibleDuplicateNumber(int bugNumber)
{
    m_possibleDuplicateBugNumber = bugNumber;
    ui.m_foundDuplicateCheckBox->setVisible(true);
    ui.m_foundDuplicateCheckBox->setCheckState(Qt::Checked);
    ui.m_foundDuplicateCheckBox->setText(i18nc("@option:check", "My crash may be duplicate of "
                                                "bug: <numid>%1</numid>", bugNumber));
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
    if (m_currentKeywords != reportInfo()->reportKeywords()) { //Keywords changed
        m_currentKeywords = reportInfo()->reportKeywords();
        //Clear list and retrieve new reports
        ui.m_bugListWidget->clear();
        resetDates();
        searchMore();
    } else {
        if (!m_searching) {
            //If I never searched before, performSearch
            if (ui.m_bugListWidget->topLevelItemCount() == 0 && canSearchMore()) {
                searchMore();
            }
        }
    }
}

void BugzillaDuplicatesPage::aboutToHide()
{
    stopCurrentSearch();
    
    if (ui.m_foundDuplicateCheckBox->isChecked() && m_possibleDuplicateBugNumber>0) {
        reportInfo()->setPossibleDuplicate(QString::number(m_possibleDuplicateBugNumber));
    } else {
        reportInfo()->setPossibleDuplicate(QString());
    }
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
    BugReport report = reportInfo()->newBugReportTemplate();
    bugzillaManager()->searchBugs(m_currentKeywords, report.product(), report.bugSeverity(),
                                    startDateStr, endDateStr, reportInfo()->firstBacktraceFunctions().join(" "));
    
    //Test search
    /*
    bugzillaManager()->searchBugs( "konqueror crash toggle mode", "konqueror", "crash",  
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
                ui.m_foundDuplicateCheckBox->setEnabled(false);
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
    setModal(true);
    
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
    connect(m_parent->bugzillaManager(), SIGNAL(bugReportFetched(BugReport)),
             this, SLOT(bugFetchFinished(BugReport)));
    connect(m_parent->bugzillaManager(), SIGNAL(bugReportError(QString)),
             this, SLOT(bugFetchError(QString)));
             
    setInitialSize(QSize(800, 600));
    KConfigGroup config(KGlobal::config(), "BugzillaReportInformationDialog");
    restoreDialogSize(config);
}

BugzillaReportInformationDialog::~BugzillaReportInformationDialog()
{
    KConfigGroup config(KGlobal::config(), "BugzillaReportInformationDialog");
    saveDialogSize(config);
}

void BugzillaReportInformationDialog::showBugReport(int bugNumber)
{
    m_bugNumber = bugNumber;
    m_parent->bugzillaManager()->fetchBugReport(m_bugNumber);

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
                                            
    ui.m_backtraceBrowser->setPlainText(m_parent->reportInfo()->backtrace());
    show();
}

void BugzillaReportInformationDialog::bugFetchFinished(BugReport report)
{
    if (report.isValid()) {
        if (isVisible()) {
            QString comments;
            QStringList commentList = report.comments();
            for (int i = 0; i < commentList.count(); i++) {
                QString comment = commentList.at(i);
                comment.replace('\n', "<br />");
                comments += "<br /><strong>----</strong><br />" + comment;
            }

            QString text =
                i18n("<strong>Bug ID:</strong> %1<br />", report.bugNumber()) +
                i18n("<strong>Product:</strong> %1/%2<br />", report.product(),
                                                                report.component()) +
                i18n("<strong>Short Description:</strong> %1<br />",
                                                                report.shortDescription()) +
                i18n("<strong>Status:</strong> %1<br />", report.bugStatus()) +
                i18n("<strong>Resolution:</strong> %1<br />", report.resolution()) +
                i18n("<strong>Full Description:</strong><br />%1",
                                                report.description().replace('\n', "<br />")) +
                QLatin1String("<br /><br />") +
                i18n("<strong>Comments:</strong> %1", comments);

            ui.m_infoBrowser->setText(text);
            ui.m_infoBrowser->setEnabled(true);
            
            button(KDialog::User1)->setEnabled(true);

            ui.m_statusWidget->setIdle(i18nc("@info:status", "Showing report <numid>%1</numid>",
                                                            report.bugNumberAsInt()));
        }
    } else {
        bugFetchError(i18nc("@info","Invalid report data"));
    }
}

void BugzillaReportInformationDialog::mayBeDuplicateClicked()
{
    m_parent->setPossibleDuplicateNumber(m_bugNumber);
    hide();
}

void BugzillaReportInformationDialog::bugFetchError(QString err)
{
    if (isVisible()) {
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

BugzillaInformationPage::BugzillaInformationPage(DrKonqiBugReport * parent)
        : DrKonqiAssistantPage(parent),
        m_textsOK(false)
{
    ui.setupUi(this);
    
    connect(ui.m_titleEdit, SIGNAL(textChanged(QString)), this, SLOT(checkTexts()));
    connect(ui.m_detailsEdit, SIGNAL(textChanged()), this, SLOT(checkTexts()));

    checkTexts();
}

void BugzillaInformationPage::aboutToShow()
{
    if (ui.m_titleEdit->text().isEmpty()) {
        ui.m_titleEdit->setText(reportInfo()->reportKeywords());
    }

    bool showDetails = reportInfo()->userCanDetail();
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
    bool textsOk = false;
    checkTexts();
    if (m_textsOK) { //not empty
        bool titleShort = ui.m_titleEdit->text().size() < 50;
        bool detailsShort = ui.m_detailsEdit->isVisible() ?
                                (ui.m_detailsEdit->toPlainText().size() < 150) : false;

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
                textsOk = true; //Allow to continue
            }
        } else {
            textsOk = true;
        }
    }
    
    if (textsOk) {
        if (KMessageBox::questionYesNo(this, i18nc("@info question","Are you ready to submit this "
                                            "report?") , i18nc("@title:window","Are you sure?"))
                                            == KMessageBox::Yes) {
            return true;
        } else {
            return false;
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
    reportInfo()->setReportKeywords(ui.m_titleEdit->text());
    reportInfo()->setDetailText(ui.m_detailsEdit->toPlainText());
}

//END BugzillaInformationPage

//BEGIN BugzillaSendPage

BugzillaSendPage::BugzillaSendPage(DrKonqiBugReport * parent)
        : DrKonqiAssistantPage(parent)
{
    connect(bugzillaManager(), SIGNAL(reportSent(int)), this, SLOT(sent(int)));
    connect(bugzillaManager(), SIGNAL(sendReportError(QString)), this, SLOT(sendError(QString)));

    ui.setupUi(this);
    
    ui.m_retryButton->setGuiItem(KGuiItem2(i18nc("@action:button", "Retry..."),
                                              KIcon("view-refresh"),
                                              i18nc("@info:tooltip", "Use this button to retry "
                                                  "sending the crash report if it failed before.")));
    ui.m_retryButton->setVisible(false);
    connect(ui.m_retryButton, SIGNAL(clicked()), this , SLOT(retryClicked()));
    
    ui.m_launchPageOnFinish->setVisible(false);

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
    reportInfo()->sendBugReport(bugzillaManager());
}

void BugzillaSendPage::sent(int bug_id)
{
    ui.m_statusWidget->setVisible(false);
    ui.m_retryButton->setEnabled(false);
    ui.m_retryButton->setVisible(false);
    
    ui.m_launchPageOnFinish->setVisible(true);
    
    reportUrl = bugzillaManager()->urlForBug(bug_id);
    ui.m_finishedLabel->setText(i18nc("@info/rich","Crash report sent.<nl/>"
                                             "Bug Number :: <numid>%1</numid><nl/>"
                                             "URL :: <link>%2</link><nl/>"
                                             "Thanks for contributing to KDE. "
                                             "You can now close this window.", bug_id,
                                              reportUrl));
    
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
    if (ui.m_launchPageOnFinish->checkState()==Qt::Checked && !reportUrl.isEmpty()) {
        KToolInvocation::invokeBrowser(reportUrl);
    }
}

//END BugzillaSendPage
