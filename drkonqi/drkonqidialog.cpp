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

#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QTreeWidget>
#include <QtGui/QMenu>

#include <kicon.h>
#include <kpushbutton.h>
#include <ktextbrowser.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <ktabwidget.h>

DrKonqiDialog::DrKonqiDialog( QWidget * parent ) :
    KDialog(parent), 
    m_aboutBugReportingDialog(0),
    m_backtraceWidget(0)
{
    connect( DrKonqi::instance(), SIGNAL(debuggerRunning(bool)), this, SLOT(enableDebugMenu(bool)) );
    //connect( krashConfig, SIGNAL(newDebuggingApplication(QString)), SLOT(slotNewDebuggingApp(QString))); //FIXME
    
    //Setting dialog title and icon
    setCaption( DrKonqi::instance()->krashConfig()->programName() );
    setWindowIcon( KIcon("tools-report-bug") );

    m_tabWidget = new KTabWidget( this );
    m_tabWidget->setMinimumSize( QSize(600, 300) );
    setMainWidget( m_tabWidget );
    
    connect( m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabIndexChanged(int)) );
    
    buildMainWidget();
    m_tabWidget->addTab( m_introWidget, "General" );
    
    m_backtraceWidget = new GetBacktraceWidget(DrKonqi::instance()->backtraceGenerator());
    m_backtraceWidget->layout()->setContentsMargins( 5,5,5,5 );
    m_tabWidget->addTab( m_backtraceWidget, "Advanced (Backtrace)" );
    
    buildDialogOptions();
    
    resize( minimumSize() );
}

void DrKonqiDialog::tabIndexChanged( int index )
{
    if( index == 1 )
        m_backtraceWidget->generateBacktrace();
}

void DrKonqiDialog::buildMainWidget()
{
    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();
    
    //Main widget components
    QLabel * title = new QLabel( i18nc( "applicationName closed unexpectedly", "<title>%1 closed unexpectedly</title>", krashConfig->programName() ) );
    title->setWordWrap( true ); 
    
    QLabel * infoLabel = new QLabel( i18nc("Small explanation of the crash cause",
    "<para>This probably happened because there is a bug in the application.</para><para>You can help us improve KDE by reporting this crash. <link url=\"#aboutbugreporting\">Learn more about bug reporting</link>.</para><para><strong>Note:</strong> It is safe to close this dialog. If you do not want to, you do not have to file a bug report.</para>" ) );
    connect( infoLabel, SIGNAL(linkActivated(QString)), this, SLOT(aboutBugReporting(QString)));
    infoLabel->setWordWrap( true ); 
    
    //Main widget layout
    QVBoxLayout * introLayout = new QVBoxLayout;
    introLayout->setSpacing( 10 );
    introLayout->setContentsMargins( 10, 10, 10, 10 );
    introLayout->addWidget( title );
    introLayout->addWidget( infoLabel );
    introLayout->addStretch();

    //Details
    introLayout->addWidget( new QLabel("<strong>Details:</strong>") );
    introLayout->addWidget( new QLabel(  i18n( "<ul><li>Executable: %1</li><li>PID: %2</li><li>Signal: %3 (%4)</li></ul>",krashConfig->appName(),QString::number(krashConfig->pid() ), krashConfig->signalNumber(), krashConfig->signalName() ) ) );
    
    //Introduction widget
    m_introWidget = new QWidget( this );
    m_introWidget->setLayout( introLayout );
    
    QString styleSheet = QString( ".QWidget {"
                       "background-image: url(%1);"
                       "background-repeat: no-repeat;"
                       "background-position: right;"
                       "} ").arg( KStandardDirs::locate( "appdata", QLatin1String( "pics/konqi.png" ) ) );
    //m_introWidget->setStyleSheet( styleSheet );
}

void DrKonqiDialog::buildDialogOptions()
{
    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();
    
    //Set kdialog buttons
    setButtons( KDialog::User1 | KDialog::User2 | KDialog::User3 | KDialog::Close );
    
    //Report bug button button
    setButtonGuiItem( KDialog::User1, KGuiItem( i18nc("Button action", "Report Bug"), KIcon("tools-report-bug"), i18nc("help text", "Starts the bug report assistant" ), i18nc("help text", "Starts the bug report assistant" ) ) );
    connect( this, SIGNAL(user1Clicked()), this, SLOT(reportBugAssistant()) );
    
    //Default debugger button and menu (only for developer mode)
    setButtonGuiItem( KDialog::User2, KGuiItem( i18nc("Button action", "Debug") , KIcon("applications-development"), i18nc("help text", "Starts a program to debug the crashed application" ), i18nc("help text", "Starts a program to debug the crashed application" ) ) );
    showButton( KDialog::User2, krashConfig->showDebugger() );
    
    m_debugMenu = new QMenu();
    setButtonMenu( KDialog::User2, m_debugMenu );
    
    m_defaultDebugAction = new QAction( KIcon("applications-development"), "Debug in default application", m_debugMenu );
    connect( m_defaultDebugAction, SIGNAL(triggered()), DrKonqi::instance(), SLOT(startDefaultExternalDebugger()) );
    
    m_customDebugAction = new QAction( KIcon("applications-development"), "Debug in custom application", m_debugMenu );
    connect( m_customDebugAction, SIGNAL(triggered()), DrKonqi::instance(), SLOT(startCustomExternalDebugger()) );
    m_customDebugAction->setEnabled( false );
    m_customDebugAction->setVisible( false );
    
    m_debugMenu->addAction( m_defaultDebugAction );
    m_debugMenu->addAction( m_customDebugAction );
    
    //Restart application button
    setButtonGuiItem( KDialog::User3, KGuiItem( i18nc( "button action", "Restart Application" ) , KIcon("system-restart") , i18nc( "button help", "Use this button to restart the crashed application"), i18nc( "button help", "Use this button to restart the crashed application") ) );
    connect( this, SIGNAL(user3Clicked()), this, SLOT(restartApplication()) );
    
    //Close button
    setButtonToolTip( KDialog::Close, i18nc("help text", "Close this dialog (you will lose the crash information)") );
    setButtonWhatsThis( KDialog::Close, i18nc("help text", "Close this dialog (you will lose the crash information)") );
    setDefaultButton( KDialog::Close );
    setButtonFocus( KDialog::Close );
    
}

void DrKonqiDialog::enableDebugMenu( bool debuggerRunning )
{
    enableButton( KDialog::User2, !debuggerRunning );
    m_defaultDebugAction->setEnabled( !debuggerRunning );
    m_customDebugAction->setEnabled( !debuggerRunning );
}

void DrKonqiDialog::reportBugAssistant()
{
    DrKonqiBugReport * m_bugReportAssistant = new DrKonqiBugReport();
    close();
    m_bugReportAssistant->show();
}

void DrKonqiDialog::aboutBugReporting(QString)
{
    if ( !m_aboutBugReportingDialog )
        m_aboutBugReportingDialog = new AboutBugReportingDialog( this );
    m_aboutBugReportingDialog->show();
}

void DrKonqiDialog::restartApplication()
{
    enableButton( KDialog::User3, false );
    DrKonqi::instance()->restartCrashedApplication();
}

void DrKonqiDialog::slotNewDebuggingApp( const QString & app )
{
    m_customDebugAction->setVisible( true );
    m_customDebugAction->setEnabled( true );
    m_customDebugAction->setText( app );
}

DrKonqiDialog::~DrKonqiDialog()
{
    delete m_aboutBugReportingDialog;
    delete m_backtraceWidget;
    delete m_debugMenu;
}
