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

#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QStackedWidget>

#include <QMenu>

#include <kicon.h>
#include <kpushbutton.h>
#include <ktextbrowser.h>
#include <kstandarddirs.h>
#include <klocale.h>

#include "crashinfo.h"

#include "getbacktracewidget.h"
#include "drkonqibugreport.h"
#include "aboutbugreportingdialog.h"

DrKonqiDialog::DrKonqiDialog( CrashInfo * info, QWidget * parent ) : 
    KDialog(parent), 
    m_aboutBugReportingDialog(0),
    m_backtraceWidget(0),
    m_crashInfo(info)
{
    connect( m_crashInfo->getCrashConfig(), SIGNAL(newDebuggingApplication(QString)), SLOT(slotNewDebuggingApp(QString)));
    
    setCaption( m_crashInfo->getProductName() );
    setWindowIcon( KIcon("tools-report-bug") );

    //GUI
    QLabel * title = new QLabel( i18n("<title>The application closed unexpectedly</title>") );
    title->setWordWrap( true ); 
    
    QLabel * crashTitle = new QLabel( m_crashInfo->getCrashTitle() );
    crashTitle->setWordWrap( true ); 
    
    QLabel * infoLabel = new QLabel( i18nc("Small explanation of the crash cause",
    "This probably happened because there is a bug in the application.<nl />Signal: %1 (%2)"
    , m_crashInfo->getCrashConfig()->signalName(), m_crashInfo->getCrashConfig()->signalText() ) );
    infoLabel->setWordWrap( true ); 

    QLabel * whatToDoLabel = new QLabel( i18n( "You can help us to improve the software reporting this bug." ) );
    
    m_aboutBugReportingButton = new KPushButton( KGuiItem( i18nc("button action", "Learn more about bug reporting") , KIcon("help-hint"),  i18nc("help text", "Get help in order to know how to file an useful bug report"), i18nc("help text", "Get help in order to know how to file an useful bug report") ) ); //TODO rewrite text?
    
    m_aboutBugReportingButton->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
    connect( m_aboutBugReportingButton, SIGNAL(clicked()), this, SLOT(aboutBugReporting()) );

    //Introduction widget layout
    QVBoxLayout * introLayout = new QVBoxLayout;
    introLayout->setSpacing( 10 );
    introLayout->addWidget( title );
    introLayout->addWidget( crashTitle );
    introLayout->addWidget( infoLabel );
    introLayout->addWidget( whatToDoLabel );
    introLayout->addWidget( m_aboutBugReportingButton );
    introLayout->addStretch();
    
    //Introduction widget
    m_introWidget = new QWidget();
    m_introWidget->setLayout( introLayout );
    //m_introWidget->setMinimumSize( QSize(500, 220) );
    
    QString styleSheet = QString( ".QWidget {"
                       "background-image: url(%1);"
                       "background-repeat: no-repeat;"
                       "background-position: right;"
                       "} ").arg( KStandardDirs::locate( "appdata", QLatin1String( "pics/konqi.png" ) ) );
    m_introWidget->setStyleSheet( styleSheet );
    
    //Backtrace Widget
    m_backtraceWidget = new GetBacktraceWidget( m_crashInfo );
    m_backtraceWidget->setMinimumSize( QSize(500, 10) );

    //Stacked main widget
    m_stackedWidget = new QStackedWidget();
    m_stackedWidget->addWidget( m_introWidget );
    m_stackedWidget->addWidget( m_backtraceWidget );
    setMainWidget( m_stackedWidget );
    
    //Show backtrace/introduction
    m_guiItemShowBacktrace = KGuiItem(i18nc("button action", "Show Backtrace (Advanced)"), KIcon("document-new"), i18nc("help text", "Generate and show the backtrace (crash information)" ), i18nc("help text", "Generate and show the backtrace (crash information)" ) );
    
    m_guiItemShowIntroduction = KGuiItem(i18nc("button action", "Show Introduction"), KIcon("help-contextual"), i18nc("help text", "Show the Crash Dialog introduction page" ), i18nc("help text", "Show the Crash Dialog introduction page" ) );
    
    //Set kdialog buttons
    showButtonSeparator( true );
    setButtons( KDialog::Help | KDialog::User1 | KDialog::User2 | KDialog::User3 | KDialog::Close );
    
    //Show backtrace (toggle) button
    setButtonGuiItem( KDialog::Help, m_guiItemShowBacktrace );
    connect( this, SIGNAL(helpClicked()), this, SLOT(toggleBacktrace()) );
    
    //Default debugger button and menu (only for developer mode)
    setButtonGuiItem( KDialog::User2, KGuiItem( i18nc("Button action", "Debug") , KIcon("document-edit"), i18nc("help text", "Starts a program to debug the crashed application" ), i18nc("help text", "Starts a program to debug the crashed application" ) ) );
    showButton( KDialog::User2, m_crashInfo->getCrashConfig()->showDebugger() );
    
    m_debugMenu = new QMenu();
    setButtonMenu( KDialog::User2, m_debugMenu );
    
    m_defaultDebugAction = new QAction( KIcon("document-edit"), "Debug in default application", m_debugMenu );
    connect( m_defaultDebugAction, SIGNAL(triggered()), this, SLOT(startDefaultDebugger()) );
    
    m_customDebugAction = new QAction( KIcon("document-edit"), "Debug in custom application", m_debugMenu );
    connect( m_customDebugAction, SIGNAL(triggered()), this, SLOT(startCustomDebugger()) );
    m_customDebugAction->setEnabled( false );
    m_customDebugAction->setVisible( false );
    
    m_debugMenu->addAction( m_defaultDebugAction );
    m_debugMenu->addAction( m_customDebugAction );
    
    //Restart application button
    setButtonGuiItem( KDialog::User3, KGuiItem( i18nc( "button action", "Restart Application" ) , KIcon("system-restart") , i18nc( "button help", "Use this button to restart the crashed application"), i18nc( "button help", "Use this button to restart the crashed application") ) );
    connect( this, SIGNAL(user3Clicked()), this, SLOT(restartApplication()) );
    
    //Report bug button
    setButtonGuiItem( KDialog::User1, KGuiItem( i18nc("Button action", "Report Bug"), KIcon("tools-report-bug"), i18nc("help text", "Starts the bug report assistant" ), i18nc("help text", "Starts the bug report assistant" ) ) );
    connect( this, SIGNAL(user1Clicked()), this, SLOT(reportBugAssistant()) );
    
    //Close button
    setButtonToolTip( KDialog::Close, i18nc("help text", "Close this dialog (you will lose the crash information)") );
    setButtonWhatsThis( KDialog::Close, i18nc("help text", "Close this dialog (you will lose the crash information)") );
    setDefaultButton( KDialog::Close );
    setButtonFocus( KDialog::Close );    
}

