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

#include <QtDebug>

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

//BASE interface which implements some signals, and aboutTo + setBusy/setIdle functions
class DrKonqiAssistantPage: public QWidget
{
    Q_OBJECT
    
    public:
        DrKonqiAssistantPage()
        {
            isBusy = false;
        }
    
        //aboutToShow may load the widget data if empty
        virtual void aboutToShow() {}
        //aboutToHide may save the widget data to crashInfo, to continue
        virtual void aboutToHide() {}
        
    public Q_SLOTS:
        //Set the page as busy and doesn't allow to change.
        void setBusy()
        {
            if( !isBusy )
            {
                isBusy = true;
                QApplication::setOverrideCursor(Qt::BusyCursor);
                setNextButton( false );
                emit enableBackButton( false );
            }
        }
        
        //Set the page as idle and allow to go back, but only to go forward if showNext==true
        void setIdle( bool showNext )
        {
            isBusy = false;
            QApplication::restoreOverrideCursor();
            setNextButton( showNext );
            emit enableBackButton( true );
        }
        
        void setNextButton( bool showNext )
        {
            emit enableNextButton( showNext );
        }
        
        void setBackButton( bool showBack )
        {
            emit enableBackButton( showBack );
        }
        
    Q_SIGNALS:
        //Enable/disable buttons on the assistant based on the widget state
        void enableNextButton( bool );
        void enableBackButton( bool );
        void enableFinishButton( bool );
        
    private:
    
        bool isBusy;

};

//Introduction assistant page --------------
class IntroductionPage: public DrKonqiAssistantPage
{
    Q_OBJECT
    
    public:
        IntroductionPage( );
        
        void aboutToShow();
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
        void aboutToHide();
        
    private Q_SLOTS:
        void searchFinished( const BugMapList& );
        void searchError( QString );
        
        void itemClicked( QTreeWidgetItem *, int );
        
        void bugFetchFinished( BugReport * );
        void bugFetchError( QString );
        
        void searchMore();
        void performSearch();
        
        void checkBoxChanged(int);
        void mayBeDuplicateClicked();
        
        void enableControls( bool );
        
    private:
        bool            m_searching;
        
        QLabel *        m_searchingLabel;
        
        QDate           m_startDate;
        QDate           m_endDate;
        
        KPushButton *   m_searchMoreButton;
        QTreeWidget *   m_bugListWidget;

        KDialog *       m_infoDialog;
        KTextBrowser *  m_infoDialogBrowser;
        QLabel *        m_infoDialogLink;
        
        QCheckBox *     m_foundDuplicateCheckBox;
        KLineEdit *     m_possibleDuplicateEdit;
        KPushButton *   m_mineMayBeDuplicateButton;
        
        int             m_currentBugNumber;
        
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
        void commited(int);
        
    private:
        
        KTextBrowser *  m_statusBrowser;
        CrashInfo *  m_crashInfo;

};
#endif
