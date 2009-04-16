/*******************************************************************
* drkonqibugreport.cpp
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

#include "drkonqibugreport.h"
#include "drkonqiassistantpages_base.h"
#include "drkonqiassistantpages_bugzilla.h"
#include "aboutbugreportingdialog.h"
#include "reportinfo.h"
#include "drkonqi_globals.h"

#include <QtGui/QCloseEvent>
#include <KMessageBox>

DrKonqiBugReport::DrKonqiBugReport(QWidget * parent) :
        KAssistantDialog(parent),
        m_aboutBugReportingDialog(0),
        m_reportInfo(new ReportInfo(this)),
        m_bugzillaManager(new BugzillaManager(this)),
        m_canClose(false)
{
    KGlobal::ref();
    setAttribute(Qt::WA_DeleteOnClose, true);

    setWindowTitle(i18nc("@title:window","Crash Reporting Assistant"));
    setWindowIcon(KIcon("tools-report-bug"));

    connect(this, SIGNAL(currentPageChanged(KPageWidgetItem *, KPageWidgetItem *)),
            this, SLOT(currentPageChanged_slot(KPageWidgetItem *, KPageWidgetItem *)));
    connect(this, SIGNAL(helpClicked()), this, SLOT(showHelp()));

    //Introduction Page
    IntroductionPage * m_intro = new IntroductionPage(this);
    connectSignals(m_intro);

    KPageWidgetItem * m_introPage = new KPageWidgetItem(m_intro, QLatin1String(PAGE_INTRODUCTION_ID));
    m_introPage->setHeader(i18nc("@title", "Introduction"));
    m_introPage->setIcon(KIcon("tools-report-bug"));

    //Crash Information Page
    CrashInformationPage * m_backtrace = new CrashInformationPage(this);
    connectSignals(m_backtrace);

    KPageWidgetItem * m_backtracePage = new KPageWidgetItem(m_backtrace, 
                                                        QLatin1String(PAGE_CRASHINFORMATION_ID));
    m_backtracePage->setHeader(i18nc("@title","Crash Information (Backtrace)"));
    m_backtracePage->setIcon(KIcon("tools-report-bug"));

    //Bug Awareness Page
    BugAwarenessPage * m_awareness = new BugAwarenessPage(this);
    connectSignals(m_awareness);

    KPageWidgetItem * m_awarenessPage = new KPageWidgetItem(m_awareness, 
                                                                QLatin1String(PAGE_AWARENESS_ID));
    m_awarenessPage->setHeader(i18nc("@title","What do you know about the crash?"));
    m_awarenessPage->setIcon(KIcon("tools-report-bug"));

    //Results Page
    ConclusionPage * m_conclusions = new ConclusionPage(this);
    connectSignals(m_conclusions);

    KPageWidgetItem * m_conclusionsPage = new KPageWidgetItem(m_conclusions,
                                                                QLatin1String(PAGE_CONCLUSIONS_ID));
    m_conclusionsPage->setHeader(i18nc("@title","Crash Analysis Results"));
    m_conclusionsPage->setIcon(KIcon("tools-report-bug"));
    connect(m_conclusions, SIGNAL(finished(bool)), this, SLOT(assistantFinished(bool)));

    //Bugzilla Login
    BugzillaLoginPage * m_bugzillaLogin =  new BugzillaLoginPage(this);
    connectSignals(m_bugzillaLogin);

    KPageWidgetItem * m_bugzillaLoginPage = new KPageWidgetItem(m_bugzillaLogin,
                                                                QLatin1String(PAGE_BZLOGIN_ID));
    m_bugzillaLoginPage->setHeader(i18nc("@title","KDE Bug Tracking System Login"));
    m_bugzillaLoginPage->setIcon(KIcon("tools-report-bug"));

    //Bugzilla keywords
    BugzillaKeywordsPage * m_bugzillaKeywords =  new BugzillaKeywordsPage(this);
    connectSignals(m_bugzillaKeywords);

    KPageWidgetItem * m_bugzillaKeywordsPage =  new KPageWidgetItem(m_bugzillaKeywords, 
                                                                QLatin1String(PAGE_BZKEYWORDS_ID));
    m_bugzillaKeywordsPage->setHeader(i18nc("@title","Bug Report Keywords"));
    m_bugzillaKeywordsPage->setIcon(KIcon("tools-report-bug"));

    //Bugzilla duplicates
    BugzillaDuplicatesPage * m_bugzillaDuplicates =  new BugzillaDuplicatesPage(this);
    connectSignals(m_bugzillaDuplicates);

    KPageWidgetItem * m_bugzillaDuplicatesPage = new KPageWidgetItem(m_bugzillaDuplicates, 
                                                            QLatin1String(PAGE_BZDUPLICATES_ID));
    m_bugzillaDuplicatesPage->setHeader(i18nc("@title","Bug Report Possible Duplicates List"));
    m_bugzillaDuplicatesPage->setIcon(KIcon("tools-report-bug"));

    //Bugzilla information
    BugzillaInformationPage * m_bugzillaInformation =  new BugzillaInformationPage(this);
    connectSignals(m_bugzillaInformation);

    KPageWidgetItem * m_bugzillaInformationPage = new KPageWidgetItem(m_bugzillaInformation, 
                                                                QLatin1String(PAGE_BZDETAILS_ID));
    m_bugzillaInformationPage->setHeader(i18nc("@title","Details of the Bug Report"));
    m_bugzillaInformationPage->setIcon(KIcon("tools-report-bug"));

    //Bugzilla commit
    BugzillaSendPage * m_bugzillaSend =  new BugzillaSendPage(this);

    KPageWidgetItem * m_bugzillaSendPage = new KPageWidgetItem(m_bugzillaSend, 
                                                                    QLatin1String(PAGE_BZSEND_ID));
    m_bugzillaSendPage->setHeader(i18nc("@title","Send Crash Report"));
    m_bugzillaSendPage->setIcon(KIcon("tools-report-bug"));
    connect(m_bugzillaSend, SIGNAL(finished(bool)), this, SLOT(assistantFinished(bool)));

    //TODO remember to keep ordered

    addPage(m_introPage);
    addPage(m_backtracePage);
    addPage(m_awarenessPage);
    addPage(m_conclusionsPage);
    addPage(m_bugzillaLoginPage);
    addPage(m_bugzillaKeywordsPage);
    addPage(m_bugzillaDuplicatesPage);
    addPage(m_bugzillaInformationPage);
    addPage(m_bugzillaSendPage);

    setMinimumSize(QSize(600, 400));
    resize(QSize(600, 400));
}

DrKonqiBugReport::~DrKonqiBugReport()
{
    KGlobal::deref();
}

void DrKonqiBugReport::connectSignals(DrKonqiAssistantPage * page)
{
    connect(page, SIGNAL(completeChanged(DrKonqiAssistantPage*, bool)),
             this, SLOT(completeChanged(DrKonqiAssistantPage*, bool)));
}

void DrKonqiBugReport::currentPageChanged_slot(KPageWidgetItem * current , KPageWidgetItem * before)
{
    //Reset standard buttons state
    //setButtonGuiItem(KDialog::User1,m_finishButtonItem);
    
    //showButton(KDialog::Close, true);
    //enableButton(KDialog::Close, true);
    
    enableButton(KDialog::Cancel, true);
    m_canClose = false;

    if (before) {
        DrKonqiAssistantPage* beforePage = dynamic_cast<DrKonqiAssistantPage*>(before->widget());
        beforePage->aboutToHide();
    }

    if (current) {
        DrKonqiAssistantPage* currentPage = dynamic_cast<DrKonqiAssistantPage*>(current->widget());
        enableNextButton(currentPage->isComplete());
        currentPage->aboutToShow();
    }
        
    //Disable all buttons on the bugzilla send page (until we get the send result)
    if (current->name() == QLatin1String(PAGE_BZSEND_ID)) { 
        enableNextButton(false);
        enableBackButton(false);
        enableButton(KDialog::User1, false);
    }
}

void DrKonqiBugReport::completeChanged(DrKonqiAssistantPage* page, bool isComplete)
{
    if (page == dynamic_cast<DrKonqiAssistantPage*>(currentPage()->widget())) {
        enableNextButton(isComplete);
    }
}

void DrKonqiBugReport::assistantFinished(bool showBack)
{
    enableNextButton(false);
    enableBackButton(showBack);
    enableButton(KDialog::User1, true);
    enableButton(KDialog::Cancel, false);
    
    /*
    showButton(KDialog::Cancel, false);
    showButton(KDialog::User1, false);
    showButton(KDialog::Close, true);
    enableButton(KDialog::Close, true);
    */
    
    //Show close instead of finish
    //setButtonGuiItem(KDialog::User1,m_closeButtonItem);
    
    m_canClose = true;
}

