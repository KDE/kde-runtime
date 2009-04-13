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
#include "getbacktracewidget.h"
#include "drkonqibugreport.h"
#include "aboutbugreportingdialog.h"
#include "krashconf.h"
#include "drkonqi_globals.h"

#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QTreeWidget>
#include <QtGui/QMenu>

#include <KIcon>
#include <KStandardDirs>
#include <KLocale>
#include <KTabWidget>

DrKonqiDialog::DrKonqiDialog(QWidget * parent) :
        KDialog(parent),
        m_aboutBugReportingDialog(0),
        m_backtraceWidget(0)
{
    connect(DrKonqi::instance(), SIGNAL(debuggerRunning(bool)), this, SLOT(enableDebugMenu(bool)));
    connect(DrKonqi::instance(), SIGNAL(newDebuggingApplication(QString)),
            this, SLOT(slotNewDebuggingApp(QString)));

    //Setting dialog title and icon
    setCaption(DrKonqi::instance()->krashConfig()->programName());
    setWindowIcon(KIcon("tools-report-bug"));

    m_tabWidget = new KTabWidget(this);
    //m_tabWidget->setMinimumSize( QSize(600, 300) );
    //m_tabWidget->setTabPosition( QTabWidget::South );
    setMainWidget(m_tabWidget);

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabIndexChanged(int)));

    buildMainWidget();
    m_tabWidget->addTab(m_introWidget, i18nc("tab header", "&General"));

    m_backtraceWidget = new GetBacktraceWidget(DrKonqi::instance()->backtraceGenerator());
    m_backtraceWidget->setMinimumSize(QSize(575, 240));
    m_backtraceWidget->layout()->setContentsMargins(5, 5, 5, 5);
    m_tabWidget->addTab(m_backtraceWidget, i18nc("tab header", "&Developer Information"));

    buildDialogOptions();
    //setButtonsOrientation( Qt::Vertical );

    resize(minimumSize());
}

void DrKonqiDialog::tabIndexChanged(int index)
{
    if (index == 1)
        m_backtraceWidget->generateBacktrace();
}

void DrKonqiDialog::buildMainWidget()
{
    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();

    //Main widget components
    QLabel * title = new QLabel(i18nc("applicationName closed unexpectedly",
                                      "<para>We are sorry, %1 closed unexpectedly.</para>",
                                      krashConfig->programName()));
    title->setWordWrap(true);

    QLabel * infoLabel = new QLabel(i18nc("Small explanation of the crash cause",
                                          "<para>You can help us improve KDE by reporting this "
                                          "error.<nl /><a href=\"#aboutbugreporting\">Learn more "
                                          "about bug reporting</a></para><para>It is safe to close "
                                          "this dialog if you do not want to report</para>"));
    connect(infoLabel, SIGNAL(linkActivated(QString)), this, SLOT(aboutBugReporting()));
    infoLabel->setWordWrap(true);

    //Main widget layout
    QVBoxLayout * introLayout = new QVBoxLayout;
    introLayout->setSpacing(10);
    introLayout->setContentsMargins(8, 8, 8, 8);

    QHBoxLayout * horizontalLayout = new QHBoxLayout();

    QVBoxLayout * verticalTextLayout = new QVBoxLayout();
    verticalTextLayout->addWidget(title);
    verticalTextLayout->addWidget(infoLabel);
    horizontalLayout->addLayout(verticalTextLayout);

    QLabel * iconLabel = new QLabel();
    iconLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    iconLabel->setPixmap(QPixmap(KStandardDirs::locate("appdata", QLatin1String("pics/crash.png"))));
    horizontalLayout->addWidget(iconLabel);

    introLayout->addLayout(horizontalLayout);
    introLayout->addStretch();

    //Details
    introLayout->addWidget(new QLabel("<strong>Details:</strong>"));
    introLayout->addWidget(new QLabel(i18n("<para>Executable: %1 PID: %2 Signal: %3 (%4)</para>",
                                           krashConfig->appName(), krashConfig->pid(),
                                           krashConfig->signalNumber(), krashConfig->signalName())));

    //Introduction widget
    m_introWidget = new QWidget(this);
    m_introWidget->setLayout(introLayout);
}

