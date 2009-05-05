/*******************************************************************
* drkonqiassistantpages_base.cpp
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

#include "drkonqiassistantpages_base.h"
#include "drkonqi.h"
#include "krashconf.h"
#include "reportinfo.h"
#include "backtraceparser.h"
#include "backtracegenerator.h"
#include "drkonqi_globals.h"

#include <QtGui/QLabel>
#include <QtGui/QCheckBox>

#include <KToolInvocation>
#include <KIcon>
#include <KMessageBox>

//BEGIN IntroductionPage

IntroductionPage::IntroductionPage(DrKonqiBugReport * parent) :
        DrKonqiAssistantPage(parent)
{
    ui.setupUi(this);
}

//END IntroductionPage

//BEGIN CrashInformationPage

CrashInformationPage::CrashInformationPage(DrKonqiBugReport * parent)
        : DrKonqiAssistantPage(parent)
{
    m_backtraceWidget = new GetBacktraceWidget(DrKonqi::instance()->backtraceGenerator());
    connect(m_backtraceWidget, SIGNAL(stateChanged()) , this, SLOT(emitCompleteChanged()));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(m_backtraceWidget);
    layout->addSpacing(10); //We need this for better usability until we get something better
}

void CrashInformationPage::aboutToShow()
{
    m_backtraceWidget->generateBacktrace();
    m_backtraceWidget->hilightExtraDetailsLabel(false);
    emitCompleteChanged();
}

void CrashInformationPage::aboutToHide()
{
    BacktraceGenerator *btGenerator = DrKonqi::instance()->backtraceGenerator();
    BacktraceParser::Usefulness use = btGenerator->parser()->backtraceUsefulness();

    if (use != BacktraceParser::Useless && use != BacktraceParser::InvalidUsefulness) {
        reportInfo()->setBacktrace(btGenerator->backtrace());
    }
    reportInfo()->setFirstBacktraceFunctions(btGenerator->parser()->firstValidFunctions());
}

bool CrashInformationPage::isComplete()
{
    BacktraceGenerator *generator = DrKonqi::instance()->backtraceGenerator();
    return (generator->state() != BacktraceGenerator::NotLoaded &&
            generator->state() != BacktraceGenerator::Loading);
}

bool CrashInformationPage::showNextPage()
{
    BacktraceParser::Usefulness use =
                        DrKonqi::instance()->backtraceGenerator()->parser()->backtraceUsefulness();

    if (use == BacktraceParser::InvalidUsefulness || use == BacktraceParser::ProbablyUseless
            || use == BacktraceParser::Useless) {
        if ( KMessageBox::Yes == KMessageBox::questionYesNo(this,
                                i18nc("@info","This crash information is not useful enough, "
                                              "do you want to try to improve it? You will need "
                                              "to install some debugging packages."),
                                i18nc("@title:window","Crash Information is not useful enough")) ) {
            m_backtraceWidget->hilightExtraDetailsLabel(true);
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

BugAwarenessPage::BugAwarenessPage(DrKonqiBugReport * parent)
        : DrKonqiAssistantPage(parent)
{
    ui.setupUi(this);
}

void BugAwarenessPage::aboutToHide()
{
    //Save data
    reportInfo()->setUserCanDetail(ui.m_canDetailCheckBox->checkState() == Qt::Checked);
    reportInfo()->setDevelopersCanContactReporter(
           ui.m_developersCanContactReporterCheckBox->checkState() == Qt::Checked);
}

//END BugAwarenessPage

//BEGIN ConclusionPage

ConclusionPage::ConclusionPage(DrKonqiBugReport * parent)
        : DrKonqiAssistantPage(parent),
        needToReport(false)
{
    isBKO = DrKonqi::instance()->krashConfig()->isKDEBugzilla();
    
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
    if (needToReport && !isBKO) {
        const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();

        if (krashConfig->isReportMail()) {
            QString subject = QString("Automatic crash report generated by DrKonqi for %1.")
                                    .arg(krashConfig->productName());
            QString body = reportInfo()->generateReport();
            KToolInvocation::invokeMailer(krashConfig->getReportLink(), "", "" , subject, body);
        } else {
            KToolInvocation::invokeBrowser(krashConfig->getReportLink());
        }
    }
    if (ui.m_restartAppOnFinish->isChecked()) {
         DrKonqi::instance()->restartCrashedApplication();
    }
}

void ConclusionPage::aboutToShow()
{
    connect(assistant(), SIGNAL(user1Clicked()), this, SLOT(finishClicked()));
    ui.m_restartAppOnFinish->setVisible(false);
    ui.m_restartAppOnFinish->setChecked(false);

    needToReport = false;
    emitCompleteChanged();

    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();
    BacktraceParser::Usefulness use =
                DrKonqi::instance()->backtraceGenerator()->parser()->backtraceUsefulness();
    bool canDetails = reportInfo()->userCanDetail();
    bool developersCanContactReporter = reportInfo()->developersCanContactReporter();

    QString explanationHTML = QLatin1String("<p><ul>");
    
    switch (use) {
    case BacktraceParser::ReallyUseful: {
        needToReport = (canDetails || developersCanContactReporter);
        explanationHTML += QString("<li>%1</li>").arg(i18nc("@info","The automatically generated "
                                                            "crash information is useful."));
        break;
    }
    case BacktraceParser::MayBeUseful: {
        needToReport = (canDetails || developersCanContactReporter);
        explanationHTML += QString("<li>%1</li>").arg(i18nc("@info","The automatically generated "
                                                            "crash information lacks some "
                                                            "details "
                                                            "but may be still be useful."));
        break;
    }
    case BacktraceParser::ProbablyUseless: {
        needToReport = (canDetails && developersCanContactReporter);
        explanationHTML += QString("<li>%1</li>").arg(i18nc("@info","The automatically generated "
                                                        "crash information lacks important details "
                                                        "and it is probably not helpful."));
        break;
    }
    case BacktraceParser::Useless:
    case BacktraceParser::InvalidUsefulness: {
        needToReport =  false;
        explanationHTML += QString("<li>%1<br />%2</li>").arg(
                                        i18nc("@info","The automatically generated crash "
                                        "information does not contain enough information to be "
                                        "helpful."),
                                        i18nc("@info","<note>You can improve it by "
                                        "installing some software and reloading the crash on "
                                        "the Crash Information page. You can get help with the Bug "
                                        "Reporting Guide by clicking on the "
                                        "<interface>Help</interface> button.</note>"));
                                    //but this guide doesn't mention bt packages? that's techbase
        break;
    }
    }
    
    //User can provide details / is Willing to help
    if (canDetails) {
        if (developersCanContactReporter) {
            explanationHTML += QString("<li>%1</li>").arg(i18nc("@info","You can provide crash "
                                "details, and the "
                                "developers can contact you if required."));
        } else {
            explanationHTML += QString("<li>%1</li>").arg(i18nc("@info","You can provide crash "
                                "details, but do not want "
                                "developers to contact you for more information if required."));
        }
    } else {
        if (developersCanContactReporter) {
            explanationHTML += QString("<li>%1</li>").arg(i18nc("@info","You are not sure what you "
                                    "were doing when the application crashed, but the developers "
                                    "can contact you for more information if required."));
        } else {
            explanationHTML += QString("<li>%1</li>").arg(i18nc("@info","You are not sure what you "
                                    "were doing when the application crashed, and do not want the " "developers to contact you for more information if required."));
        }
    }
    
    explanationHTML += QLatin1String("</ul></p>");
    
    ui.m_explanationLabel->setText(explanationHTML);

    if (needToReport) {
        ui.m_conclusionsLabel->setText(QString("<p><strong>%1</strong>").arg(i18nc("@info","This "
                                           "report is considered helpful.")));

        if (isBKO) {
            emitCompleteChanged();
            ui.m_howToProceedLabel->setText(i18nc("@info","This application's bugs are reported "
                                            "to the KDE bug tracking system: click <interface>Next"
                                            "</interface> to start the reporting process. "
                                            "You can manually report at <link>%1</link>",
                                                  QLatin1String(KDE_BUGZILLA_URL)));

        } else {
            if (!DrKonqi::instance()->appRestarted()) {
                ui.m_restartAppOnFinish->setVisible(true);
            }
            
            ui.m_howToProceedLabel->setText(i18nc("@info","This application is not supported in the "
                                                "KDE bug tracking system. Click <interface>"
                                                "Finish</interface> to report this bug to "
                                                "the application maintainer. Also, you can manually "
                                                "report at <link>%1</link>.",
                                                krashConfig->getReportLink()));

            emit finished(false);
        }

    } else { // (needToReport)
        if (!DrKonqi::instance()->appRestarted()) {
            ui.m_restartAppOnFinish->setVisible(true);
        }
        
        ui.m_conclusionsLabel->setText(QString("<p><strong>%1</strong><br />%2</p>").arg(
                            i18nc("@info","This report does not contain enough information for the "
                            "developers, so the automated bug reporting process is not "
                            "enabled for this crash."),
                            i18nc("@info","If you wish, you can go back and change your answers. "
                            "Also, you can manually report the crash if you would like to.")));

        if (krashConfig->isKDEBugzilla()) {
            ui.m_howToProceedLabel->setText(i18nc("@info","This application is supported in the KDE "
                                            "bug tracking system. You can manually report this bug "
                                            "at <link>%1</link>. "
                                            "Click <interface>Finish</interface> to close the "
                                            "assistant.",
                                            QLatin1String(KDE_BUGZILLA_URL)));
        } else {
            ui.m_howToProceedLabel->setText(i18nc("@info","This application is not supported in the "
                                              "KDE bug tracking system. You can manually report this "
                                              "bug to its maintainer at <link>%1</link>. "
                                              "Click <interface>Finish</interface> to close the "
                                              "assistant.",
                                              krashConfig->getReportLink()));
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
        QString reportUri;
        if (isBKO) {
            reportUri = QLatin1String(KDE_BUGZILLA_URL);
        } else {
            const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();
            reportUri = krashConfig->getReportLink();
        }
        QString info = reportInfo()->generateReport() + '\n' +
                            i18nc("@info/plain","Report to %1", reportUri);

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
}

void ReportInformationDialog::saveReport()
{
    DrKonqi::saveReport(ui.m_reportInformationBrowser->toPlainText(), this);
}

//END ReportInformationDialog

