/*******************************************************************
* drkonqiassistantpages.h
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

#ifndef DRKONQIASSISTANTPAGES__H
#define DRKONQIASSISTANTPAGES__H

#include <QtGui/QWidget>

#include "getbacktracewidget.h"
#include "crashinfo.h"

class KPushButton;
class QLabel;
class KTextEdit;

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

//Conclusions/Result page
class ConclusionPage : public QWidget
{
    Q_OBJECT
    
    public:
    
      ConclusionPage( CrashInfo* );
      void generateResult();
    
    private Q_SLOTS:
    
        void reportButtonClicked();
        void saveReport();
        
    private:
    
        QLabel * conclusionLabel;
        KTextEdit * reportEdit;

        KPushButton * reportButton;
        KPushButton * saveReportButton;
        QLabel * noticeLabel;

        CrashInfo * crashInfo;
      
    Q_SIGNALS:
    
        void setFinishButton( bool );
  
};

#endif
