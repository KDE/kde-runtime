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
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QStackedWidget>
#include <QtGui/QMenu>

#include <kicon.h>
#include <kpushbutton.h>
#include <ktextbrowser.h>
#include <kstandarddirs.h>
#include <klocale.h>

DrKonqiDialog::DrKonqiDialog( QWidget * parent ) :
    KDialog(parent), 
    m_aboutBugReportingDialog(0),
    m_backtraceWidget(0)
{
    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();
    //connect( krashConfig, SIGNAL(newDebuggingApplication(QString)), SLOT(slotNewDebuggingApp(QString))); //FIXME
    
    //Setting dialog title and icon
    setCaption( krashConfig->programName() );
    setWindowIcon( KIcon("tools-report-bug") );

    //Main widget components
    QLabel * title = new QLabel( i18nc( "applicationName closed unexpectedly", "<title>%1 closed unexpectedly</title>", DrKonqi::instance()->krashConfig()->programName() ) );
    title->setWordWrap( true ); 
    
    QLabel * infoLabel = new QLabel( i18nc("Small explanation of the crash cause",
    "<para>This probably happened because there is a bug in the application.</para><para>You can help us improve KDE by reporting this bug.</para>" ) );
    infoLabel->setWordWrap( true ); 
    
    m_aboutBugReportingButton = new KPushButton( KGuiItem( i18nc("button action", "Learn more about bug reporting") , KIcon("help-hint"),  i18nc("help text", "Get help in order to know how to file an useful bug report"), i18nc("help text", "Get help in order to know how to file an useful bug report") ) ); //TODO rewrite text?
    m_aboutBugReportingButton->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
    connect( m_aboutBugReportingButton, SIGNAL(clicked()), this, SLOT(aboutBugReporting()) );

    m_reportBugButton = new KPushButton( KGuiItem( i18nc("Button action", "Report Bug"), KIcon("tools-report-bug"), i18nc("help text", "Starts the bug report assistant" ), i18nc("help text", "Starts the bug report assistant" ) ) );
    m_reportBugButton->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
    connect( m_reportBugButton, SIGNAL(clicked()), this,  SLOT(reportBugAssistant()) );
    
    QHBoxLayout * buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget( m_aboutBugReportingButton );
    buttonLayout->addWidget( m_reportBugButton );
    buttonLayout->addStretch();
    
    //Main widget layout
    QVBoxLayout * introLayout = new QVBoxLayout;
    introLayout->setSpacing( 10 );
    introLayout->addWidget( title );
    introLayout->addWidget( infoLabel );
    introLayout->addLayout( buttonLayout );
    introLayout->addStretch();
    
    //Introduction widget
    m_introWidget = new QWidget( this );
    m_introWidget->setLayout( introLayout );
    m_introWidget->setMinimumSize( QSize(550, 220) );
    
    QString styleSheet = QString( ".QWidget {"
                       "background-image: url(%1);"
                       "background-repeat: no-repeat;"
                       "background-position: right;"
                       "} ").arg( KStandardDirs::locate( "appdata", QLatin1String( "pics/konqi.png" ) ) );
    m_introWidget->setStyleSheet( styleSheet );
    
    //Advanced Widget  FIXME discuss
    m_advancedWidget = new QWidget( this );
    
    QVBoxLayout * advancedLayout = new QVBoxLayout();

    advancedLayout->addWidget( new QLabel( i18n( "Application: %1 , PID: %2 , Signal: %3 (%4)", krashConfig->appName(), QString::number(krashConfig->pid()), krashConfig->signalNumber(), krashConfig->signalName() ) ) );

    m_backtraceWidget = new GetBacktraceWidget(DrKonqi::instance()->backtraceGenerator());
    m_backtraceWidget->setMinimumSize( QSize(550, 220) );
    advancedLayout->addWidget( m_backtraceWidget );
    m_advancedWidget->setLayout( advancedLayout );
    
    //Stacked main widget
    m_stackedWidget = new QStackedWidget();
    m_stackedWidget->addWidget( m_introWidget );
    m_stackedWidget->addWidget( m_advancedWidget );
    setMainWidget( m_stackedWidget );
    
    //KDialog default buttons
    
    //Show backtrace/introduction
    m_guiItemShowBacktrace = KGuiItem(i18nc("button action", "Advanced ( Show Backtrace )"), KIcon("document-new"), i18nc("help text", "Generate and show the backtrace and the crash details" ), i18nc("help text", "Generate and show the backtrace and the crash details" ) );
    
    m_guiItemShowIntroduction = KGuiItem(i18nc("button action", "Show Introduction"), KIcon("help-contextual"), i18nc("help text", "Show the Crash Dialog introduction page" ), i18nc("help text", "Show the Crash Dialog introduction page" ) );
    
    //Set kdialog buttons
    showButtonSeparator( true );
    setButtons( KDialog::User1 | KDialog::User2 | KDialog::User3 | KDialog::Close );
    
    //Toggle to advanced mode button
    setButtonGuiItem( KDialog::User1, m_guiItemShowBacktrace );
    connect( this, SIGNAL(user1Clicked()), this, SLOT(toggleBacktrace()) );
    
    //Default debugger button and menu (only for developer mode)
    setButtonGuiItem( KDialog::User2, KGuiItem( i18nc("Button action", "Debug") , KIcon("document-edit"), i18nc("help text", "Starts a program to debug the crashed application" ), i18nc("help text", "Starts a program to debug the crashed application" ) ) );
    showButton( KDialog::User2, krashConfig->showDebugger() );
    
    m_debugMenu = new QMenu();
    setButtonMenu( KDialog::User2, m_debugMenu );
    
    m_defaultDebugAction = new QAction( KIcon("document-edit"), "Debug in default application", m_debugMenu );
    connect( m_defaultDebugAction, SIGNAL(triggered()), DrKonqi::instance(), SLOT(startDefaultExternalDebugger()) );
    
    m_customDebugAction = new QAction( KIcon("document-edit"), "Debug in custom application", m_debugMenu );
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
    
    resize( QSize(550, 220) );
}

void DrKonqiDialog::reportBugAssistant()
{
    DrKonqiBugReport * m_bugReportAssistant = new DrKonqiBugReport();
    hide();
    m_bugReportAssistant->show();
}

void DrKonqiDialog::aboutBugReporting()
{
    if ( !m_aboutBugReportingDialog )
        m_aboutBugReportingDialog = new AboutBugReportingDialog( this );
    m_aboutBugReportingDialog->show();
}

void DrKonqiDialog::toggleBacktrace()
{
    if( m_stackedWidget->currentWidget() == m_introWidget )
    {
        m_backtraceWidget->generateBacktrace();
        m_stackedWidget->setCurrentWidget( m_advancedWidget );
        
        setButtonGuiItem( KDialog::User1, m_guiItemShowIntroduction );
    }
    else
    {
        m_stackedWidget->setCurrentWidget( m_introWidget );
        
        setButtonGuiItem( KDialog::User1, m_guiItemShowBacktrace );
    }
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
    delete m_stackedWidget;
    delete m_debugMenu;
}
