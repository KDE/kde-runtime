/*******************************************************************
* aboutbugreportingdialog.cpp
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
* Copyright 2009    A. L. Spehr <spehr@kde.org>
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

#include "aboutbugreportingdialog.h"
#include "drkonqi_globals.h"

#include <KLocale>
#include <KToolInvocation>
#include <KTextBrowser>

AboutBugReportingDialog::AboutBugReportingDialog(QWidget * parent):
        KDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    setWindowIcon(KIcon("help-hint"));
    setCaption(i18nc("@title title of the dialog", "About Bug Reporting"));

    setButtons(KDialog::Close);
    setDefaultButton(KDialog::Close);

    m_textBrowser = new KTextBrowser(this);
    m_textBrowser->setMinimumSize(QSize(500, 300));
    m_textBrowser->setNotifyClick(true);
    connect(m_textBrowser, SIGNAL(urlClick(QString)), this, SLOT(handleInternalLinks(QString)));

    QString text =

        //Introduction
        QString("<a name=\"%1\" /><h1>%2</h1>").arg(QLatin1String(PAGE_HELP_BEGIN_ID),
                                i18nc("@title","Information about bug reporting")) +
        QString("<p>%1</p><p>%2</p><p>%3</p>").arg(
            i18nc("@info/rich", "You can help us improve this software by filing a bug report."),
            i18nc("@info/rich","<note>It is safe to close this dialog. If you do not "
                            "want to, you do not have to file a bug report.</note>"),
            i18nc("@info/rich","In order to generate a useful bug report we need some "
                            "information about both the crash and your system. (You may also "
                            "need to install some debug packages.)")) +
        //Sub-introduction
        QString("<h1>%1</h1>").arg(i18nc("@title","Bug Reporting Assistant Steps Guide")) +
        QString("<p>%1</p>").arg(
            i18nc("@info/rich","This assistant will guide you through the crash "
                            "reporting process for the KDE Bug Reports Database Site. All the "
                            "information you enter on the bug report must be in English, if "
                            "possible, as KDE is developed internationally.")) +
        //Crash Information Page   I suspect this needs more information...
        QString("<a name=\"%1\" /><h2>%2</h2>").arg(QLatin1String(PAGE_CRASHINFORMATION_ID),
                                i18nc("@title","Crash Information")) +
        QString("<p>%1</p><p>%2</p>").arg(
            i18nc("@info/rich","This page will generate a backtrace of the crash. This "
                            "is information that tells the developers where the application "
                            "crashed. If the crash information is not detailed enough, you may "
                            "need to install some debug packages and reload it. You can find "
                            "more information about backtraces, what they mean, and how they "
                            "are useful at <link>%1</link>",QString(TECHBASE_HOWTO_DOC) ),
            i18nc("@info/rich","Once you get a useful backtrace (or if you do not want to "
                            "install the missing debugging packages) you can continue.")) +
        //Bug Awareness Page
        QString("<a name=\"%1\" /><h2>%2</h2>").arg(QLatin1String(PAGE_AWARENESS_ID),
                                i18nc("@title","What do you know about the crash?")) +
        QString("<p>%1</p><p>%2<ul><li>%3</li><li>%4</li><li>%5</li><li>%6</li><li>%7</li>"
        "</ul>%8</p>").arg(
            i18nc("@info/rich","In this page you answer some questions about the crash context, "
                            "and note whether you are willing to help the developers in "
                            "the future. To do this, you will need to open a KDE "
                            "Bug tracking system account. This is strongly encouraged, as "
                            "often a developer may need to ask you for clarification. "
                            "Also, you can track the status of your bug report by having "
                            "email updates sent to you."),
            i18nc("@info/rich","If you can, describe in as much detail as possible, "
                            "the crash circumstances, and "
                            "what you were doing when the application crashed. You can mention: "),
            i18nc("@info/rich crash situation example","actions you were taking inside or outside "
                            "the application"),
            i18nc("@info/rich crash situation example","documents or images that you were using "
                            "and their type/format (later if you go to look at the report in the "
                            "bug tracking system, you can attach a file to the report)"),
            i18nc("@info/rich crash situation example","widgets that you were running"),
            i18nc("@info/rich crash situation example","the url of a web site you were browsing"),
            i18nc("@info/rich crash situation example","or other strange things you notice before "
                            "or after the crash. "),
            i18nc("@info/rich","Screenshots can be very helpful. You can attach them to the bug "
                            "report after it is posted to the bug tracking system.")) +
        //Conclusions Page
        QString("<a name=\"%1\" /><h2>%2</h2>").arg(QLatin1String(PAGE_CONCLUSIONS_ID),
                                i18nc("@title","Conclusions")) +
        QString("<p>%1</p><p>%2</p><p>%3</p>").arg(
            i18nc("@info/rich","Using the quality of the crash information gathered, "
                            "and your answers on the previous page, the assistant will "
                            "tell you if the crash is worth reporting or not."),
            i18nc("@info/rich","If the crash is worth reporting, and the application "
                            "is supported in the KDE bug tracking system, you can click "
                            "<interface>Next</interface>. However, if it is not supported, you "
                            "will need to directly contact the maintainer of the application."),
            i18nc("@info/rich","If the crash is listed as being not worth reporting, "
                            "and you think the assistant has made a mistake, "
                             "you can still manually report the bug by logging into the "
                             "bug tracking system. You can also go back and change information "
                            "and download debug packages.")) +
        //Bugzilla Login Page
        QString("<a name=\"%1\" /><h2>%2</h2>").arg(QLatin1String(PAGE_BZLOGIN_ID),
                                    i18nc("@title","KDE Bug Tracking System Login")) +
        QString("<p>%1</p><p>%2</p><p>%3</p>").arg(
            i18nc("@info/rich","We may need to contact you in the future to ask for "
                            "further information. As we need to keep track of the bug reports, "
                            "you "
                            "need to have an account on the KDE bug tracking system. If you do "
                            "not have one, you can create one here: <link>%1</link>",
                            QString(KDE_BUGZILLA_CREATE_ACCOUNT_URL)),
            i18nc("@info/rich","Then, enter your username and password and "
                            "press the Login button. Once you are authenticated, you can press "
                            "Next to continue. You can use this login to directly access the "
                            "KDE bug tracking system later."),
            i18nc("@info/rich","The KWallet dialog may appear when pressing Login to "
                            "save your password in the KWallet password system. Also, it will "
                            "prompt you for the KWallet password upon loading to autocomplete "
                            "the login fields if you use this assistant again.")) +
        //Bugzilla Duplicates Page
        QString("<a name=\"%1\" /><h2>%2</h2>").arg(QLatin1String(PAGE_BZDUPLICATES_ID),
                                i18nc("@title","Bug Report Possible Duplicate list")) +
        QString("<p>%1</p><p>%2</p><p>%3</p><p>%4</p><p>%5</p><p>%6</p>").arg(
        //needs some more string cleanup below
            i18nc("@info/rich","This is an optional step."),
            i18nc("@info/rich","This page will search the bug report database for "
                            "similar crashes which are possible duplicates of your bug. If "
                            "there are similar bug reports found, you can double click on them "
                            "to see details. Then, read the current bug report information so "
                            "you can check to see if they are similar. "),
            i18nc("@info/rich","If you are very sure your bug is the same as another that is "
                            "previously reported, you can open the previously "
                            "reported crash in the bug tracking program by clicking on "
                            "<interface>Bug report page at the KDE bug tracking "
                            "system</interface>. You can then add "
                            "any new information that you might have. "),
            i18nc("@info/rich","If you are unsure whether your report is the same, follow the main "
                            "options to tentatively mark your crash as a duplicate of that "
                            "report. This is usually the safest thing to do. We cannot "
                            "uncombine bug reports, but we can easily merge them."),
            i18nc("@info/rich","If not enough possible duplicates are found, or you "
                            "did not find a similar report, then you can force it to search "
                            "for more bug reports (only if the date range is not reached.)"),
            i18nc("@info/rich","Do not worry if you cannot find a similar bug report "
                            "or you do not know what to look at. The bug report database "
                            "maintainers will look at it later. It is better to file a "
                            "duplicate "
                            "then to not file at all.")) +
        //Bugzilla Crash Information - Details Page
        QString("<a name=\"%1\" /><h2>%2</h2>").arg(QLatin1String(PAGE_BZDETAILS_ID),
                                        i18nc("@title","Details of the Bug report")) +
        QString("<p>%1<a href=\"#%2\">%3</a></p><p>%4</p>").arg(
            i18nc("@info/rich","In this case you need to write a title and description "
                            "of the crash. Explain as best you can. "),
            QLatin1String(PAGE_AWARENESS_ID),
            i18nc("@title","What do you know about the crash?"),
            i18nc("@info/rich","<note>You should write in English.</note>")) +
        //Bugzilla Send Page
        QString("<a name=\"%1\" /><h2>%2</h2>").arg(QLatin1String(PAGE_BZSEND_ID),
                                       i18nc("@title","Send Crash Report")) +
        QString("<p>%1</p><p>%2</p>").arg(
            i18nc("@info/rich","This page will send the bug report to the bug tracking "
                            "system and will notify you when it is done. It will then show "
                            "the web address of the bug report in the KDE bug tracking system, "
                            "so that you can look at the report later."),
            i18nc("@info/rich","If the process fails, you can click "
                            "<interface>Retry</interface> to try sending the bug report again."));
            //I feel like we should now congratulate them and ask them to join BugSquad.

    m_textBrowser->setText(text);

    setMainWidget(m_textBrowser);
}

void AboutBugReportingDialog::handleInternalLinks(const QString& url)
{
    if (!url.isEmpty()) {
        if (url.startsWith('#')) {
            showSection(url.mid(1,url.length()));
        } else {
            KToolInvocation::invokeBrowser(url);
        }
    }
}

void AboutBugReportingDialog::showSection(const QString& anchor)
{
    m_textBrowser->scrollToAnchor(anchor);
}

