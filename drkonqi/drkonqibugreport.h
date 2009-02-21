/*******************************************************************
* drkonqibugreport.h
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

#ifndef DRKONQIBUGREPORT__H
#define DRKONQIBUGREPORT__H

#include <QtGui/QWidget>

#include <kassistantdialog.h>

#include "crashinfo.h"

class IntroductionPage;
class CrashInformationPage;
class BugAwarenessPage;
class ConclusionPage;
class BugzillaLoginPage;
class BugzillaKeywordsPage;
class BugzillaDuplicatesPage;

class DrKonqiBugReport: public KAssistantDialog
{
  Q_OBJECT

  //enum{ Page_Intro, Page_FetchInformation, Page_BugAwareness, Page_Result };
  
  public:
    explicit DrKonqiBugReport( CrashInfo *, QWidget * parent = 0);
    ~DrKonqiBugReport();
    
  private:
    
    IntroductionPage *          m_intro;
    CrashInformationPage *      m_backtrace;
    BugAwarenessPage *          m_awareness;
    ConclusionPage *            m_conclusions;
    BugzillaLoginPage *         m_bugzillaLogin;
    BugzillaKeywordsPage *      m_bugzillaKeywords;
    BugzillaDuplicatesPage *    m_bugzillaDuplicates;
    
    KPageWidgetItem *           m_introPage;
    KPageWidgetItem *           m_backtracePage;
    KPageWidgetItem *           m_awarenessPage;
    KPageWidgetItem *           m_conclusionsPage;
    KPageWidgetItem *           m_bugzillaLoginPage;
    KPageWidgetItem *           m_bugzillaKeywordsPage;
    KPageWidgetItem *           m_bugzillaDuplicatesPage;
    
    CrashInfo *                 m_crashInfo;
    
  private Q_SLOTS:
    
    void currentPageChanged_slot(KPageWidgetItem *, KPageWidgetItem *);  
    
    void enableNextButton( bool );

};

#endif
