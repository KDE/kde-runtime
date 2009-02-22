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

#include "drkonqiassistantpages.h"

DrKonqiBugReport::DrKonqiBugReport( CrashInfo * crash, QWidget * parent ) : 
    KAssistantDialog(parent),
    m_crashInfo( crash )
{
    setWindowTitle( "Crash Reporting Assistant" );
    setWindowIcon( KIcon("tools-report-bug") );
    
    connect( this, SIGNAL(currentPageChanged(KPageWidgetItem *, KPageWidgetItem *)), this, SLOT(currentPageChanged_slot(KPageWidgetItem *, KPageWidgetItem *)));  
    
    //Introduction Page
    m_intro = new IntroductionPage();
    m_introPage = new KPageWidgetItem( m_intro, "Intro" );
    m_introPage->setHeader( "Introduction" );
    m_introPage->setIcon( KIcon("tools-report-bug") );
    
    //Crash Information Page
    m_backtrace = new CrashInformationPage( m_crashInfo );
    connect( m_backtrace, SIGNAL(setNextButton(bool)), this, SLOT(enableNextButton(bool)) );
    m_backtracePage = new KPageWidgetItem( m_backtrace , "Backtrace" );
    m_backtracePage->setHeader( "Crash Information (Backtrace)" );
    m_backtracePage->setIcon( KIcon("tools-report-bug") );

    //Bug Awareness Page
    m_awareness = new BugAwarenessPage( m_crashInfo );
    m_awarenessPage = new KPageWidgetItem( m_awareness, "Awareness" );
    m_awarenessPage->setHeader( "What do you know about the crash ?" );
    m_awarenessPage->setIcon( KIcon("tools-report-bug") );

    //Results Page
    m_conclusions = new ConclusionPage( m_crashInfo );
    connect( m_conclusions, SIGNAL(setNextButton(bool)), this, SLOT(enableNextButton(bool)) );
    m_conclusionsPage = new KPageWidgetItem( m_conclusions , "Results" );
    m_conclusionsPage->setHeader( "Crash Analysis Results" );
    m_conclusionsPage->setIcon( KIcon("tools-report-bug") );
    
    //Bugzilla Login
    m_bugzillaLogin =  new BugzillaLoginPage( m_crashInfo );
    connect( m_bugzillaLogin, SIGNAL(setNextButton(bool)), this, SLOT(enableNextButton(bool)) );
    m_bugzillaLoginPage = new KPageWidgetItem( m_bugzillaLogin, "BugzillaLogin");
    m_bugzillaLoginPage->setHeader( "KDE Bugtracker Login" );
    m_bugzillaLoginPage->setIcon( KIcon("tools-report-bug") );
    
    //Bugzilla keywords
    m_bugzillaKeywords =  new BugzillaKeywordsPage( m_crashInfo );
    connect( m_bugzillaKeywords, SIGNAL(setNextButton(bool)), this, SLOT(enableNextButton(bool)) );
    m_bugzillaKeywordsPage = new KPageWidgetItem( m_bugzillaKeywords, "BugzillaKeywords");
    m_bugzillaKeywordsPage->setHeader( "Bug Report Keywords" );
    m_bugzillaKeywordsPage->setIcon( KIcon("tools-report-bug") );
    
    //Bugzilla duplicates
    m_bugzillaDuplicates =  new BugzillaDuplicatesPage( m_crashInfo );
    m_bugzillaDuplicatesPage = new KPageWidgetItem( m_bugzillaDuplicates, "BugzillaDuplicates");
    m_bugzillaDuplicatesPage->setHeader( "Bug Report Duplicate Listing" );
    m_bugzillaDuplicatesPage->setIcon( KIcon("tools-report-bug") );
    
    addPage( m_introPage );
    addPage( m_backtracePage );
    addPage( m_awarenessPage );
    addPage( m_conclusionsPage );
    addPage( m_bugzillaLoginPage );
    addPage( m_bugzillaKeywordsPage );
    addPage( m_bugzillaDuplicatesPage );
    
    KPageWidgetItem * dummy = new KPageWidgetItem( new QWidget() );
    dummy->setHeader( "Dummy Last Page :)" );
    addPage( dummy );
    
    setMinimumSize( QSize(600,400) );
    resize( QSize(600,400) );
    
}

void DrKonqiBugReport::enableNextButton( bool enabled)
{
    enableButton( KDialog::User2, enabled );
}

DrKonqiBugReport::~DrKonqiBugReport()
{
    delete m_crashInfo;
}

void DrKonqiBugReport::currentPageChanged_slot(KPageWidgetItem * current , KPageWidgetItem * before)  
{
    Q_UNUSED( before );
    
    if( current->name() == "Backtrace" )
    {
        m_backtrace->loadBacktrace();
    }
    else if( current->name() == "BugzillaDuplicates" )
    {
        m_bugzillaDuplicates->searchDuplicates();
    }
    else if( current->name() == "BugzillaKeywords" )
    {
        enableNextButton( false );
        m_bugzillaKeywords->aboutToShow();
    }
    else if( current->name() == "BugzillaLogin" )
    {
        enableNextButton( false );
        m_bugzillaLogin->aboutToShow();
    }
    else if( current->name() == "Results" )
    {
        enableNextButton( false );
        m_conclusions->generateResult();
    }
}
