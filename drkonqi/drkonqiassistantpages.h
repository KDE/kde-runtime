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

class QFormLayout;
class KLineEdit;
class KPushButton;
class QLabel;
class KTextEdit;
class QProgressDialog;

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
        QLabel * explanationLabel;
        KTextEdit * reportEdit;

        KPushButton * reportButton;
        KPushButton * saveReportButton;

        CrashInfo * crashInfo;
      
    Q_SIGNALS:
    
        void setFinishButton( bool );
        void setNextButton( bool );
  
};

//Bugzilla Login
class BugzillaLoginPage: public QWidget
{
    Q_OBJECT
    
    public: 

        BugzillaLoginPage(CrashInfo*);
        void aboutToShow();
        
    private Q_SLOTS:
        
        void loginClicked();
        void loginFinished( bool );
        
    Q_SIGNALS:
    
        void setNextButton( bool );
        void loginOk();
        
    private:
    
        QFormLayout * m_form;
        
        QLabel * m_subTitle;
        QLabel * m_loginLabel;
        KPushButton * m_loginButton;
        KLineEdit * m_userEdit;
        KLineEdit * m_passwordEdit;
        
        QProgressDialog * m_progressDialog;
        CrashInfo * crashInfo;
};


class BugzillaKeywordsPage : public QWidget
{
    Q_OBJECT
    
    public:
    
        BugzillaKeywordsPage( CrashInfo* );
        void aboutToShow();
        
    private Q_SLOTS:
        void textEdited( QString );
        
    private:
    
        KLineEdit * m_keywordsEdit;
        CrashInfo * crashInfo;
        
    Q_SIGNALS:
        
        void setNextButton( bool );
};
        

class BugzillaDuplicatesPage : public QWidget
{
    Q_OBJECT
    
    public:
        BugzillaDuplicatesPage(CrashInfo*);
        void searchDuplicates();
        
    private Q_SLOTS:
        
        void searchFinished(BugList*);
        void performSearch( QString, QString );
        
    private:
    
        
        bool searching;
        
        QLabel * m_searchingLabel;
        
        KTextEdit * m_duplicateList;
        CrashInfo * crashInfo;
};

#endif
