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

#include <kwallet.h>

class QFormLayout;
class KLineEdit;
class KPushButton;
class QLabel;
class KTextEdit;
class QProgressDialog;
class QDate;
class QTreeWidget;
class QTreeWidgetItem;
class QCheckBox;

//BASE class which implements some signals, and aboutTo functions
class DrKonqiAssistantPage: public QWidget
{
    Q_OBJECT
    
    public:
        DrKonqiAssistantPage() {}
    
        virtual void aboutToShow() {}
        virtual void aboutToHide() {}
        
    Q_SIGNALS:
        void setNextButton( bool );
        void setBackButton( bool );
        void setFinishButton( bool );

};

//Introduction assistant page --------------
class IntroductionPage: public DrKonqiAssistantPage
{
    Q_OBJECT
    
    public:
        IntroductionPage( );
        
        void aboutToShow() {}
        void aboutToHide() {}
};

//Backtrace Page ---------------------------
class CrashInformationPage: public DrKonqiAssistantPage
{

    Q_OBJECT
    
    public:
        CrashInformationPage( CrashInfo * );
        
        void aboutToShow() { m_backtraceWidget->generateBacktrace(); }
        void aboutToHide() {}
        
    private:
        GetBacktraceWidget * m_backtraceWidget;
};

//Bug Awareness Page ---------------
class BugAwarenessPage: public DrKonqiAssistantPage
{
    Q_OBJECT
    
    public:
        BugAwarenessPage(CrashInfo*);
        
        void aboutToShow() {}
        void aboutToHide();
       
    private:
        QCheckBox * m_canDetailCheckBox;
        QCheckBox * m_compromiseCheckBox;
        QCheckBox * m_canReproduceCheckBox;
        
        CrashInfo * m_crashInfo;
};

//Conclusions/Result page
class ConclusionPage : public DrKonqiAssistantPage
{
    Q_OBJECT
    
    public:
        ConclusionPage( CrashInfo* );
        
        void aboutToShow();
        void aboutToHide() {}
        
    private Q_SLOTS:
        void reportButtonClicked();
        void saveReport();
        
    private:
        QLabel *        m_conclusionLabel;
        QLabel *        m_explanationLabel;
        KTextBrowser *  m_reportEdit;

        KPushButton *   m_reportButton;
        KPushButton *   m_saveReportButton;

        CrashInfo *     m_crashInfo;
};

//Bugzilla Login
class BugzillaLoginPage: public DrKonqiAssistantPage
{
    Q_OBJECT
    
    public: 
        BugzillaLoginPage(CrashInfo*);
        ~BugzillaLoginPage();
        
        void aboutToShow();
        void aboutToHide() {}
        
    private Q_SLOTS:
        void loginClicked();
        void loginFinished( bool );
        
    private:
        QFormLayout *   m_form;
        
        QLabel *        m_subTitle;
        QLabel *        m_loginLabel;
        KPushButton *   m_loginButton;
        KLineEdit *     m_userEdit;
        KLineEdit *     m_passwordEdit;
        
        //QProgressDialog * m_progressDialog;
        KWallet::Wallet *   m_wallet;
        CrashInfo * m_crashInfo;
};


class BugzillaKeywordsPage : public DrKonqiAssistantPage
{
    Q_OBJECT
    
    public:
        BugzillaKeywordsPage( CrashInfo* );
        
        void aboutToShow();
        void aboutToHide();
        
    private Q_SLOTS:
        void textEdited( QString );
        
    private:
        KLineEdit * m_keywordsEdit;
        CrashInfo * m_crashInfo;
};
        

class BugzillaDuplicatesPage : public DrKonqiAssistantPage
{
    Q_OBJECT
    
    public:
        BugzillaDuplicatesPage(CrashInfo*);
        ~BugzillaDuplicatesPage();
        
        void aboutToShow();
        void aboutToHide() {}
        
    private Q_SLOTS:
        void searchFinished( const BugMapList& );
        
        void itemClicked( QTreeWidgetItem *, int );
        
        void bugFetchFinished( BugReport * );
        
        void searchMore();
        void performSearch();
        
        
    private:
        bool            m_searching;
        
        QLabel *        m_searchingLabel;
        
        QCheckBox *     m_foundDuplicateCheckBox;
        
        QDate           m_startDate;
        QDate           m_endDate;
        
        KPushButton *   m_searchMoreButton;
        QTreeWidget *   m_bugListWidget;

        KDialog *       m_infoDialog;
        KTextBrowser *  m_infoDialogBrowser;
        QLabel *        m_infoDialogLink;
        
        CrashInfo *     m_crashInfo;
};

class BugzillaInformationPage : public DrKonqiAssistantPage
{
    Q_OBJECT
    
    public:
        BugzillaInformationPage( CrashInfo * );
        
        void aboutToShow();
        void aboutToHide();
        
    private Q_SLOTS:
        void checkTexts();
        
    private:
        QLabel *        m_titleLabel;
        KLineEdit *     m_titleEdit;
        QLabel *        m_detailsLabel;
        KTextEdit *     m_detailsEdit;
        QLabel *        m_reproduceLabel;
        KTextEdit *     m_reproduceEdit;
        
        CrashInfo * m_crashInfo;
};

class BugzillaCommitPage : public DrKonqiAssistantPage
{
    Q_OBJECT
    
    public:
        BugzillaCommitPage( CrashInfo * );
        
        void aboutToShow();
        void aboutToHide() {}
        
    private Q_SLOTS:
        void commited();
        
    private:
        
        KTextBrowser *  m_statusBrowser;
        CrashInfo *  m_crashInfo;

};
#endif
