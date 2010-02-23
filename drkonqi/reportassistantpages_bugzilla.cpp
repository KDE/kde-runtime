/*******************************************************************
* reportassistantpages_bugzilla.cpp
* Copyright 2009, 2010    Dario Andres Rodriguez <andresbajotierra@gmail.com>
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
#include "crashedapplication.h"
#include "statuswidget.h"
#include "drkonqi.h"
#include "drkonqi_globals.h"
#include "applicationdetailsexamples.h"

#include <QtCore/QTimer>

#include <QtGui/QLabel>
#include <QtGui/QCheckBox>
#include <QtGui/QToolTip>
#include <QtGui/QCursor>

#include <KIcon>
#include <KMessageBox>
#include <KToolInvocation>
#include <KWallet/Wallet>
#include <kcapacitybar.h>

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
                            DrKonqi::crashedApplication()->bugReportAddress(),
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

//BEGIN BugzillaInformationPage

BugzillaInformationPage::BugzillaInformationPage(ReportAssistantDialog * parent)
        : ReportAssistantPage(parent),
        m_textsOK(false), m_distributionComboSetup(false), m_distroComboVisible(false),
        m_requiredCharacters(1)
{
    ui.setupUi(this);
    m_textCompleteBar = new KCapacityBar(KCapacityBar::DrawTextInline, this);
    ui.horizontalLayout_2->addWidget(m_textCompleteBar);

    connect(ui.m_titleEdit, SIGNAL(textChanged(QString)), this, SLOT(checkTexts()));
    connect(ui.m_detailsEdit, SIGNAL(textChanged()), this, SLOT(checkTexts()));

    connect(ui.m_titleLabel, SIGNAL(linkActivated(QString)), this, SLOT(showTitleExamples()));
    connect(ui.m_detailsLabel, SIGNAL(linkActivated(QString)), this,
            SLOT(showDescriptionHelpExamples()));

    ui.m_compiledSourcesCheckBox->setChecked(
                                    DrKonqi::systemInformation()->compiledSources());

    //ui.m_distributionGroupBox->setVisible(false);  //TODO do this?
}

void BugzillaInformationPage::aboutToShow()
{
    if (!m_distributionComboSetup) {
        //Autodetecting distro failed ?
        if (DrKonqi::systemInformation()->bugzillaPlatform() == QLatin1String("unspecified")) {
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
                                                   "SuSE/OpenSUSE"), "openSUSE RPMs");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "RedHat"), "RedHat RPMs");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Fedora"), "Fedora RPMs");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Kubuntu/Ubuntu (and derivatives)"), 
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

    //Calculate the minimum number of characters required for a description
    //If creating a new report: minimum 40, maximum 80
    //If attaching to an existent report: minimum 30, maximum 50
    int multiplier = (reportInterface()->attachToBugNumber() == 0) ? 10 : 5;
    m_requiredCharacters = 20 + (reportInterface()->selectedOptionsRating() * multiplier);
    
    //Fill the description textedit with some headings:
    QString descriptionTemplate;
    if (ui.m_detailsEdit->toPlainText().isEmpty()) {
        if (reportInterface()->userCanProvideActionsAppDesktop()) {
            descriptionTemplate += "- What I was doing when the application crashed:\n\n\n";
        }
        if (reportInterface()->userCanProvideUnusualBehavior()) {
            descriptionTemplate += "- Unusual behavior I noticed:\n\n\n";
        }
        if (reportInterface()->userCanProvideApplicationConfigDetails()) {
            descriptionTemplate += "- Custom settings of the application:\n\n\n";
        }
        ui.m_detailsEdit->setText(descriptionTemplate);
    }

    checkTexts(); //May be the options (canDetail) changed and we need to recheck
}

int BugzillaInformationPage::currentDescriptionCharactersCount()
{
    QString description = ui.m_detailsEdit->toPlainText();

    //Do not count template messages, and other misc chars
    description.remove("What I was doing when the application crashed");
    description.remove("Unusual behavior I noticed");
    description.remove("Custom settings of the application");
    description.remove("\n");
    description.remove("-");
    description.remove(":");
    description.remove(" ");

    return description.size();
}

void BugzillaInformationPage::checkTexts()
{
    //If attaching this report to an existing one then the title is not needed
    bool showTitle = (reportInterface()->attachToBugNumber() == 0);
    ui.m_titleEdit->setVisible(showTitle);
    ui.m_titleLabel->setVisible(showTitle);

    bool ok = !((ui.m_titleEdit->isVisible() && ui.m_titleEdit->text().isEmpty())
        || ui.m_detailsEdit->toPlainText().isEmpty());

    QString message;
    int percent = currentDescriptionCharactersCount() * 100 / m_requiredCharacters;
    if (percent >= 100) {
        percent = 100;
        message = i18nc("the minimum required lenght of a text was reached",
                        "Minimum length reached");
    } else {
        message = i18nc("the minimum required lenght of a text wasn't reached yet",
                        "Provide more information");
    }
    m_textCompleteBar->setValue(percent);
    m_textCompleteBar->setText(message);

    if (ok != m_textsOK) {
        m_textsOK = ok;
        emitCompleteChanged();
    }
}

bool BugzillaInformationPage::showNextPage()
{
    checkTexts();

    if (m_textsOK) {
        bool titleShort = ui.m_titleEdit->isVisible() ? (ui.m_titleEdit->text().size() < 30) : false;

        bool detailsShort = currentDescriptionCharactersCount() < m_requiredCharacters;

        if (titleShort || detailsShort) {
            //The user input is less than we want.... encourage to write more
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

            message += ' ' + i18nc("@info","If you cannot provide enough information, your report "
                                    "will probably waste developers' time. Can you tell us more?");

            KGuiItem yesItem = KStandardGuiItem::yes();
            yesItem.setText("Yes, let me add more information");

            KGuiItem noItem = KStandardGuiItem::no();
            noItem.setText("No, I cannot add any other information");

            if (KMessageBox::warningYesNo(this, message,
                                           i18nc("@title:window","We need more information"),
                                           yesItem, noItem)
                                            == KMessageBox::No) {
                //Request the assistant to close itself (it will prompt for confirmation anyways)
                assistant()->close();
                return false;
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
        DrKonqi::systemInformation()->setBugzillaPlatform(bugzillaPlatform);
    }
    bool compiledFromSources = ui.m_compiledSourcesCheckBox->checkState() == Qt::Checked;
    DrKonqi::systemInformation()->setCompiledSources(compiledFromSources);
    
}

void BugzillaInformationPage::showTitleExamples()
{
    QString titleExamples = i18nc("@info:tooltip examples of good bug report titles",
          "<strong>Examples of good titles:</strong><nl />\"Plasma crashed after adding the Notes "
          "widget and writing on it\"<nl />\"Konqueror crashed when accessing the Facebook "
          "application 'X'\"<nl />\"Kopete suddenly closed after resuming the computer and "
          "talking to a MSN buddy\"<nl />\"Kate closed while editing a log file and pressing the "
          "Delete key a couple of times\"");
    QToolTip::showText(QCursor::pos(), titleExamples);
}

void BugzillaInformationPage::showDescriptionHelpExamples()
{
    QString descriptionHelp = i18nc("@info:tooltip help and examples of good bug descriptions",
                                  "Describe in as much detail as possible the crash circumstances:");
    if (reportInterface()->userCanProvideActionsAppDesktop()) {
        descriptionHelp += "<br />" +
                           i18nc("@info:tooltip help and examples of good bug descriptions",
                                 "- Detail which actions were you taking inside and outside the "
                                 "application an instant before the crash.");
    }
    if (reportInterface()->userCanProvideUnusualBehavior()) {
        descriptionHelp += "<br />" + 
                           i18nc("@info:tooltip help and examples of good bug descriptions",
                                 "- Note if you noticed any unusual behavior in the application "
                                 "or in the whole environment.");
    }
    if (reportInterface()->userCanProvideApplicationConfigDetails()) {
        descriptionHelp += "<br />" +
                           i18nc("@info:tooltip help and examples of good bug descriptions",
                                 "- Note any non-default configuration in the application.");
        if (reportInterface()->appDetailsExamples()->hasExamples()) {
            descriptionHelp += " " +
                               i18nc("@info:tooltip examples of configuration details. "
                                     "the examples are already translated",
                                     "Examples: %1",
                                     reportInterface()->appDetailsExamples()->examples());
        }
    }
    QToolTip::showText(QCursor::pos(), descriptionHelp);
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
    reportInterface()->sendBugReport();
}

void BugzillaSendPage::sent(int bug_id)
{
    ui.m_statusWidget->setVisible(false);
    ui.m_retryButton->setEnabled(false);
    ui.m_retryButton->setVisible(false);
    
    ui.m_showReportContentsButton->setVisible(false);
    
    ui.m_launchPageOnFinish->setVisible(true);
    ui.m_restartAppOnFinish->setVisible(!DrKonqi::crashedApplication()->hasBeenRestarted());
    ui.m_restartAppOnFinish->setChecked(false);
    
    reportUrl = bugzillaManager()->urlForBug(bug_id);
    ui.m_finishedLabel->setText(i18nc("@info/rich","Crash report sent.<nl/>"
                                             "URL: <link>%1</link><nl/>"
                                             "Thank you for being part of KDE. "
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
        DrKonqi::crashedApplication()->restart();
    }
}

void BugzillaSendPage::openReportContents()
{
    if (!m_contentsDialog)
    {
        QString report = reportInterface()->generateReport(false) + QLatin1Char('\n') +
                            i18nc("@info/plain report to KDE bugtracker address","Report to %1", 
                                  DrKonqi::crashedApplication()->bugReportAddress());
        m_contentsDialog = new ReportInformationDialog(report);
    }
    m_contentsDialog->show();
    m_contentsDialog->raise();
    m_contentsDialog->activateWindow();
}

//END BugzillaSendPage
