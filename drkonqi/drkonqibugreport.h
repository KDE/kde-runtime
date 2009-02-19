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

class DrKonqiBugReport: public KAssistantDialog
{
  Q_OBJECT

  //enum{ Page_Intro, Page_FetchInformation, Page_BugAwareness, Page_Result };
  
  public:
    DrKonqiBugReport( CrashInfo *, QWidget * parent = 0);
    ~DrKonqiBugReport();
    
  private:
    
    IntroductionPage * intro;
    CrashInformationPage * backtrace;
    BugAwarenessPage * awareness;
    ConclusionPage * conclusions;
    
    KPageWidgetItem * introPage;
    KPageWidgetItem * backtracePage;
    KPageWidgetItem * awarenessPage;
    KPageWidgetItem * conclusionsPage;
    
    CrashInfo * crashInfo;
    
  private Q_SLOTS:
  
    void currentPageChanged_slot(KPageWidgetItem *, KPageWidgetItem *);  
    
    void enableNextButton( bool );
    
    /*
    void enableFinishButton( bool );
        */

};

#endif
