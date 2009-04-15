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

namespace KWallet { class Wallet; }
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
    
    QFormLayout *   m_form;

    KPushButton *   m_loginButton;
    KLineEdit *     m_userEdit;
    KLineEdit *     m_passwordEdit;
    QCheckBox   *   m_savePasswordCheckBox;
    
    StatusWidget *  m_statusWidget;

    KWallet::Wallet *   m_wallet;
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
    KLineEdit * m_keywordsEdit;
    bool    m_keywordsOK;
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

private Q_SLOTS:
    void searchFinished(const BugMapList&);
    void searchError(QString);

    void openSelectedReport();
    void itemClicked(QTreeWidgetItem *, int);
    void itemSelectionChanged();

    void bugFetchFinished(BugReport);
    void bugFetchError(QString);

    void searchMore();
    void performSearch();

    void checkBoxChanged(int);
    void mayBeDuplicateClicked();

    void enableControls(bool);

    bool canSearchMore();

private:
    void resetDates();
    
    bool            m_searching;

    StatusWidget *  m_statusWidget;

    QDate           m_startDate;
    QDate           m_endDate;

    KPushButton *   m_searchMoreButton;
    KPushButton *   m_openReportButton;
    QTreeWidget *   m_bugListWidget;

    KDialog *       m_infoDialog;
    KTextBrowser *  m_infoDialogBrowser;
    QLabel *        m_infoDialogLink;
    StatusWidget *  m_infoDialogStatusWidget;

    QCheckBox *     m_foundDuplicateCheckBox;
    KLineEdit *     m_possibleDuplicateEdit;
    KPushButton *   m_mineMayBeDuplicateButton;

    int             m_currentBugNumber;
    QString         m_currentKeywords;
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
    QLabel *        m_titleLabel;
    KLineEdit *     m_titleEdit;
    QLabel *        m_detailsLabel;
    KTextEdit *     m_detailsEdit;

    bool m_textsOK;
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
    void sendUsingDefaults();

private:
    KPushButton *   m_retryButton;
    StatusWidget *  m_statusWidget;

Q_SIGNALS:

    void finished(bool);

};

#endif
