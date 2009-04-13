/*******************************************************************
* aboutbugreportingdialog.cpp
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

#include "aboutbugreportingdialog.h"
#include "drkonqi_globals.h"

#include <KLocale>
#include <KTextBrowser>

AboutBugReportingDialog::AboutBugReportingDialog(QWidget * parent):
        KDialog(parent)
{
    setWindowIcon(KIcon("help-hint"));
    setCaption(i18nc("title", "About Bug Reporting"));
    setModal(true);

    setButtons(KDialog::Close);
    setDefaultButton(KDialog::Close);

    m_textBrowser = new KTextBrowser(this);
    m_textBrowser->setMinimumSize(QSize(500, 300));

    QString text =

        QString("<a name=\"Begin\" /><h1>%1</h1>").arg(
                                        Qt::escape(i18n("Information about bug reporting"))) +
        QString("<p>%1</p><p>%2</p><p>%3</p>").arg(
            Qt::escape(i18n("You can help us improve this software by filing a bug report.")),
            Qt::escape(i18n("Note: It is safe to close this dialog. If you do not want to, "
                            "you do not have to file a bug report.")),
            Qt::escape(i18n("In order to generate a useful bug report we need some information "
                            "about both the crash and your system. ( Also, you may need to install "
                            "some packages )"))) +

        QString("<h1>%1</h1>").arg(Qt::escape(i18n("Bug Reporting Assistant Steps Guide"))) +
        QString("<p>%1</p>").arg(
            Qt::escape(i18n("This assistant will guide you through the crash reporting process of "
                            "the Bug Reports Database Site of KDE. All the information you enter "
                            "to the bug report must be in ENGLISH as KDE is an international "
                            "software"))) +

        QString("<a name=\"Backtrace\" /><h2>%1</h2>").arg(Qt::escape(i18n("Crash Information"))) +
        QString("<p>%1<a href=\"%2\">%2</a></p><p>%3</p>").arg(
            Qt::escape(i18n("This page will generate the trace of the crash. This information is "
                            "really useful to know where and why the application crashed. If the "
                            "crash information isn't useful enough, you may need to install some "
                            "packages and reload it. You can find more information about the traces "
                            "of a crash on ")),
            QString(TECHBASE_HOWTO_DOC),
            Qt::escape(i18n("Once you get an useful trace (or if you don't want to install the "
                            "missing packages) you can continue."))) +

        QString("<a name=\"Awareness\" /><h2>%1</h2>").arg(
                                        Qt::escape(i18n("What do you know about the crash"))) +
        QString("<p>%1</p><p>%2</p>").arg(
            Qt::escape(i18n("In this page you need to answer some questions about the crash context "
                            "and your will to help the developers in the future if it's needed.")),
            Qt::escape(i18n("If you can describe the crash context (the details of what were you "
                            "doing when the application crashed: you can mention: Documents that "
                            "you were using (or its type/format), actions you were taking in the "
                            "application and the whole system, web sites you were browsing, another "
                            "strange things you notices after of before the crash, and so on."))) +

        QString("<a name=\"Results\" /><h2>%1</h2>").arg(Qt::escape(i18n("Conclusions"))) +
        QString("<p>%1</p><p>%2</p><p>%3</p>").arg(
            Qt::escape(i18n("Using the usefulness of the crash information gathered previously , "
                            "and your answers on the previous page, the assistant will tell you if "
                            "the crash is worth reporting or not.")),
            Qt::escape(i18n("If the crash is worth reporting, and the application is supported in "
                            "the KDE bug tracking system, you can click next. However if it's not "
                            "supported you will need to contact the maintainer of the application.")),
            Qt::escape(i18n("If the crash is not worth reporting, you can still manually report "
                            "the bug if you really want to."))) +

        QString("<a name=\"BugzillaLogin\" /><h2>%1</h2>").arg(
                                        Qt::escape(i18n("KDE Bug Tracking System Login"))) +
        QString("<p>%1<a href=\"%2\">%2</a></p><p>%3</p><p>%4</p>").arg(
            Qt::escape(i18n("As we may need to contact you in the future to ask further information "
                            "and we need to keep record of the bug reports, you need to have an "
                            "account on the KDE bug tracking system. If you don't have one, you can "
                            "freely create it here: ")),
            QString(KDE_BUGZILLA_CREATE_ACCOUNT_URL),
            Qt::escape(i18n("Then you can write your username and password and press the Login "
                            "button. Once you are autenticated, you can press Next to continue.")),
            Qt::escape(i18n("The KWallet dialog may appear when pressing Login to save your password "
                            "in the KWallet password system. Also, it will prompt you for the "
                            "KWallet password on loading to autocomplete the data if you use this "
                            "assistant again."))) +

        QString("<a name=\"BugzillaKeywords\" /><h2>%1</h2>").arg(
                                        Qt::escape(i18n("Bug Report Keywords"))) +
        QString("<p>%1</p><p>%2</p>").arg(
            Qt::escape(i18n("In this step you need to write at least four(4) words to identify the "
                            "crash. You can use the application name, or write a little sentence "
                            "about the crash or the context. This text will be used to search "
                            "similar crashes already reported in the database, so you can look at "
                            "them later. Also, this text can be used as the title of the future bug "
                            "report (you can change this later)")),
            Qt::escape(i18n("You should write in English"))) +

        QString("<a name=\"BugzillaDuplicates\" /><h2>%1</h2>").arg(
                                        Qt::escape(i18n("Bug Report Possible Duplicate list"))) +
        QString("<p>%1</p><p>%2</p><p>%3</p><p>%4</p>").arg(
            Qt::escape(i18n("This is an optional step")),
            Qt::escape(i18n("This page will search at the bug report database for similar crashes "
                            "(possible duplicates). If there are some bug reports found, you can "
                            "double click on them to show their details. Then, reading the current "
                            "bug report information you can check if the situations were similar and "
                            "you can tentatively mark your crash as a duplicate of that report.")),
            Qt::escape(i18n("If the listed possible duplicates aren't enough (or you didn't found a "
                            "similar report) you can force the search of more bug reports (only if "
                            "the date range is not reached)")),
            Qt::escape(i18n("Don't worry if you can't find a similar bug report or you don't know "
                            "what to look at. The bug report database maintainers will look at it "
                            "later"))) +

        QString("<a name=\"BugzillaInformation\" /><h2>%1</h2>").arg(
                                        Qt::escape(i18n("Details of the Bug report"))) +
        QString("<p>%1</p><p>%2</p>").arg(
            Qt::escape(i18n("In this case you need to write a title and description of the crash "
                            "context. Explain the best you can.")),
            Qt::escape(i18n("You should write in English"))) +

        QString("<a name=\"BugzillaSend\" /><h2>%1</h2>").arg(Qt::escape(i18n("Send Crash Report"))) +
        QString("<p>%1</p><p>%2</p>").arg(
            Qt::escape(i18n("This page will send the bug report to the bug tracking system and it "
                            "will notify you when it is done. Then it will show the URL of the bug "
                            "report in the KDE bug tracking system, so that you can look at the "
                            "report.")),
            Qt::escape(i18n("If the process fails, you can click Retry to try sending the bug "
                            "report again.")));

    m_textBrowser->setText(text);

    setMainWidget(m_textBrowser);
}

void AboutBugReportingDialog::showSection(QString anchor)
{
    m_textBrowser->scrollToAnchor(anchor);
}