void DrKonqiBugReport::showHelp()
{
    if (!m_aboutBugReportingDialog) {
        m_aboutBugReportingDialog = new AboutBugReportingDialog(this);
    }
    m_aboutBugReportingDialog->show();
    m_aboutBugReportingDialog->showSection(QLatin1String(PAGE_HELP_BEGIN_ID));
    m_aboutBugReportingDialog->showSection(currentPage()->name());
}

//Override KAssistantDialog "next" page implementation
void DrKonqiBugReport::next()
{
    DrKonqiAssistantPage * page = dynamic_cast<DrKonqiAssistantPage*>(currentPage()->widget());
    if (page) {
        if (page->showNextPage()) { //Time to ask the user if we need to
            KAssistantDialog::next();
        }
    } else {
        KAssistantDialog::next();
    }
}

void DrKonqiBugReport::enableNextButton(bool enabled)
{
    enableButton(KDialog::User2, enabled);
}

void DrKonqiBugReport::enableBackButton(bool enabled)
{
    enableButton(KDialog::User3, enabled);
}

void DrKonqiBugReport::reject()
{
    close();
}

void DrKonqiBugReport::closeEvent(QCloseEvent * event)
{
    if (!m_canClose) {
        if (KMessageBox::questionYesNo(this, i18nc("@info","Do you really want to close the bug "
                                                   "assistant without sending the bug report?"),
                                       i18nc("@title:window","Close Crash Reporting Assistant"))
                                        == KMessageBox::Yes) {
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}
