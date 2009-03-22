/*******************************************************************
* drkonqiassistantpages_bugzilla.h
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

#ifndef DRKONQIASSISTANTPAGESBUGZILLA__H
#define DRKONQIASSISTANTPAGESBUGZILLA__H

#include "crashinfo.h"
#include <kwallet.h>

#include "drkonqiassistantpages_base.h"

class QFormLayout;
class KLineEdit;
class KPushButton;
class QLabel;
class KTextBrowser;
class KTextEdit;
class QDate;
class QTreeWidget;
class QTreeWidgetItem;
class QCheckBox;

class StatusWidget;

//Bugzilla Login
class BugzillaLoginPage: public DrKonqiAssistantPage
{
    Q_OBJECT
    
    public: 
        BugzillaLoginPage(CrashInfo*);
        ~BugzillaLoginPage();
        
        void aboutToShow();
        bool isComplete();
        
    private Q_SLOTS:
        void loginClicked();
        void loginFinished( bool );
        void loginError( QString );
        
        void walletLogin();
        
        //void progress( KJob*, unsigned long );
        
    private:
        QFormLayout *   m_form;
        
        KPushButton *   m_loginButton;
        KLineEdit *     m_userEdit;
        KLineEdit *     m_passwordEdit;
                
        StatusWidget *  m_statusWidget;
        
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
        
        bool isComplete();
        
    private Q_SLOTS:
        void textEdited( QString );
        
    private:
        KLineEdit * m_keywordsEdit;
        CrashInfo * m_crashInfo;
        bool    m_keywordsOK;
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
        
        bool canSearchMore();
        
    private:
        bool            m_searching;
        
        StatusWidget *  m_statusWidget;
        
        QDate           m_startDate;
        QDate           m_endDate;
        
        KPushButton *   m_searchMoreButton;
        QTreeWidget *   m_bugListWidget;

        KDialog *       m_infoDialog;
        KTextBrowser *  m_infoDialogBrowser;
        QLabel *        m_infoDialogLink;
        StatusWidget *  m_infoDialogStatusWidget;
        
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
        
        bool isComplete();
        bool showNextPage();
        
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
        
        bool m_textsOK;
};

class BugzillaCommitPage : public DrKonqiAssistantPage
{
    Q_OBJECT
    
    public:
        BugzillaCommitPage( CrashInfo * );
        
        void aboutToShow();
        
    private Q_SLOTS:
        void commited(int);
        void commitError( QString );
        
        void retryClicked();
        
    private:
        KPushButton *   m_retryButton;
        StatusWidget * m_statusWidget;
        CrashInfo *  m_crashInfo;
        
    Q_SIGNALS:
    
        void finished();

};

#endif