void DrKonqiDialog::buildDialogOptions()
{
    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();

    //Set kdialog buttons
    setButtons(KDialog::User1 | KDialog::User2 | KDialog::User3 | KDialog::Close);

    //Report bug button button
    setButtonGuiItem(KDialog::User1, KGuiItem2(i18nc("Button action", "Report Bug"),
                                              KIcon("tools-report-bug"),
                                              i18nc("help text", "Starts the bug report assistant")));
    connect(this, SIGNAL(user1Clicked()), this, SLOT(reportBugAssistant()));

    //Default debugger button and menu (only for developer mode)
    setButtonGuiItem(KDialog::User2, KGuiItem2(i18nc("Button action", "Debug"),
                                               KIcon("applications-development"),
                                               i18nc("help text", "Starts a program to debug "
                                                     "the crashed application")));
    showButton(KDialog::User2, krashConfig->showDebugger());

    m_debugMenu = new QMenu();
    setButtonMenu(KDialog::User2, m_debugMenu);

    m_defaultDebugAction = new QAction(KIcon("applications-development"),
                                       i18nc("action. 1 is the debugger name", "Debug using %1",
                                              krashConfig->debuggerName()), m_debugMenu);
    connect(m_defaultDebugAction, SIGNAL(triggered()),
            DrKonqi::instance(), SLOT(startDefaultExternalDebugger()));

    m_customDebugAction = new QAction(KIcon("applications-development"),
                                      i18n("Debug in custom application"), m_debugMenu);
    connect(m_customDebugAction, SIGNAL(triggered()),
            DrKonqi::instance(), SLOT(startCustomExternalDebugger()));
    m_customDebugAction->setEnabled(false);
    m_customDebugAction->setVisible(false);

    m_debugMenu->addAction(m_defaultDebugAction);
    m_debugMenu->addAction(m_customDebugAction);

    //Restart application button
    setButtonGuiItem(KDialog::User3, KGuiItem2(i18nc("button action", "Restart Application"),
                                               KIcon("system-restart"),
                                               i18nc("button help", "Use this button to restart "
                                                     "the crashed application")));
    connect(this, SIGNAL(user3Clicked()), this, SLOT(restartApplication()));

    //Close button
    QString tooltipText = i18nc("close button tooltip text",
                                "Close this dialog (you will lose the crash information)");
    setButtonToolTip(KDialog::Close, tooltipText);
    setButtonWhatsThis(KDialog::Close, tooltipText);
    setDefaultButton(KDialog::Close);
    setButtonFocus(KDialog::Close);

}

void DrKonqiDialog::enableDebugMenu(bool debuggerRunning)
{
    enableButton(KDialog::User2, !debuggerRunning);
    m_defaultDebugAction->setEnabled(!debuggerRunning);
    m_customDebugAction->setEnabled(!debuggerRunning);
}

void DrKonqiDialog::reportBugAssistant()
{
    DrKonqiBugReport * m_bugReportAssistant = new DrKonqiBugReport();
    close();
    m_bugReportAssistant->show();
}

void DrKonqiDialog::aboutBugReporting()
{
    if (!m_aboutBugReportingDialog) {
        m_aboutBugReportingDialog = new AboutBugReportingDialog(this);
    }
    m_aboutBugReportingDialog->show();
}

void DrKonqiDialog::restartApplication()
{
    enableButton(KDialog::User3, false);
    DrKonqi::instance()->restartCrashedApplication();
}

void DrKonqiDialog::slotNewDebuggingApp(const QString & app)
{
    m_customDebugAction->setVisible(true);
    m_customDebugAction->setEnabled(true);
    m_customDebugAction->setText(app);
}

DrKonqiDialog::~DrKonqiDialog()
{
    delete m_aboutBugReportingDialog;
    delete m_backtraceWidget;
    delete m_debugMenu;
}
