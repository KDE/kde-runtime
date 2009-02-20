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
    crashInfo( crash )
{
    setWindowTitle( "Crash Reporting Assistant" );
    setWindowIcon( KIcon("tools-report-bug") );
    
    connect( this, SIGNAL(currentPageChanged(KPageWidgetItem *, KPageWidgetItem *)), this, SLOT(currentPageChanged_slot(KPageWidgetItem *, KPageWidgetItem *)));  
    
    //Introduction Page
    intro = new IntroductionPage();
    introPage = new KPageWidgetItem( intro, "Intro" );
    introPage->setHeader( "Introduction" );
    introPage->setIcon( KIcon("tools-report-bug") );
    
    //Crash Information Page
    backtrace = new CrashInformationPage( crashInfo );
    connect( backtrace, SIGNAL(setNextButton(bool)), this, SLOT(enableNextButton(bool)) );
    backtracePage = new KPageWidgetItem( backtrace , "Backtrace" );
    backtracePage->setHeader( "Crash Information (Backtrace)" );
    backtracePage->setIcon( KIcon("tools-report-bug") );

    //Bug Awareness Page
    awareness = new BugAwarenessPage( crashInfo );
    awarenessPage = new KPageWidgetItem( awareness, "Awareness" );
    awarenessPage->setHeader( "What do you know about the crash ?" );
    awarenessPage->setIcon( KIcon("tools-report-bug") );

    //Results Page
    conclusions = new ConclusionPage( crashInfo );
    connect( conclusions, SIGNAL(setNextButton(bool)), this, SLOT(enableNextButton(bool)) );
    conclusionsPage = new KPageWidgetItem( conclusions , "Results" );
    conclusionsPage->setHeader( "Results" );
    conclusionsPage->setIcon( KIcon("tools-report-bug") );
    
    //Bugzilla Login
    bugzillaLogin =  new BugzillaLoginPage( crashInfo );
    connect( bugzillaLogin, SIGNAL(setNextButton(bool)), this, SLOT(enableNextButton(bool)) );
    bugzillaLoginPage = new KPageWidgetItem( bugzillaLogin, "BugzillaLogin");
    bugzillaLoginPage->setHeader( "KDE Bugtracker Login" );
    bugzillaLoginPage->setIcon( KIcon("tools-report-bug") );
    
    //Bugzilla keywords
    bugzillaKeywords =  new BugzillaKeywordsPage( crashInfo );
    connect( bugzillaKeywords, SIGNAL(setNextButton(bool)), this, SLOT(enableNextButton(bool)) );
    bugzillaKeywordsPage = new KPageWidgetItem( bugzillaKeywords, "BugzillaKeywords");
    bugzillaKeywordsPage->setHeader( "Bug Report Keywords" );
    bugzillaKeywordsPage->setIcon( KIcon("tools-report-bug") );
    
    //Bugzilla duplicates
    bugzillaDuplicates =  new BugzillaDuplicatesPage( crashInfo );
    bugzillaDuplicatesPage = new KPageWidgetItem( bugzillaDuplicates, "BugzillaDuplicates");
    bugzillaDuplicatesPage->setHeader( "Bug Report Duplicate Listing" );
    bugzillaDuplicatesPage->setIcon( KIcon("tools-report-bug") );
    
    addPage( introPage );
    addPage( backtracePage );
    addPage( awarenessPage );
    addPage( conclusionsPage );
    addPage( bugzillaLoginPage );
    addPage( bugzillaKeywordsPage );
    addPage( bugzillaDuplicatesPage );    
    
    KPageWidgetItem * dummy = new KPageWidgetItem( new QWidget() );
    dummy->setHeader( "Dummy Last Page" );
    addPage( dummy );
    
    setMinimumSize( QSize(600,400) );
    resize( QSize(600,400) );
    
}

void DrKonqiBugReport::enableNextButton( bool enabled)
{
    enableButton( KDialog::User2, enabled );
}

/*
void DrKonqui::enableFinishButton( bool enabled )
{
    enableButton( KDialog::User1, enabled );
}

*/

DrKonqiBugReport::~DrKonqiBugReport()
{
    delete crashInfo;
}

void DrKonqiBugReport::currentPageChanged_slot(KPageWidgetItem * current , KPageWidgetItem * before)  
{
    Q_UNUSED( before );
    
    if( current->name() == "Backtrace" )
    {
        backtrace->loadBacktrace();
    }
    else if( current->name() == "BugzillaDuplicates" )
    {
        bugzillaDuplicates->searchDuplicates();
    }
    else if( current->name() == "BugzillaKeywords" )
    {
        enableNextButton( false );
        bugzillaKeywords->aboutToShow();
    }
    else if( current->name() == "BugzillaLogin" )
    {
        enableNextButton( false );
        bugzillaLogin->aboutToShow();
    }
    else if( current->name() == "Results" )
    {
        enableNextButton( false );
        conclusions->generateResult();
    }
}
