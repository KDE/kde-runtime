#ifndef DRKONQIASSISTANTPAGES__H
#define DRKONQIASSISTANTPAGES__H

#include <QtGui> //TODO
#include <KPushButton>

#include "getbacktracewidget.h"
#include "crashinfo.h"

//Introduction assistant page --------------
class IntroductionPage: public QWidget
{
  Q_OBJECT
  
  public:
  
    IntroductionPage( );

};

//Backtrace Page ---------------------------
class CrashInformationPage: public QWidget
{

    Q_OBJECT
    
    public:
    
        CrashInformationPage( CrashInfo * );
        void loadBacktrace() { backtraceWidget->generateBacktrace(); }
        
    Q_SIGNALS:
        void setNextButton(bool);
        
    private:
    
        GetBacktraceWidget * backtraceWidget;
};

//Bug Awareness Page ---------------
class BugAwarenessPage: public QWidget
{
  Q_OBJECT
  
  public:
  
    BugAwarenessPage(CrashInfo*);
    
  private Q_SLOTS:
  
    void detailStateChanged( int );
    void reproduceStateChanged( int );
    void compromiseStateChanged( int );
    
  private:
  
    CrashInfo * crashInfo;
};

//Results page
class ConclusionPage : public QWidget
{
    Q_OBJECT
    
    public:
    
      ConclusionPage( CrashInfo* );
      void generateResult();
    
    private Q_SLOTS:
    
        void reportButtonClicked();
        
    private:
    
        QLabel * conclusionLabel;
        QTextEdit * reportEdit;

        KPushButton * reportButton;
        KPushButton * saveReportButton;
        QLabel * noticeLabel;

        CrashInfo * crashInfo;
      
    Q_SIGNALS:
    
        void setFinishButton( bool );
  
};

#endif