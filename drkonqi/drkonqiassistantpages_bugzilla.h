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

#include "drkonqiassistantpages_base.h"
#include "bugzillalib.h"

#include "ui_assistantpage_bugzilla_login.h"
#include "ui_assistantpage_bugzilla_keywords.h"
#include "ui_assistantpage_bugzilla_duplicates.h"
#include "ui_assistantpage_bugzilla_duplicates_dialog.h"
#include "ui_assistantpage_bugzilla_information.h"
#include "ui_assistantpage_bugzilla_send.h"

namespace KWallet { class Wallet; }
class QDate;
class QTreeWidgetItem;
class BugzillaReportInformationDialog;

/** Bugs.kde.org login **/
class BugzillaLoginPage: public DrKonqiAssistantPage
{
    Q_OBJECT

public:
    BugzillaLoginPage(DrKonqiBugReport *);
    ~BugzillaLoginPage();

    void aboutToShow();
    bool isComplete();

private Q_SLOTS:
    void loginClicked();
    void loginFinished(bool);
    void loginError(QString);

    void walletLogin();

private:
    bool kWalletEntryExists();
    void openWallet();
    
    Ui::AssistantPageBugzillaLogin      ui;

    KWallet::Wallet *                   m_wallet;
};

/** Enter keywords page **/
class BugzillaKeywordsPage : public DrKonqiAssistantPage
{
    Q_OBJECT

public:
    BugzillaKeywordsPage(DrKonqiBugReport *);

    void aboutToShow();
    void aboutToHide();

    bool isComplete();

private Q_SLOTS:
    void textEdited(QString);

private:
    Ui::AssistantPageBugzillaKeywords   ui;
    bool                                m_keywordsOK;
};

/** Searching for duplicates and showing report information page**/
class BugzillaDuplicatesPage : public DrKonqiAssistantPage
{
    Q_OBJECT

public:
    BugzillaDuplicatesPage(DrKonqiBugReport *);
    ~BugzillaDuplicatesPage();

    void aboutToShow();
    void aboutToHide();

    void setPossibleDuplicateNumber(int);
    
private Q_SLOTS:
    void searchFinished(const BugMapList&);
    void searchError(QString);
    
    void stopCurrentSearch();
    
    void openSelectedReport();
    void itemClicked(QTreeWidgetItem *, int);
    void itemSelectionChanged();

    void searchMore();
    void performSearch();

    void checkBoxChanged(int);

    void enableControls(bool);

    bool canSearchMore();

private:
    void resetDates();
    
    bool                                        m_searching;

    Ui::AssistantPageBugzillaDuplicates         ui;
    
    BugzillaReportInformationDialog *           m_infoDialog;
    
    //Dates of current Results
    QDate                                       m_startDate;
    QDate                                       m_endDate;
    //Dates of searching process
    QDate                                       m_searchingStartDate;
    QDate                                       m_searchingEndDate;
    
    QString                                     m_currentKeywords;
};

/** Internal bug-info dialog **/
class BugzillaReportInformationDialog : public KDialog
{
    Q_OBJECT

public:
    BugzillaReportInformationDialog(BugzillaDuplicatesPage*parent=0);
    
    void showBugReport(int bugNumber);

private Q_SLOTS:
    void bugFetchFinished(BugReport);
    void bugFetchError(QString);
    
    void mayBeDuplicateClicked();
    
private:
    Ui::AssistantPageBugzillaDuplicatesDialog   ui;
    BugzillaDuplicatesPage *                      m_parent;
    int                                         m_bugNumber;
    
};

/** Title and details page **/
class BugzillaInformationPage : public DrKonqiAssistantPage
{
    Q_OBJECT

public:
    BugzillaInformationPage(DrKonqiBugReport *);

    void aboutToShow();
    void aboutToHide();

    bool isComplete();
    bool showNextPage();

private Q_SLOTS:
    void checkTexts();

private:
    Ui::AssistantPageBugzillaInformation    ui;

    bool                                    m_textsOK;
};

/** Send crash report page **/
class BugzillaSendPage : public DrKonqiAssistantPage
{
    Q_OBJECT

public:
    BugzillaSendPage(DrKonqiBugReport *);

    void aboutToShow();

private Q_SLOTS:
    void sent(int);
    void sendError(QString);

    void retryClicked();
    void finishClicked();

private:
    Ui::AssistantPageBugzillaSend           ui;
    QString                                 reportUrl;
    
Q_SIGNALS:
    void finished(bool);

};

#endif
