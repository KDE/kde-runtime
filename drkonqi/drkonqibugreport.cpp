/*******************************************************************
* drkonqibugreport.cpp
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

#include "drkonqibugreport.h"

#include <QtGui/QCloseEvent>

#include <kdebug.h>
#include <kmessagebox.h>

#include "drkonqiassistantpages_base.h"
#include "drkonqiassistantpages_bugzilla.h"
#include "aboutbugreportingdialog.h"
#include "reportinfo.h"

DrKonqiBugReport::DrKonqiBugReport( QWidget * parent ) :
    KAssistantDialog(parent),
    m_aboutBugReportingDialog(0),
    m_reportInfo(new ReportInfo),
    m_canClose(false)
{
    setWindowTitle( i18n( "Crash Reporting Assistant" ) );
    setWindowIcon( KIcon( "tools-report-bug" ) );
    
    connect( this, SIGNAL(currentPageChanged(KPageWidgetItem *, KPageWidgetItem *)), this, SLOT(currentPageChanged_slot(KPageWidgetItem *, KPageWidgetItem *)));  
    connect( this, SIGNAL(helpClicked()), this, SLOT(showHelp()) );
    
    //Introduction Page
    IntroductionPage * m_intro = new IntroductionPage( this );
    connectSignals( m_intro );
    
    KPageWidgetItem * m_introPage = new KPageWidgetItem( m_intro, "Intro" );
    m_introPage->setHeader( i18nc( "the Introduction page", "Introduction" ) );
    m_introPage->setIcon( KIcon("tools-report-bug") );
    
    //Crash Information Page
    CrashInformationPage * m_backtrace = new CrashInformationPage( this );
    connectSignals( m_backtrace );
    
    KPageWidgetItem * m_backtracePage = new KPageWidgetItem( m_backtrace , "Backtrace" );
    m_backtracePage->setHeader( i18n( "Crash Information (Backtrace)" ) );
    m_backtracePage->setIcon( KIcon("tools-report-bug") );

    //Bug Awareness Page
    BugAwarenessPage * m_awareness = new BugAwarenessPage( this );
    connectSignals( m_awareness );
    
    KPageWidgetItem * m_awarenessPage = new KPageWidgetItem( m_awareness, "Awareness" );
    m_awarenessPage->setHeader( i18n("What do you know about the crash ?" ) );
    m_awarenessPage->setIcon( KIcon("tools-report-bug") );

    //Results Page
    ConclusionPage * m_conclusions = new ConclusionPage( this );
    connectSignals( m_conclusions );
    
    KPageWidgetItem * m_conclusionsPage = new KPageWidgetItem( m_conclusions , "Results" );
    m_conclusionsPage->setHeader( i18n( "Crash Analysis Results" ) );
    m_conclusionsPage->setIcon( KIcon("tools-report-bug") );
    connect( m_conclusions, SIGNAL(finished(bool)), this, SLOT(assistantFinished(bool)) );
    
    //Bugzilla Login
    BugzillaLoginPage * m_bugzillaLogin =  new BugzillaLoginPage( this );
    connectSignals( m_bugzillaLogin );
    
    KPageWidgetItem * m_bugzillaLoginPage = new KPageWidgetItem( m_bugzillaLogin, "BugzillaLogin");
    m_bugzillaLoginPage->setHeader( i18n( "KDE Bug Tracking System Login" ) );
    m_bugzillaLoginPage->setIcon( KIcon("tools-report-bug") );
    
    //Bugzilla keywords
    BugzillaKeywordsPage * m_bugzillaKeywords =  new BugzillaKeywordsPage( this );
    connectSignals( m_bugzillaKeywords ); 
    
    KPageWidgetItem * m_bugzillaKeywordsPage = new KPageWidgetItem( m_bugzillaKeywords, "BugzillaKeywords");
    m_bugzillaKeywordsPage->setHeader( i18n( "Bug Report Keywords" ) );
    m_bugzillaKeywordsPage->setIcon( KIcon("tools-report-bug") );
    
    //Bugzilla duplicates
    BugzillaDuplicatesPage * m_bugzillaDuplicates =  new BugzillaDuplicatesPage( this );
    connectSignals( m_bugzillaDuplicates );
    
    KPageWidgetItem * m_bugzillaDuplicatesPage = new KPageWidgetItem( m_bugzillaDuplicates, "BugzillaDuplicates");
    m_bugzillaDuplicatesPage->setHeader( i18n( "Bug Report Possible Duplicates List" ) );
    m_bugzillaDuplicatesPage->setIcon( KIcon("tools-report-bug") );
 
    //Bugzilla information
    BugzillaInformationPage * m_bugzillaInformation =  new BugzillaInformationPage( this );
    connectSignals( m_bugzillaInformation );
    
    KPageWidgetItem * m_bugzillaInformationPage = new KPageWidgetItem( m_bugzillaInformation, "BugzillaInformation");
    m_bugzillaInformationPage->setHeader( i18n( "Details of the Bug Report" ) );
    m_bugzillaInformationPage->setIcon( KIcon("tools-report-bug") );

    //Bugzilla commit
    BugzillaSendPage * m_bugzillaSend =  new BugzillaSendPage( this );

    KPageWidgetItem * m_bugzillaSendPage = new KPageWidgetItem( m_bugzillaSend, "BugzillaSend");
    m_bugzillaSendPage->setHeader( i18n( "Send Crash Report" ) );
    m_bugzillaSendPage->setIcon( KIcon("tools-report-bug") );
    connect( m_bugzillaSend, SIGNAL(finished(bool)), this, SLOT(assistantFinished(bool)) );
    
    //TODO remember to keep ordered
    addPage( m_introPage );
    addPage( m_backtracePage );
    addPage( m_awarenessPage );
    addPage( m_conclusionsPage );
    addPage( m_bugzillaLoginPage );
    addPage( m_bugzillaKeywordsPage );
    addPage( m_bugzillaDuplicatesPage );
    addPage( m_bugzillaInformationPage );
    addPage( m_bugzillaSendPage );
    
    setMinimumSize( QSize(600,400) );
    resize( QSize(600,400) );
}

DrKonqiBugReport::~DrKonqiBugReport()
{
    delete m_aboutBugReportingDialog;
    delete m_reportInfo;
}

void DrKonqiBugReport::connectSignals( DrKonqiAssistantPage * page )
{
    connect( page, SIGNAL(completeChanged(DrKonqiAssistantPage*, bool)), this, SLOT(completeChanged(DrKonqiAssistantPage*, bool)) );
}

void DrKonqiBugReport::currentPageChanged_slot(KPageWidgetItem * current , KPageWidgetItem * before)  
{
    enableButton( KDialog::Cancel, true );
    m_canClose = false;
    
    if( before )
    {
        DrKonqiAssistantPage* beforePage = dynamic_cast<DrKonqiAssistantPage*>(before->widget());
        beforePage->aboutToHide();
    }
    
    if ( current )
    {
        DrKonqiAssistantPage* currentPage = dynamic_cast<DrKonqiAssistantPage*>(current->widget());
        enableNextButton( currentPage->isComplete() );
        currentPage->aboutToShow();
    }
        
    if( current->name() == "BugzillaSend" ) //Disable all buttons on last page
    {
        enableNextButton( false );
        enableBackButton( false );
        enableButton( KDialog::User1, false );
    }
}

void DrKonqiBugReport::completeChanged( DrKonqiAssistantPage* page, bool isComplete )
{
    if ( page == dynamic_cast<DrKonqiAssistantPage*>(currentPage()->widget()) )
    {
        enableNextButton( isComplete );
    }
}

void DrKonqiBugReport::assistantFinished( bool showBack )
{
    enableNextButton( false );
    enableBackButton( showBack );
    enableButton( KDialog::User1, true );
    enableButton( KDialog::Cancel, false );
    m_canClose = true;
}

void DrKonqiBugReport::showHelp()
{
    if ( !m_aboutBugReportingDialog )
        m_aboutBugReportingDialog = new AboutBugReportingDialog( this );
    m_aboutBugReportingDialog->show();
    m_aboutBugReportingDialog->showSection( QString("Begin") );
    m_aboutBugReportingDialog->showSection( currentPage()->name() );
}

//Override KAssistantDialog "next" page implementation
void DrKonqiBugReport::next() 
{
    DrKonqiAssistantPage * page = dynamic_cast<DrKonqiAssistantPage*>(currentPage()->widget());
    if( page )
    {
        if ( page->showNextPage() ) //Time to ask the user if we need to
        {
            KAssistantDialog::next();
        }
    } else {
        KAssistantDialog::next();
    }
}

void DrKonqiBugReport::enableNextButton( bool enabled )
{
    enableButton( KDialog::User2, enabled );
}

void DrKonqiBugReport::enableBackButton( bool enabled )
{
    enableButton( KDialog::User3, enabled );
}

void DrKonqiBugReport::reject()
{
    close();
}

void DrKonqiBugReport::closeEvent( QCloseEvent * event )
{
    if( !m_canClose )
    {
        if (KMessageBox::questionYesNo( this, i18n("Do you really want to close the bug assistant without sending the bug report?"),
                i18n("Close Crash Reporting Assistant") ) == KMessageBox::Yes )
        {
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}