void DrKonqiDialog::reportBugAssistant()
{
    DrKonqiBugReport * m_bugReportAssistant = new DrKonqiBugReport( m_crashInfo );
    hide();
    m_bugReportAssistant->show();
}

void DrKonqiDialog::aboutBugReporting()
{
    if ( !m_aboutBugReportingDialog )
    {
        m_aboutBugReportingDialog = new AboutBugReportingDialog( this );
    }
    m_aboutBugReportingDialog->show();
}

void DrKonqiDialog::toggleBacktrace()
{
    if( m_stackedWidget->currentWidget() == m_introWidget )
    {
        m_backtraceWidget->generateBacktrace();
        m_stackedWidget->setCurrentWidget( m_backtraceWidget );
        
        setButtonGuiItem( KDialog::Help, m_guiItemShowIntroduction );
    }
    else
    {
        m_stackedWidget->setCurrentWidget( m_introWidget );
        
        setButtonGuiItem( KDialog::Help, m_guiItemShowBacktrace );
    }
}

void DrKonqiDialog::startDefaultDebugger()
{
    QString str = m_crashInfo->getCrashConfig()->debuggerCommand();
    m_crashInfo->getCrashConfig()->expandString(str, KrashConfig::ExpansionUsageShell);

    KProcess proc;
    proc.setShellCommand(str);
    proc.startDetached();
}

void DrKonqiDialog::startCustomDebugger()
{
    m_crashInfo->startCustomDebugger();
}

void DrKonqiDialog::restartApplication()
{
    enableButton( KDialog::User3, false );
    
    KProcess proc;
    proc.setShellCommand( m_crashInfo->getApplicationCommand() );
    proc.startDetached();
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
    delete m_stackedWidget;
}
