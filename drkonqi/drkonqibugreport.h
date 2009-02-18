#ifndef DRKONQIBUGREPORT__H
#define DRKONQIBUGREPORT__H

#include <QtGui>

//#include "krashconf.h"
#include "crashinfo.h"

#include <kassistantdialog.h>

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
