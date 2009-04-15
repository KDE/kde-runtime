/*******************************************************************
* drkonqiassistantpages_base.cpp
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

#include "drkonqiassistantpages_base.h"
#include "drkonqi.h"
#include "krashconf.h"
#include "reportinfo.h"
#include "backtraceparser.h"
#include "backtracegenerator.h"
#include "drkonqi_globals.h"

#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>

#include <KPushButton>
#include <KTextBrowser>
#include <KToolInvocation>
#include <KIcon>
#include <KMessageBox>
#include <KLocale>

//BEGIN IntroductionPage

IntroductionPage::IntroductionPage(DrKonqiBugReport * parent) :
        DrKonqiAssistantPage(parent)
{
    QLabel * mainLabel = new QLabel(i18nc("@info","<para>This assistant will analyze the crash "
            "information and guide you through the bug reporting process.</para><para>You can get "
            "help on this bug reporting assistant by clicking the <interface>Help</interface> "
            "button.</para><para>To start gathering the crash information press the "
            "<interface>Next</interface> button.</para>"));
    mainLabel->setWordWrap(true);

    QVBoxLayout * layout = new QVBoxLayout(this);
    layout->addWidget(mainLabel);
    layout->addStretch();
    setLayout(layout);
}

//END IntroductionPage

//BEGIN CrashInformationPage

CrashInformationPage::CrashInformationPage(DrKonqiBugReport * parent)
        : DrKonqiAssistantPage(parent)
{
    m_backtraceWidget = new GetBacktraceWidget(DrKonqi::instance()->backtraceGenerator());

    //connect backtraceWidget signals
    connect(m_backtraceWidget, SIGNAL(stateChanged()) , this, SLOT(emitCompleteChanged()));

    QLabel * subTitle = new QLabel(i18nc("@info","This page will generate some information "
                                      "about your crash. Developers need this in a bug report about crashes."));
    subTitle->setWordWrap(true);
    subTitle->setAlignment(Qt::AlignHCenter);

    QVBoxLayout * layout = new QVBoxLayout(this);
    layout->addWidget(subTitle);
    layout->addWidget(m_backtraceWidget);
    setLayout(layout);
}

void CrashInformationPage::aboutToShow()
{
    m_backtraceWidget->generateBacktrace();
    emitCompleteChanged();
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
                                i18nc("@info","The crash information is not useful enough, "
                                              "do you want to try to improve it?"),
                                i18nc("@title:window","Crash Information is not useful enough")) ) {
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
    //User can detail layout
    QVBoxLayout * canDetailLayout = new QVBoxLayout();

    QLabel * detailQuestionLabel = new QLabel(QString("<strong>%1</strong>").arg(
                                              i18nc("@info","Can you give detailed information "
                                                            "about what you were doing when the "
                                                            "application crashed?")));
    detailQuestionLabel->setWordWrap(true);
    canDetailLayout->addWidget(detailQuestionLabel);

    QLabel * detailLabel = new QLabel(i18nc("@info refers to 'if you can give detailed information'",
                            "If you can, that will help the developers reproduce and fix the bug. "
                            "<emphasis>Examples of Details: steps to take to reproduce the crash, "
                            "what you were doing in the application, whether it happens every time, or "
                            "whether it crashes on a particular URL or document.</emphasis>."));

    detailLabel->setWordWrap(true);
    canDetailLayout->addWidget(detailLabel);

    m_canDetailCheckBox = new QCheckBox(i18nc("@option:check",
                                              "Yes, I can give information about the crash."));
    canDetailLayout->addWidget(m_canDetailCheckBox);

    //User is willing to help layout
    QVBoxLayout * developersCanContactReporterLayout = new QVBoxLayout();

    QLabel * developersCanContactReporterQuestionLabel =
                    new QLabel(QString("<strong>%1</strong>").arg(
                                        i18nc("@info", "Can the developers contact you for more "
                                                        "information?")));
    developersCanContactReporterLayout->addWidget(developersCanContactReporterQuestionLabel);

    QLabel * developersCanContactReporterLabel =
                    new QLabel(i18nc("@info","Sometimes, while the bug is being investigated, "
                                    "the developers need more information from the reporter in "
                                    "order to fix the bug."));
    developersCanContactReporterLabel->setWordWrap(true);
    developersCanContactReporterLayout->addWidget(developersCanContactReporterLabel);

    m_developersCanContactReporterCheckBox = new QCheckBox(i18nc("@option:check",
                                                               "Yes, developers can contact me "
                                                               "for more information if required."));
    developersCanContactReporterLayout->addWidget(m_developersCanContactReporterCheckBox);

    //Main layout
    QVBoxLayout * layout = new QVBoxLayout();
    layout->setSpacing(8);
    layout->addLayout(canDetailLayout);
    layout->addLayout(developersCanContactReporterLayout);
    layout->addStretch();
    setLayout(layout);
}

void BugAwarenessPage::aboutToHide()
{
    //Save data
    reportInfo()->setUserCanDetail(m_canDetailCheckBox->checkState() == Qt::Checked);
    reportInfo()->setDevelopersCanContactReporter(
            m_developersCanContactReporterCheckBox->checkState() == Qt::Checked);
}

//END BugAwarenessPage

//BEGIN ConclusionPage

ConclusionPage::ConclusionPage(DrKonqiBugReport * parent)
        : DrKonqiAssistantPage(parent),
        needToReport(false)
{
    isBKO = DrKonqi::instance()->krashConfig()->isKDEBugzilla();

    m_reportEdit = new KTextBrowser();
    m_reportEdit->setReadOnly(true);

    m_saveReportButton = new KPushButton(
                    KGuiItem2(i18nc("@action:button", "&Save to File"),
                            KIcon("document-save"),
                            i18nc("@info:tooltip", "Use this button to save the generated "
                            "report and conclusions about this crash to a file. You can use "
                            "this option to report the bug later.")));
    connect(m_saveReportButton, SIGNAL(clicked()), this, SLOT(saveReport()));

    m_reportButton = new KPushButton(
                    KGuiItem2(i18nc("@action:button", "&Report bug to the application maintainer"),
                              KIcon("document-new"),
                              i18nc("@info:tooltip", "Use this button to report this bug to "
                                    "the application maintainer. This will launch the browser or "
                                    "the mail client using the assigned bug report address.")));
    m_reportButton->setVisible(false);
    connect(m_reportButton, SIGNAL(clicked()), this , SLOT(reportButtonClicked()));

    m_explanationLabel = new QLabel();
    m_explanationLabel->setWordWrap(true);

    QHBoxLayout *bLayout = new QHBoxLayout();
    bLayout->addStretch();
    bLayout->addWidget(m_saveReportButton);
    bLayout->addWidget(m_reportButton);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(10);
    layout->addWidget(m_reportEdit);
    layout->addWidget(m_explanationLabel);
    layout->addLayout(bLayout);
    setLayout(layout);
}

void ConclusionPage::saveReport()
{
    DrKonqi::saveReport(m_reportEdit->toPlainText(), this);
}

void ConclusionPage::reportButtonClicked()
{
    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();

    if (krashConfig->isReportMail()) {
        QString subject = QString("Automatic crash report generated by DrKonqi for %1.")
                                 .arg(krashConfig->productName());
        QString body = reportInfo()->generateReportBugzilla();
        KToolInvocation::invokeMailer(krashConfig->getReportLink(), "", "" , subject, body);
    } else {
        KToolInvocation::invokeBrowser(krashConfig->getReportLink());
    }

    emit finished(false);
}

void ConclusionPage::aboutToShow()
{
    needToReport = false;
    emitCompleteChanged();

    QString conclusionsHTML = QLatin1String("<p>");

    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();
    BacktraceParser::Usefulness use =
                DrKonqi::instance()->backtraceGenerator()->parser()->backtraceUsefulness();
    bool canDetails = reportInfo()->getUserCanDetail();
    bool developersCanContactReporter = reportInfo()->getDevelopersCanContactReporter();

    // Calculate needToReport
    conclusionsHTML = QLatin1String("<ul>");
    
    switch (use) {
    case BacktraceParser::ReallyUseful: {
        needToReport = (canDetails || developersCanContactReporter);
        conclusionsHTML += QString("<li>%1</li>").arg(i18nc("@info","The automatically generated "
                                                              "crash information is very useful."));
        break;
    }
    case BacktraceParser::MayBeUseful: {
        needToReport = (canDetails || developersCanContactReporter);
        conclusionsHTML += QString("<li>%1</li>").arg(i18nc("@info","The automatically generated "
                                                            "crash information lacks some details "
                                                            "but may be still be useful."));
        break;
    }
    case BacktraceParser::ProbablyUseless: {
        needToReport = (canDetails && developersCanContactReporter);
        conclusionsHTML += QString("<li>%1</li>").arg(i18nc("@info","The automatically generated "
                                                        "crash information lacks important details "
                                                        "and it is probably not very useful."));
                                                        //should we add "use your judgment"?
        break;
    }
    case BacktraceParser::Useless:
    case BacktraceParser::InvalidUsefulness: {
        needToReport =  false;
        conclusionsHTML += QString("<li>%1</li>").arg(i18nc("@info","The automatically generated "
                                                               "crash information is not useful enough."));
        conclusionsHTML += QLatin1String("<br />");
        conclusionsHTML += i18nc("@info","<note>You can try to improve this by installing some "
                                "packages and reloading the information on the Crash Information "
                                "page. You can read the Bug Reporting Guide by clicking on the "
                                "<interface>Help</interface> button.</note>");
                                //but this guide doesn't mention bt packages? that's techbase
        break;
    }
    }
    
    //User can provide details / is Willing to help
    //conclusionsHTML.append(QLatin1String("<br />"));
    if (canDetails) {
        if (developersCanContactReporter) {
            conclusionsHTML += QString("<li>%1</li>").arg(i18nc("@info","You can explain in detail "
                                "what you were doing when the application crashed and the "
                                "developers can contact you for more information if required."));
        } else {
            conclusionsHTML += QString("<li>%1</li>").arg(i18nc("@info","You can explain in detail "
                                "what you were doing when the application crashed but the "
                                "developers cannot contact you for more information if required."));
        }
    } else {
        if (developersCanContactReporter) {
            conclusionsHTML += QString("<li>%1</li>").arg(i18nc("@info","You are not sure what you "
                                    "were doing when the application crashed but the developers "
                                    "can contact you for more information if required."));
        } else {
            conclusionsHTML += QString("<li>%1</li>").arg(i18nc("@info","You are not sure what you "
                                    "were doing when the application crashed and the developers "
                                    "cannot contact you for more information if required."));
        }
    }
    
    conclusionsHTML += QLatin1String("</ul></p>");
    
    if (needToReport) {
        conclusionsHTML += QString("<p><strong>%1</strong>").arg(i18nc("@info","This is considered "
                                                        "helpful for the application developers."));

        QString reportMethod;
        QString reportLink;

        if (isBKO) {
            m_reportButton->setVisible(false);

            emitCompleteChanged();
            m_explanationLabel->setText(i18nc("@info","This application's bugs are reported "
                                            "to the KDE bug tracking system: press <interface>Next"
                                            "</interface> to start the reporting process."));
                                            // string needs work: what assistant were they in, then?

            reportMethod = i18nc("@info","You need to file a new bug report, "
                                "press <interface>Next</interface> to start the report assistant.");
            reportLink = i18nc("@info", "You can manually report at <link>%1</link>",
                               QLatin1String(KDE_BUGZILLA_URL));
        } else {
            m_reportButton->setVisible(true);

            m_explanationLabel->setText(i18nc("@info","This application is not supported in the "
                                                "KDE bug tracking system. You can report this bug "
                                                "to its maintainer at <link>%1</link>.",
                                                krashConfig->getReportLink()));

            reportMethod = i18nc("@info","You need to file a new bug report "
                                         "with the following information:");
            reportLink =  i18nc("@info", "You can manually report at <link>%1</link>.",
                                 krashConfig->getReportLink());

            if (krashConfig->isReportMail()) {
                m_reportButton->setGuiItem(
                            KGuiItem2(i18nc("@action:button",
                                            "Send an e-mail to the application maintainer"),
                                      KIcon("mail-message-new"),
                                      i18nc("@info:tooltip","Launches the mail client to send "
                                            "an e-mail to the application maintainer with the "
                                            "crash information.")));
            } else {
                m_reportButton->setGuiItem(
                            KGuiItem2(i18nc("@action:button",
                                            "Open the application maintainer's web site"),
                                      KIcon("internet-web-browser"),
                                      i18nc("@info:tooltip","Launches the web browser showing "
                                            "the application's web site in order to contact "
                                            "the maintainer.")));
            }
        }

        conclusionsHTML += QString("<br />%1</p>").arg(reportMethod);
        conclusionsHTML += QString("<p>%1</p>").arg(reportInfo()->generateReportHtml());
        conclusionsHTML += QString("<p>%1</p>").arg(reportLink);
    } else { // (needToReport)
        m_reportButton->setVisible(false);

        conclusionsHTML += QString("<p><strong>%1</strong><br />%2</p>").arg(
                            i18nc("@info","This is not considered helpful enough for the application "
                            "developers and therefore the automated bug reporting process is not "
                            "enabled for this crash."),
                            i18nc("@info","If you change your mind, you can go back and review the "
                            "assistant questions. Also, you can manually report it on your own if "
                            "you would like to. Include the following information: "));

        conclusionsHTML += QString("<p>%1</p>").arg(reportInfo()->generateReportHtml());

        conclusionsHTML.append(QString("<p>"));
        if (krashConfig->isKDEBugzilla()) {
            conclusionsHTML += i18nc("@info", "Report at <link>%1</link>",
                                     QLatin1String(KDE_BUGZILLA_URL));
            m_explanationLabel->setText(i18nc("@info","This application is supported in the KDE "
                                              "bug tracking system. You can report this bug at "
                                              "<link>%1</link>.", QLatin1String(KDE_BUGZILLA_URL)));
        } else {
            conclusionsHTML += i18nc("@info", "Report at <link>%1</link>",
                                      krashConfig->getReportLink());
            m_explanationLabel->setText(i18nc("@info","This application is not supported in the "
                                              "KDE bug tracking system. You can report this bug "
                                              "to its maintainer at <link>%1</link>.",
                                              krashConfig->getReportLink()));
        }
        conclusionsHTML.append(QString("</p>"));
        
        emit finished(true);
    }

    m_reportEdit->setHtml(conclusionsHTML);
}

bool ConclusionPage::isComplete()
{
    return (isBKO && needToReport);
}

//END ConclusionPage
