/*******************************************************************
* reportassistantpages_base.cpp
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
* Copyright 2009    A. L. Spehr <spehr@kde.org>
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

#include "reportassistantpages_base.h"

#include "drkonqi.h"
#include "debuggermanager.h"
#include "crashedapplication.h"
#include "reportinterface.h"
#include "backtraceparser.h"
#include "backtracegenerator.h"
#include "drkonqi_globals.h"
#include "applicationdetailsexamples.h"

#include <QtGui/QLabel>
#include <QtGui/QCheckBox>
#include <QtGui/QToolTip>

#include <KToolInvocation>
#include <KIcon>
#include <KMessageBox>

//BEGIN IntroductionPage

IntroductionPage::IntroductionPage(ReportAssistantDialog * parent)
        : ReportAssistantPage(parent)
{
    ui.setupUi(this);
    ui.m_warningIcon->setPixmap(KIcon("dialog-warning").pixmap(64,64));
}

//END IntroductionPage

//BEGIN CrashInformationPage

CrashInformationPage::CrashInformationPage(ReportAssistantDialog * parent)
        : ReportAssistantPage(parent)
{
    m_backtraceWidget = new BacktraceWidget(DrKonqi::debuggerManager()->backtraceGenerator(), this, true);
    connect(m_backtraceWidget, SIGNAL(stateChanged()) , this, SLOT(emitCompleteChanged()));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(m_backtraceWidget);
    layout->addSpacing(10); //We need this for better usability until we get something better

    //If the backtrace was already fetched on the main dialog, save it.
    BacktraceGenerator *btGenerator = DrKonqi::debuggerManager()->backtraceGenerator();
    if (btGenerator->state() == BacktraceGenerator::Loaded) {
        BacktraceParser::Usefulness use = btGenerator->parser()->backtraceUsefulness();
        if (use != BacktraceParser::Useless && use != BacktraceParser::InvalidUsefulness) {
            reportInterface()->setBacktrace(btGenerator->backtrace());
        }
    }
}

void CrashInformationPage::aboutToShow()
{
    m_backtraceWidget->generateBacktrace();
    m_backtraceWidget->hilightExtraDetailsLabel(false);
    emitCompleteChanged();
}

void CrashInformationPage::aboutToHide()
{
    BacktraceGenerator *btGenerator = DrKonqi::debuggerManager()->backtraceGenerator();
    BacktraceParser::Usefulness use = btGenerator->parser()->backtraceUsefulness();

    if (use != BacktraceParser::Useless && use != BacktraceParser::InvalidUsefulness) {
        reportInterface()->setBacktrace(btGenerator->backtrace());
    }
    reportInterface()->setFirstBacktraceFunctions(btGenerator->parser()->firstValidFunctions());
}

bool CrashInformationPage::isComplete()
{
    BacktraceGenerator *generator = DrKonqi::debuggerManager()->backtraceGenerator();
    return (generator->state() != BacktraceGenerator::NotLoaded &&
            generator->state() != BacktraceGenerator::Loading);
}

bool CrashInformationPage::showNextPage()
{
    BacktraceParser::Usefulness use =
                    DrKonqi::debuggerManager()->backtraceGenerator()->parser()->backtraceUsefulness();

    if ((use == BacktraceParser::InvalidUsefulness || use == BacktraceParser::ProbablyUseless
            || use == BacktraceParser::Useless) && m_backtraceWidget->canInstallDebugPackages()) {
        if ( KMessageBox::Yes == KMessageBox::questionYesNo(this,
                                i18nc("@info","This crash information is not useful enough, "
                                              "do you want to try to improve it? You will need "
                                              "to install some debugging packages."),
                                i18nc("@title:window","Crash Information is not useful enough")) ) {
            m_backtraceWidget->hilightExtraDetailsLabel(true);
            m_backtraceWidget->focusImproveBacktraceButton();
            return false; //Cancel show next, to allow the user to write more
        } else {
            return true; //Allow to continue
        }
    } else {
        return true;
    }
}

//END CrashInformationPage

//BEGIN BugAwarenessPage

BugAwarenessPage::BugAwarenessPage(ReportAssistantDialog * parent)
        : ReportAssistantPage(parent)
{
    ui.setupUi(this);

    ui.m_actionsInsideApp->setText(i18nc("@option:check kind of information the user can provide "
                                    "about the crash, %1 is the application name",
                                     "What I was doing when the application \"%1\" crashed",
                                         DrKonqi::crashedApplication()->name()));

    connect(ui.m_rememberGroup, SIGNAL(buttonClicked(int)), this, SLOT(updateCheckBoxes()));

    ui.m_appSpecificDetailsExamples->setVisible(reportInterface()->appDetailsExamples()->hasExamples());
    
    connect(ui.m_appSpecificDetailsExamples, SIGNAL(linkHovered(QString)), this,
            SLOT(showApplicationDetailsExamples()));
    connect(ui.m_appSpecificDetailsExamples, SIGNAL(linkActivated(QString)), this,
            SLOT(showApplicationDetailsExamples()));
}

void BugAwarenessPage::aboutToShow()
{
    updateCheckBoxes();
}

void BugAwarenessPage::aboutToHide()
{
    //Save data
    ReportInterface::Reproducible reproducible = ReportInterface::ReproducibleUnsure;
    switch(ui.m_reproducibleBox->currentIndex()) {
        case 0: {
            reproducible = ReportInterface::ReproducibleUnsure;
            break;
        }
        case 1: {
            reproducible = ReportInterface::ReproducibleNever;
            break;
        }
        case 2: {
            reproducible = ReportInterface::ReproducibleSometimes;
            break;
        }
        case 3: {
            reproducible = ReportInterface::ReproducibleEverytime;
            break;
        }
    }

    reportInterface()->setBugAwarenessPageData(ui.m_rememberCrashSituationYes->isChecked(),
                                               reproducible,
                                               ui.m_actionsInsideApp->isChecked(),
                                               ui.m_unusualSituation->isChecked(),
                                               ui.m_appSpecificDetails->isChecked());
}

void BugAwarenessPage::updateCheckBoxes()
{
    bool rememberSituation = ui.m_rememberCrashSituationYes->isChecked();
    ui.m_reproducibleLabel->setEnabled(rememberSituation);
    ui.m_reproducibleBox->setEnabled(rememberSituation);
    
    ui.m_informationLabel->setEnabled(rememberSituation);
    ui.m_actionsInsideApp->setEnabled(rememberSituation);
    ui.m_unusualSituation->setEnabled(rememberSituation);

    ui.m_appSpecificDetails->setEnabled(rememberSituation);
    ui.m_appSpecificDetailsExamples->setEnabled(rememberSituation);
}

void BugAwarenessPage::showApplicationDetailsExamples()
{
    QToolTip::showText(QCursor::pos(),
                       i18nc("@label examples about information the user can provide",
                             "Examples: %1", reportInterface()->appDetailsExamples()->examples()),
                       this);
}

//END BugAwarenessPage

//BEGIN ConclusionPage

ConclusionPage::ConclusionPage(ReportAssistantDialog * parent)
        : ReportAssistantPage(parent),
        needToReport(false)
{
    isBKO = DrKonqi::crashedApplication()->bugReportAddress().isKdeBugzilla();
    
    ui.setupUi(this);
    
    ui.m_showReportInformationButton->setGuiItem(
                    KGuiItem2(i18nc("@action:button", "Sho&w Contents of the Report"),
                            KIcon("document-preview"),
                            i18nc("@info:tooltip", "Use this button to show the generated "
                            "report information about this crash.")));
    connect(ui.m_showReportInformationButton, SIGNAL(clicked()), this, 
                                                                    SLOT(openReportInformation()));
    ui.m_restartAppOnFinish->setVisible(false);
}

void ConclusionPage::finishClicked()
{
    //Manual report
    if (needToReport && !isBKO) {
        const CrashedApplication *crashedApp = DrKonqi::crashedApplication();
        BugReportAddress reportAddress = crashedApp->bugReportAddress();
        QString report = reportInterface()->generateReport(false);
        
        if (reportAddress.isEmail()) {
            QString subject = QString("Automatic crash report generated by DrKonqi for %1.")
                                    .arg(crashedApp->name());
            KToolInvocation::invokeMailer(reportAddress, "", "" , subject, report);
        } else {
            if (KUrl(reportAddress).isRelative()) { //Scheme is missing
                reportAddress = QLatin1String("http://") + reportAddress;
            }
            KToolInvocation::invokeBrowser(reportAddress);
        }
        
        //Show a copy of the bug reported
        QString info = report + QLatin1Char('\n') +
                            i18nc("@info/plain","Report to %1", reportAddress);

        ReportInformationDialog * m_infoDialog = new ReportInformationDialog(info);
        m_infoDialog->show();
        m_infoDialog->raise();
        m_infoDialog->activateWindow();
    }
    
    //Restart application
    if (ui.m_restartAppOnFinish->isChecked()) {
         DrKonqi::crashedApplication()->restart();
    }
}

void ConclusionPage::aboutToShow()
{
    connect(assistant(), SIGNAL(user1Clicked()), this, SLOT(finishClicked()));
    ui.m_restartAppOnFinish->setVisible(false);
    ui.m_restartAppOnFinish->setChecked(false);

    needToReport = reportInterface()->isWorthReporting();
    emitCompleteChanged();

    BugReportAddress reportAddress = DrKonqi::crashedApplication()->bugReportAddress();
    BacktraceParser::Usefulness use =
                DrKonqi::debuggerManager()->backtraceGenerator()->parser()->backtraceUsefulness();

    QString explanationHTML = QLatin1String("<p><ul>");

    bool backtraceGenerated = true;
    switch (use) {
    case BacktraceParser::ReallyUseful: {
        explanationHTML += QString("<li>%1</li>").arg(i18nc("@info","The automatically generated "
                                                            "crash information is useful."));
        break;
    }
    case BacktraceParser::MayBeUseful: {
        explanationHTML += QString("<li>%1</li>").arg(i18nc("@info","The automatically generated "
                                                            "crash information lacks some "
                                                            "details "
                                                            "but may be still be useful."));
        break;
    }
    case BacktraceParser::ProbablyUseless: {
        explanationHTML += QString("<li>%1</li>").arg(i18nc("@info","The automatically generated "
                                                        "crash information lacks important details "
                                                        "and it is probably not helpful."));
        break;
    }
    case BacktraceParser::Useless:
    case BacktraceParser::InvalidUsefulness: {
        BacktraceGenerator::State state = DrKonqi::debuggerManager()->backtraceGenerator()->state();
        if (state == BacktraceGenerator::NotLoaded) {
            backtraceGenerated = false;
            explanationHTML += QString("<li>%1</li>").arg(i18nc("@info","The crash information was "
                                        "not generated because it was not needed.")); 
        } else {
            explanationHTML += QString("<li>%1<br />%2</li>").arg(
                                        i18nc("@info","The automatically generated crash "
                                        "information does not contain enough information to be "
                                        "helpful."),
                                        i18nc("@info","<note>You can improve it by "
                                        "installing debugging packages and reloading the crash on "
                                        "the Crash Information page. You can get help with the Bug "
                                        "Reporting Guide by clicking on the "
                                        "<interface>Help</interface> button.</note>"));
                                    //but this guide doesn't mention bt packages? that's techbase
                                    //->>and the help guide mention techbase page... 
        }
        break;
    }
    }

    //User can provide enough information
    if (reportInterface()->isBugAwarenessPageDataUseful()) {
        explanationHTML += QString("<li>%1</li>").arg(i18nc("@info","The information you can "
                                                            "provide could be considered helpful."));
    } else {
        explanationHTML += QString("<li>%1</li>").arg(i18nc("@info","The information you can "
                                        "provide is not considered helpful enough in this case."));
    }
    
    explanationHTML += QLatin1String("</ul></p>");
    
    ui.m_explanationLabel->setText(explanationHTML);

    //Hide the "Show contents of the report" button if the backtrace was not generated
    ui.m_showReportInformationButton->setVisible(backtraceGenerated);
    
    if (needToReport) {
        ui.m_conclusionsLabel->setText(QString("<p><strong>%1</strong>").arg(i18nc("@info","This "
                                           "report is considered helpful.")));

        if (isBKO) {
            emitCompleteChanged();
            ui.m_howToProceedLabel->setText(i18nc("@info","This application's bugs are reported "
                                            "to the KDE bug tracking system: click <interface>Next"
                                            "</interface> to start the reporting process. "
                                            "You can manually report at <link>%1</link>",
                                                  reportAddress));

        } else {
            if (!DrKonqi::crashedApplication()->hasBeenRestarted()) {
                ui.m_restartAppOnFinish->setVisible(true);
            }
            
            ui.m_howToProceedLabel->setText(i18nc("@info","This application is not supported in the "
                                                "KDE bug tracking system. Click <interface>"
                                                "Finish</interface> to report this bug to "
                                                "the application maintainer. Also, you can manually "
                                                "report at <link>%1</link>.", reportAddress));

            emit finished(false);
        }

    } else { // (needToReport)
        if (!DrKonqi::crashedApplication()->hasBeenRestarted()) {
            ui.m_restartAppOnFinish->setVisible(true);
        }
        
        ui.m_conclusionsLabel->setText(QString("<p><strong>%1</strong><br />%2</p>").arg(
                            i18nc("@info","This report does not contain enough information for the "
                            "developers, so the automated bug reporting process is not "
                            "enabled for this crash."),
                            i18nc("@info","If you wish, you can go back and change your "
                            "answers. ")));

        //Only mention "manual reporting" if the backtrace was generated.
        //FIXME separate the texts "manual reporting" / "click finish to close"
        //"manual reporting" should be ~"manual report using the contents of the report"....
        //FIXME for 4.5 (workflow, see ToDo)
        if (backtraceGenerated) {
            if (isBKO) {
                ui.m_howToProceedLabel->setText(i18nc("@info","You can manually report this bug "
                                                "at <link>%1</link>. "
                                                "Click <interface>Finish</interface> to close the "
                                                "assistant.",
                                                reportAddress));
            } else {
                ui.m_howToProceedLabel->setText(i18nc("@info","You can manually report this "
                                                  "bug to its maintainer at <link>%1</link>. "
                                                  "Click <interface>Finish</interface> to close the "
                                                  "assistant.", reportAddress));
            }
        }
        emit finished(true);
    }
}

void ConclusionPage::aboutToHide()
{
    assistant()->disconnect(SIGNAL(user1Clicked()), this, SLOT(finishClicked()));
}

void ConclusionPage::openReportInformation()
{
    if (!m_infoDialog) {
        QString info = reportInterface()->generateReport(false) + QLatin1Char('\n') +
                            i18nc("@info/plain report to url/mail address","Report to %1", 
                                  DrKonqi::crashedApplication()->bugReportAddress());

        m_infoDialog = new ReportInformationDialog(info);
    }
    m_infoDialog->show();
    m_infoDialog->raise();
    m_infoDialog->activateWindow();
}

bool ConclusionPage::isComplete()
{
    return (isBKO && needToReport);
}

//END ConclusionPage

//BEGIN ReportInformationDialog

ReportInformationDialog::ReportInformationDialog(const QString & reportText)
    : KDialog()
{
    KGlobal::ref();
    setAttribute(Qt::WA_DeleteOnClose, true);

    setButtons(KDialog::Close | KDialog::User1);
    setDefaultButton(KDialog::Close);
    setCaption(i18nc("@title:window","Contents of the Report"));

    ui.setupUi(mainWidget());
    ui.m_reportInformationBrowser->setPlainText(reportText);

    setButtonGuiItem(KDialog::User1, KGuiItem2(i18nc("@action:button", "&Save to File..."),
                                               KIcon("document-save"),
                                               i18nc("@info:tooltip", "Use this button to save the "
                                               "generated crash report information to "
                                               "a file. You can use this option to report the "
                                               "bug later.")));
    connect(this, SIGNAL(user1Clicked()), this, SLOT(saveReport()));

    setInitialSize(QSize(800, 600));
    KConfigGroup config(KGlobal::config(), "ReportInformationDialog");
    restoreDialogSize(config);
}

ReportInformationDialog::~ReportInformationDialog()
{
    KConfigGroup config(KGlobal::config(), "ReportInformationDialog");
    saveDialogSize(config);
    KGlobal::deref();
}

void ReportInformationDialog::saveReport()
{
    DrKonqi::saveReport(ui.m_reportInformationBrowser->toPlainText(), this);
}

//END ReportInformationDialog

