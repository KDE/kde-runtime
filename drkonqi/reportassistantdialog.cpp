/*******************************************************************
* reportassistantdialog.cpp
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

#include "reportassistantdialog.h"

#include "drkonqi_globals.h"

#include "drkonqi.h"
#include "krashconf.h"

#include "aboutbugreportingdialog.h"

#include "reportassistantpages_base.h"
#include "reportassistantpages_bugzilla.h"
#include "reportinterface.h"

#include <QtGui/QCloseEvent>
#include <KMessageBox>

ReportAssistantDialog::ReportAssistantDialog(QWidget * parent) :
        KAssistantDialog(parent),
        m_aboutBugReportingDialog(0),
        m_reportInterface(new ReportInterface(this)),
        m_bugzillaManager(new BugzillaManager(this)),
        m_canClose(false)
{
    KGlobal::ref();
    setAttribute(Qt::WA_DeleteOnClose, true);

    //Just for Testing FIXME removeme!! 
    //m_bugzillaManager->setCustomBugtrackerUrl("http://bugstest.kde.org/");
    
    setWindowTitle(i18nc("@title:window","Crash Reporting Assistant"));
    setWindowIcon(KIcon("tools-report-bug"));

    connect(this, SIGNAL(currentPageChanged(KPageWidgetItem *, KPageWidgetItem *)),
            this, SLOT(currentPageChanged_slot(KPageWidgetItem *, KPageWidgetItem *)));
    connect(this, SIGNAL(helpClicked()), this, SLOT(showHelp()));
    
    //Bug Awareness Page
    BugAwarenessPage * m_awareness = new BugAwarenessPage(this);
    connectSignals(m_awareness);

    KPageWidgetItem * m_awarenessPage = new KPageWidgetItem(m_awareness, 
                                                                QLatin1String(PAGE_AWARENESS_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_AWARENESS_ID),m_awarenessPage);
    m_awarenessPage->setHeader(i18nc("@title","What do you know about the crash?"));
    m_awarenessPage->setIcon(KIcon("tools-report-bug"));
    
    //Crash Information Page
    CrashInformationPage * m_backtrace = new CrashInformationPage(this);
    connectSignals(m_backtrace);

    KPageWidgetItem * m_backtracePage = new KPageWidgetItem(m_backtrace, 
                                                        QLatin1String(PAGE_CRASHINFORMATION_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_CRASHINFORMATION_ID),m_backtracePage);
    m_backtracePage->setHeader(i18nc("@title","Crash Information (Backtrace)"));
    m_backtracePage->setIcon(KIcon("tools-report-bug"));

    //Results Page
    ConclusionPage * m_conclusions = new ConclusionPage(this);
    connectSignals(m_conclusions);

    KPageWidgetItem * m_conclusionsPage = new KPageWidgetItem(m_conclusions,
                                                                QLatin1String(PAGE_CONCLUSIONS_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_CONCLUSIONS_ID),m_conclusionsPage);
    m_conclusionsPage->setHeader(i18nc("@title","Crash Analysis Results"));
    m_conclusionsPage->setIcon(KIcon("tools-report-bug"));
    connect(m_conclusions, SIGNAL(finished(bool)), this, SLOT(assistantFinished(bool)));

    //Bugzilla Login
    BugzillaLoginPage * m_bugzillaLogin =  new BugzillaLoginPage(this);
    connectSignals(m_bugzillaLogin);

    KPageWidgetItem * m_bugzillaLoginPage = new KPageWidgetItem(m_bugzillaLogin,
                                                                QLatin1String(PAGE_BZLOGIN_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZLOGIN_ID),m_bugzillaLoginPage);
    m_bugzillaLoginPage->setHeader(i18nc("@title","KDE Bug Tracking System Login"));
    m_bugzillaLoginPage->setIcon(KIcon("tools-report-bug"));
    connect(m_bugzillaLogin, SIGNAL(loggedTurnToNextPage()), this, SLOT(next()));
    
    //Bugzilla duplicates
    BugzillaDuplicatesPage * m_bugzillaDuplicates =  new BugzillaDuplicatesPage(this);
    connectSignals(m_bugzillaDuplicates);

    KPageWidgetItem * m_bugzillaDuplicatesPage = new KPageWidgetItem(m_bugzillaDuplicates, 
                                                            QLatin1String(PAGE_BZDUPLICATES_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZDUPLICATES_ID),m_bugzillaDuplicatesPage);
    m_bugzillaDuplicatesPage->setHeader(i18nc("@title","Bug Report Possible Duplicates List"));
    m_bugzillaDuplicatesPage->setIcon(KIcon("tools-report-bug"));

    //Bugzilla information
    BugzillaInformationPage * m_bugzillaInformation =  new BugzillaInformationPage(this);
    connectSignals(m_bugzillaInformation);

    KPageWidgetItem * m_bugzillaInformationPage = new KPageWidgetItem(m_bugzillaInformation, 
                                                                QLatin1String(PAGE_BZDETAILS_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZDETAILS_ID),m_bugzillaInformationPage);
    m_bugzillaInformationPage->setHeader(i18nc("@title","Details of the Bug Report"));
    m_bugzillaInformationPage->setIcon(KIcon("tools-report-bug"));

    //Bugzilla Report Preview
    BugzillaPreviewPage * m_bugzillaPreview =  new BugzillaPreviewPage(this);

    KPageWidgetItem * m_bugzillaPreviewPage = new KPageWidgetItem(m_bugzillaPreview, 
                                                                QLatin1String(PAGE_BZPREVIEW_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZPREVIEW_ID),m_bugzillaPreviewPage);
    m_bugzillaPreviewPage->setHeader(i18nc("@title","Preview Report"));
    m_bugzillaPreviewPage->setIcon(KIcon("tools-report-bug"));
    
    //Bugzilla commit
    BugzillaSendPage * m_bugzillaSend =  new BugzillaSendPage(this);

    KPageWidgetItem * m_bugzillaSendPage = new KPageWidgetItem(m_bugzillaSend, 
                                                                    QLatin1String(PAGE_BZSEND_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZSEND_ID),m_bugzillaSendPage);
    m_bugzillaSendPage->setHeader(i18nc("@title","Send Crash Report"));
    m_bugzillaSendPage->setIcon(KIcon("tools-report-bug"));
    connect(m_bugzillaSend, SIGNAL(finished(bool)), this, SLOT(assistantFinished(bool)));

    //TODO remember to keep ordered
    addPage(m_awarenessPage);
    addPage(m_backtracePage);
    addPage(m_conclusionsPage);
    addPage(m_bugzillaLoginPage);
    addPage(m_bugzillaDuplicatesPage);
    addPage(m_bugzillaInformationPage);
    addPage(m_bugzillaPreviewPage);
    addPage(m_bugzillaSendPage);

    setMinimumSize(QSize(600, 400));
    resize(QSize(600, 400));
}

ReportAssistantDialog::~ReportAssistantDialog()
{
    KGlobal::deref();
}

void ReportAssistantDialog::connectSignals(ReportAssistantPage * page)
{
    connect(page, SIGNAL(completeChanged(ReportAssistantPage*, bool)),
             this, SLOT(completeChanged(ReportAssistantPage*, bool)));
}

void ReportAssistantDialog::currentPageChanged_slot(KPageWidgetItem * current , KPageWidgetItem * before)
{
    enableButton(KDialog::Cancel, true);
    m_canClose = false;

    //Save data of the previous page
    if (before) {
        ReportAssistantPage* beforePage = dynamic_cast<ReportAssistantPage*>(before->widget());
        beforePage->aboutToHide();
    }

    //Load data of the current page
    if (current) {
        ReportAssistantPage* currentPage = dynamic_cast<ReportAssistantPage*>(current->widget());
        enableNextButton(currentPage->isComplete());
        currentPage->aboutToShow();
    }
        
    //If the current page is the last one, disable all the buttons until we get the bug sent or err
    if (current->name() == QLatin1String(PAGE_BZSEND_ID)) { 
        enableNextButton(false);
        enableButton(KDialog::User3, false); //Back button
        enableButton(KDialog::User1, false);
    }
}

void ReportAssistantDialog::completeChanged(ReportAssistantPage* page, bool isComplete)
{
    if (page == dynamic_cast<ReportAssistantPage*>(currentPage()->widget())) {
        enableNextButton(isComplete);
    }
}

void ReportAssistantDialog::assistantFinished(bool showBack)
{
    //The assistant finished: allow the user to close the dialog normally
    
    enableNextButton(false);
    enableButton(KDialog::User3, showBack); //Back button
    enableButton(KDialog::User1, true);
    enableButton(KDialog::Cancel, false);
    
    m_canClose = true;
}

void ReportAssistantDialog::showHelp()
{
    if (!m_aboutBugReportingDialog) {
        m_aboutBugReportingDialog = new AboutBugReportingDialog();
    }
    m_aboutBugReportingDialog->show();
    m_aboutBugReportingDialog->raise();
    m_aboutBugReportingDialog->activateWindow();
    m_aboutBugReportingDialog->showSection(QLatin1String(PAGE_HELP_BEGIN_ID));
    m_aboutBugReportingDialog->showSection(currentPage()->name());
}

//Override KAssistantDialog "next" page implementation
void ReportAssistantDialog::next()
{
    //Allow the widget to Ask a question to the user before changing the page
    
    ReportAssistantPage * page = dynamic_cast<ReportAssistantPage*>(currentPage()->widget());
    if (page) {
        if (!page->showNextPage()) {
            return;
        }
    }
    
    //If the information the user can provide is not useful, skip the backtrace page
    if (currentPage()->name() == QLatin1String(PAGE_AWARENESS_ID))
    {
        //Force save settings in the current page
        page->aboutToHide();

        if (!(m_reportInterface->userCanDetail() || m_reportInterface->developersCanContactReporter()))
        {
            setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_CONCLUSIONS_ID)));
            return;
        }
    } else {
        if (currentPage()->name() == QLatin1String(PAGE_CRASHINFORMATION_ID))
        {
            //Force save settings in current page
            page->aboutToHide();

            //If the crash is worth reporting and it is BKO, skip the Conclusions page
            if (m_reportInterface->isWorthReporting() && 
                                                DrKonqi::instance()->krashConfig()->isKDEBugzilla())
            {
                setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_BZLOGIN_ID)));
                return;
            }
        }
    }
    
    KAssistantDialog::next();
}

//Override KAssistantDialog "back"(previous) page implementation
//It has to mirror the custom next() implementation
void ReportAssistantDialog::back()
 {
    if (currentPage()->name() == QLatin1String(PAGE_CONCLUSIONS_ID))
    {
        if (!(m_reportInterface->userCanDetail() || m_reportInterface->developersCanContactReporter()))
        {
            setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_AWARENESS_ID)));
            return;
        }
    }
    
    if (currentPage()->name() == QLatin1String(PAGE_BZLOGIN_ID))
    {
        if (m_reportInterface->isWorthReporting() && DrKonqi::instance()->krashConfig()->isKDEBugzilla())
        {
            setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_CRASHINFORMATION_ID)));
            return;
        }
    }
    
    KAssistantDialog::back();
}
 
void ReportAssistantDialog::enableNextButton(bool enabled)
{
    enableButton(KDialog::User2, enabled);
}

void ReportAssistantDialog::reject()
{
    close();
}

void ReportAssistantDialog::closeEvent(QCloseEvent * event)
{
    if (!m_canClose) {
        if (KMessageBox::questionYesNo(this, i18nc("@info","Do you really want to close the bug "
                                                   "assistant without submitting the bug report?"),
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
