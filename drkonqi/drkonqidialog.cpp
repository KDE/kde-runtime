/*******************************************************************
* drkonqidialog.cpp
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

#include "drkonqidialog.h"

#include "drkonqi.h"
#include "backtracewidget.h"
#include "reportassistantdialog.h"
#include "aboutbugreportingdialog.h"
#include "crashedapplication.h"
#include "debuggermanager.h"
#include "debuggerlaunchers.h"
#include "drkonqi_globals.h"

#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QTreeWidget>

#include <KMenu>
#include <KIcon>
#include <KStandardDirs>
#include <KLocale>
#include <KTabWidget>
#include <KDebug>
#include <KCmdLineArgs>

DrKonqiDialog::DrKonqiDialog(QWidget * parent) :
        KDialog(parent),
        m_aboutBugReportingDialog(0),
        m_backtraceWidget(0)
{
    KGlobal::ref();
    setAttribute(Qt::WA_DeleteOnClose, true);

    //Setting dialog title and icon
    setCaption(DrKonqi::crashedApplication()->name());
    setWindowIcon(KIcon("tools-report-bug"));

    m_tabWidget = new KTabWidget(this);
    setMainWidget(m_tabWidget);

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabIndexChanged(int)));

    buildMainWidget();
    m_tabWidget->addTab(m_introWidget, i18nc("@title:tab general information", "&General"));

    m_backtraceWidget = new BacktraceWidget(DrKonqi::debuggerManager()->backtraceGenerator(), this);
    m_backtraceWidget->setMinimumSize(QSize(575, 240));
    m_tabWidget->addTab(m_backtraceWidget, i18nc("@title:tab", "&Developer Information"));

    buildDialogOptions();

    resize(minimumSize());
    KConfigGroup config(KGlobal::config(), "General");
    restoreDialogSize(config);
}

DrKonqiDialog::~DrKonqiDialog()
{
    KConfigGroup config(KGlobal::config(), "General");
    saveDialogSize(config);
    
    KGlobal::deref();
}

void DrKonqiDialog::tabIndexChanged(int index)
{
    if (index == m_tabWidget->indexOf(m_backtraceWidget)) {
        m_backtraceWidget->generateBacktrace();
    }
}

void DrKonqiDialog::buildMainWidget()
{
    const CrashedApplication *crashedApp = DrKonqi::crashedApplication();

    m_introWidget = new QWidget(this);
    ui.setupUi(m_introWidget);
    
    ui.titleLabel->setText(i18nc("@info", "<para>We are sorry, <application>%1</application> "
                                               "closed unexpectedly.</para>", crashedApp->name()));

    QString reportMessage;
    if (!crashedApp->bugReportAddress().isEmpty()) {
        if (crashedApp->fakeExecutableBaseName() == QLatin1String("drkonqi")) { //Handle own crashes
            reportMessage = i18nc("@info", "<para>As the Crash Handler itself has failed, the "
                                           "automatic reporting process is disabled to reduce the "
                                           "risks of failing again.<nl /><nl />"
                                           "Please, manually report this error in "
                                           "the \"drkonqi\" product at %1. Do not forget to include "
                                           "the backtrace from the Developers Information tab.</para>",
                                           QLatin1String(KDE_BUGZILLA_URL));
        } else if (KCmdLineArgs::parsedArgs()->isSet("safer")) {
            reportMessage = i18nc("@info", "<para>The reporting assistant is disabled because "
                                           "the crash handler dialog was started in safe mode."
                                           "<nl />You can manually report this bug to %1 "
                                           "(including the backtrace from the Developer Information "
                                           "tab.)</para>", crashedApp->bugReportAddress());
        } else {
            reportMessage = i18nc("@info", "<para>You can help us improve KDE Software by reporting "
                                          "this error.<nl /><link url='#aboutbugreporting'>Learn "
                                          "more about bug reporting.</link></para><para><note>It is "
                                          "safe to close this dialog if you do not want to report "
                                          "this bug.</note></para>");
        }
    } else {
        reportMessage = i18nc("@info", "<para>You cannot report this error, because the "
                                        "application does not provide a bug reporting "
                                        "address.</para>");
    }
    ui.infoLabel->setText(reportMessage);
    connect(ui.infoLabel, SIGNAL(linkActivated(QString)), this, SLOT(aboutBugReporting()));

    ui.iconLabel->setPixmap(
                        QPixmap(KStandardDirs::locate("appdata", QLatin1String("pics/crash.png"))));

    ui.detailsTitleLabel->setText(QString("<strong>%1</strong>").arg(i18nc("@label","Details:")));

    ui.detailsLabel->setText(i18nc("@info","<para>Executable: <application>%1"
                                            "</application> PID: <numid>%2</numid> Signal: %3 (%4)"
                                            "</para>", crashedApp->executable().fileName(),
                                             crashedApp->pid(),
                                             crashedApp->signalName(),
                                    #if defined(Q_OS_UNIX)
                                             crashedApp->signalNumber()
                                    #else
                                             //windows uses weird big numbers for exception codes,
                                             //so it doesn't make sense to display them in decimal
                                             QString().sprintf("0x%8x", crashedApp->signalNumber())
                                    #endif
                                             ));
}

void DrKonqiDialog::buildDialogOptions()
{
    const CrashedApplication *crashedApp = DrKonqi::crashedApplication();

    //Set kdialog buttons
    setButtons(KDialog::User1 | KDialog::User2 | KDialog::User3 | KDialog::Close);

    //Report bug button button
    setButtonGuiItem(KDialog::User1, KGuiItem2(i18nc("@action:button", "Report Bug"),
                                               KIcon("tools-report-bug"),
                                               i18nc("@info:tooltip",
                                                     "Starts the bug report assistant.")));

    bool enableReportAssistant = !crashedApp->bugReportAddress().isEmpty() &&
                                 crashedApp->fakeExecutableBaseName() != QLatin1String("drkonqi") &&
                                 !KCmdLineArgs::parsedArgs()->isSet("safer");
    enableButton(KDialog::User1, enableReportAssistant);
    connect(this, SIGNAL(user1Clicked()), this, SLOT(reportBugAssistant()));

    //Default debugger button and menu (only for developer mode)
    DebuggerManager *debuggerManager = DrKonqi::debuggerManager();
    setButtonGuiItem(KDialog::User2, KGuiItem2(i18nc("@action:button this is the debug menu button "
                                               "label which contains the debugging applications", 
                                               "Debug"), KIcon("applications-development"),
                                               i18nc("@info:tooltip", "Starts a program to debug "
                                                     "the crashed application.")));
    showButton(KDialog::User2, debuggerManager->showExternalDebuggers());

    m_debugMenu = new KMenu(this);
    setButtonMenu(KDialog::User2, m_debugMenu);

    QList<AbstractDebuggerLauncher*> debuggers = debuggerManager->availableExternalDebuggers();
    foreach(AbstractDebuggerLauncher *launcher, debuggers) {
        addDebugger(launcher);
    }

    connect(debuggerManager, SIGNAL(externalDebuggerAdded(AbstractDebuggerLauncher*)),
            SLOT(addDebugger(AbstractDebuggerLauncher*)));
    connect(debuggerManager, SIGNAL(externalDebuggerRemoved(AbstractDebuggerLauncher*)),
            SLOT(removeDebugger(AbstractDebuggerLauncher*)));
    connect(debuggerManager, SIGNAL(debuggerRunning(bool)), SLOT(enableDebugMenu(bool)));

    //Restart application button
    setButtonGuiItem(KDialog::User3, KGuiItem2(i18nc("@action:button", "Restart Application"),
                                               KIcon("system-reboot"),
                                               i18nc("@info:tooltip", "Use this button to restart "
                                                     "the crashed application.")));
    enableButton(KDialog::User3, !crashedApp->hasBeenRestarted() &&
                                 crashedApp->fakeExecutableBaseName() != QLatin1String("drkonqi"));
    connect(this, SIGNAL(user3Clicked()), crashedApp, SLOT(restart()));
    connect(crashedApp, SIGNAL(restarted()), this, SLOT(applicationRestarted()));

    //Close button
    QString tooltipText = i18nc("@info:tooltip",
                                "Close this dialog (you will lose the crash information.)");
    setButtonToolTip(KDialog::Close, tooltipText);
    setButtonWhatsThis(KDialog::Close, tooltipText);
    setDefaultButton(KDialog::Close);
    setButtonFocus(KDialog::Close);
}

void DrKonqiDialog::addDebugger(AbstractDebuggerLauncher *launcher)
{
    QAction *action = new QAction(KIcon("applications-development"),
                                  i18nc("@action:inmenu 1 is the debugger name",
                                        "Debug in <application>%1</application>",
                                        launcher->name()), m_debugMenu);
    m_debugMenu->addAction(action);
    connect(action, SIGNAL(triggered()), launcher, SLOT(start()));
    m_debugMenuActions.insert(launcher, action);
}

void DrKonqiDialog::removeDebugger(AbstractDebuggerLauncher *launcher)
{
    QAction *action = m_debugMenuActions.take(launcher);
    if ( action ) {
        m_debugMenu->removeAction(action);
        action->deleteLater();
    } else {
        kError() << "Invalid launcher";
    }
}

void DrKonqiDialog::enableDebugMenu(bool debuggerRunning)
{
    enableButton(KDialog::User2, !debuggerRunning);
}

void DrKonqiDialog::reportBugAssistant()
{
    ReportAssistantDialog * m_bugReportAssistant = new ReportAssistantDialog();
    close();
    m_bugReportAssistant->show();
}

void DrKonqiDialog::aboutBugReporting()
{
    if (!m_aboutBugReportingDialog) {
        m_aboutBugReportingDialog = new AboutBugReportingDialog();
        connect(this, SIGNAL(destroyed(QObject*)), m_aboutBugReportingDialog, SLOT(close()));
    }
    m_aboutBugReportingDialog->show();
    m_aboutBugReportingDialog->raise();
    m_aboutBugReportingDialog->activateWindow();
}

void DrKonqiDialog::applicationRestarted()
{
    enableButton(KDialog::User3, false);
}
