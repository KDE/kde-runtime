/*******************************************************************
* reportassistantpages_bugzilla.h
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

#ifndef REPORTASSISTANTPAGES__BUGZILLA__H
#define REPORTASSISTANTPAGES__BUGZILLA__H

#include "reportassistantpage.h"

#include "reportassistantpages_base.h"
#include "bugzillalib.h"

#include "ui_assistantpage_bugzilla_login.h"
#include "ui_assistantpage_bugzilla_information.h"
#include "ui_assistantpage_bugzilla_preview.h"
#include "ui_assistantpage_bugzilla_send.h"

namespace KWallet { class Wallet; }

/** Bugs.kde.org login **/
class BugzillaLoginPage: public ReportAssistantPage
{
    Q_OBJECT

public:
    BugzillaLoginPage(ReportAssistantDialog *);
    ~BugzillaLoginPage();

    void aboutToShow();
    bool isComplete();

private Q_SLOTS:
    void loginClicked();
    void loginFinished(bool);
    void loginError(QString);

    void walletLogin();

Q_SIGNALS:
    void loggedTurnToNextPage();
 
private:
    bool kWalletEntryExists();
    void openWallet();
    
    Ui::AssistantPageBugzillaLogin      ui;

    KWallet::Wallet *                   m_wallet;
    bool                                m_walletWasOpenedBefore;
};

/** Title and details page **/
class BugzillaInformationPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    BugzillaInformationPage(ReportAssistantDialog *);

    void aboutToShow();
    void aboutToHide();

    bool isComplete();
    bool showNextPage();

private Q_SLOTS:
    void showTitleExamples();

    void checkTexts();

private:
    Ui::AssistantPageBugzillaInformation    ui;

    bool                                    m_textsOK;
    bool                                    m_distributionComboSetup;
    bool                                    m_distroComboVisible;
};

/** Preview report page **/
class BugzillaPreviewPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    BugzillaPreviewPage(ReportAssistantDialog *);

    void aboutToShow();

    bool showNextPage();
    
private:
    Ui::AssistantPageBugzillaPreview    ui;
};

/** Send crash report page **/
class BugzillaSendPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    BugzillaSendPage(ReportAssistantDialog *);

    void aboutToShow();

private Q_SLOTS:
    void sent(int);
    void sendError(QString);

    void retryClicked();
    void finishClicked();
    
    void openReportContents();

private:
    Ui::AssistantPageBugzillaSend           ui;
    QString                                 reportUrl;
    
    QPointer<KDialog>                       m_contentsDialog;
    
Q_SIGNALS:
    void finished(bool);

};

#endif
