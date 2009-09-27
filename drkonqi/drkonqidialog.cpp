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

    //Main widget components
    QLabel * title = new QLabel(i18nc("@info", "<para>We are sorry, <application>%1</application> "
                                               "closed unexpectedly.</para>", crashedApp->name()));
    title->setWordWrap(true);

    QString reportMessage;
    if (!crashedApp->bugReportAddress().isEmpty()) {
        if (KCmdLineArgs::parsedArgs()->isSet("safer")) {
            reportMessage = i18nc("@info", "<para>The reporting assistant is disabled because "
                                           "the crash handler dialog was started in safe mode."
                                           "<nl />You can manually report this bug to %1 "
                                           "(including the backtrace from the Developer Information "
                                           "tab)</para>", crashedApp->bugReportAddress());
        } else {
            reportMessage = i18nc("@info", "<para>You can help us improve KDE by reporting "
                                          "this error.<nl /><link url='#aboutbugreporting'>Learn "
                                          "more about bug reporting.</link></para><para><note>It is "
                                          "safe to close this dialog if you do not want to report "
                                          "this bug.</note></para>");
        }
    } else {
        reportMessage = i18nc("@info", "<para>You can't report this error because the "
                                        "application does not provide a bug reporting "
                                        "address</para>");
    }
    QLabel * infoLabel = new QLabel(reportMessage);

    connect(infoLabel, SIGNAL(linkActivated(QString)), this, SLOT(aboutBugReporting()));
    infoLabel->setWordWrap(true);

    //Main widget layout
    QVBoxLayout * introLayout = new QVBoxLayout;
    introLayout->addSpacing(20);

    QHBoxLayout * horizontalLayout = new QHBoxLayout();

    QVBoxLayout * verticalTextLayout = new QVBoxLayout();
    verticalTextLayout->addWidget(title);
    verticalTextLayout->addWidget(infoLabel);
    horizontalLayout->addLayout(verticalTextLayout);

    QLabel * iconLabel = new QLabel();
    iconLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    iconLabel->setPixmap(QPixmap(KStandardDirs::locate("appdata", QLatin1String("pics/crash.png"))));
    horizontalLayout->addWidget(iconLabel);
    horizontalLayout->setAlignment(iconLabel,Qt::AlignTop);

    introLayout->addLayout(horizontalLayout);
    introLayout->addStretch();

    //Details
    introLayout->addWidget(new QLabel(QString("<strong>%1</strong>").arg(i18nc("@label","Details:"))));

    QLabel * detailsLabel = new QLabel(i18nc("@info","<para>Executable: <application>%1"
                                            "</application> PID: <numid>%2</numid> Signal: %3 (%4)"
                                            "</para>", crashedApp->executable().fileName(),
                                             crashedApp->pid(), crashedApp->signalNumber(),
                                             crashedApp->signalName()));
    detailsLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    introLayout->addWidget(detailsLabel);

    //Introduction widget
    m_introWidget = new QWidget(this);
    m_introWidget->setLayout(introLayout);
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
    enableButton(KDialog::User1, !crashedApp->bugReportAddress().isEmpty() &&
                                 !KCmdLineArgs::parsedArgs()->isSet("safer"));
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
    enableButton(KDialog::User3, !crashedApp->hasBeenRestarted());
    connect(this, SIGNAL(user3Clicked()), this, SLOT(restartApplication()));

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

void DrKonqiDialog::restartApplication()
{
    enableButton(KDialog::User3, false);
    DrKonqi::crashedApplication()->restart();
}
