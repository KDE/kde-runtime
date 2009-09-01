/*******************************************************************
* getbacktracewidget.cpp
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

#include "getbacktracewidget.h"
#include "usefulnessmeter.h"
#include "drkonqi.h"
#include "backtracegenerator.h"
#include "backtraceparser.h"
#include "krashconf.h"
#include "drkonqi_globals.h"

#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>

#include <KIcon>
#include <KMessageBox>
#include <KLocale>

const char *extraDetailsLabelMargin = " margin: 5px; ";

GetBacktraceWidget::GetBacktraceWidget(BacktraceGenerator *generator, QWidget *parent) :
        QWidget(parent),
        m_btGenerator(generator)
{
    ui.setupUi(this);

    //Debug package installer
    m_debugPackageInstaller = new DebugPackageInstaller(DrKonqi::instance()->krashConfig()->productName());
    connect(m_debugPackageInstaller, SIGNAL(error(QString)), this, SLOT(debugPackageError(QString)));
    connect(m_debugPackageInstaller, SIGNAL(packagesInstalled()), this, SLOT(regenerateBacktrace()));
    connect(m_debugPackageInstaller, SIGNAL(canceled()), this, SLOT(debugPackageCanceled()));
    
    connect(m_btGenerator, SIGNAL(done()) , this, SLOT(loadData()));
    connect(m_btGenerator, SIGNAL(someError()) , this, SLOT(loadData()));
    connect(m_btGenerator, SIGNAL(failedToStart()) , this, SLOT(loadData()));
    connect(m_btGenerator, SIGNAL(newLine(QString)) , this, SLOT(backtraceNewLine(QString)));

    ui.m_extraDetailsLabel->setVisible(false);
    ui.m_extraDetailsLabel->setStyleSheet(QLatin1String(extraDetailsLabelMargin));

    ui.m_reloadBacktraceButton->setGuiItem(
                KGuiItem2(i18nc("@action:button", "&Reload"),
                          KIcon("view-refresh"), i18nc("@info:tooltip", "Use this button to "
                          "reload the crash information (backtrace). This is useful when you have "
                          "installed the proper debug symbol packages and you want to obtain "
                          "a better backtrace.")));
    connect(ui.m_reloadBacktraceButton, SIGNAL(clicked()), this, SLOT(regenerateBacktrace()));

    ui.m_installDebugButton->setGuiItem(
                KGuiItem2(i18nc("@action:button", "&Install Debug Symbols"),
                          KIcon("system-software-update"), i18nc("@info:tooltip", "Use this button to "
                          "install the missing debug symbols packages.")));
    ui.m_installDebugButton->setVisible(false);
    connect(ui.m_installDebugButton, SIGNAL(clicked()), this, SLOT(installDebugPackages()));

    ui.m_copyButton->setGuiItem(KGuiItem2(QString(), KIcon("edit-copy"),
                                          i18nc("@info:tooltip", "Use this button to copy the "
                                                "crash information (backtrace) to the clipboard.")));
    connect(ui.m_copyButton, SIGNAL(clicked()) , this, SLOT(copyClicked()));
    ui.m_copyButton->setEnabled(false);

    ui.m_saveButton->setGuiItem(KGuiItem2(QString(),
                                          KIcon("document-save"),
                                          i18nc("@info:tooltip", "Use this button to save the "
                                          "crash information (backtrace) to a file. This is useful "
                                          "if you want to take a look at it or to report the bug "
                                          "later.")));
    connect(ui.m_saveButton, SIGNAL(clicked()) , this, SLOT(saveClicked()));
    ui.m_saveButton->setEnabled(false);

    //StatusWidget

    m_usefulnessMeter = new UsefulnessMeter(ui.m_statusWidget);
    ui.m_statusWidget->addCustomStatusWidget(m_usefulnessMeter);

    ui.m_statusWidget->setIdle(QString());
}

void GetBacktraceWidget::setAsLoading()
{
    ui.m_backtraceEdit->setText(i18nc("@info:status", "Loading..."));
    ui.m_backtraceEdit->setEnabled(false);

    ui.m_statusWidget->setBusy(i18nc("@info:status",
                                     "Loading crash information... (this may take some time)"));
    m_usefulnessMeter->setUsefulness(BacktraceParser::Useless);
    m_usefulnessMeter->setState(BacktraceGenerator::Loading);

    ui.m_extraDetailsLabel->setVisible(false);
    ui.m_extraDetailsLabel->clear();

    ui.m_installDebugButton->setVisible(false);
    ui.m_reloadBacktraceButton->setEnabled(false);

    ui.m_copyButton->setEnabled(false);
    ui.m_saveButton->setEnabled(false);
}

//Force backtrace generation
void GetBacktraceWidget::regenerateBacktrace()
{
    setAsLoading();

    if (DrKonqi::instance()->currentState() != DrKonqi::DebuggerRunning) {
        m_btGenerator->start();
    } else {
        anotherDebuggerRunning();
    }

    emit stateChanged();
}

void GetBacktraceWidget::generateBacktrace()
{
    if (m_btGenerator->state() == BacktraceGenerator::NotLoaded) {
        regenerateBacktrace();    //First backtrace generation
    } else if (m_btGenerator->state() == BacktraceGenerator::Loading) {
        setAsLoading(); //Set in loading state, the widget will catch the backtrace events anyway
        emit stateChanged();
    } else { //*Finished* states
        setAsLoading();
        emit stateChanged();
        loadData(); //Load already gathered information
    }
}

void GetBacktraceWidget::anotherDebuggerRunning()
{
    ui.m_backtraceEdit->setEnabled(false);
    ui.m_backtraceEdit->setText(i18nc("@info", "Another debugger is currently debugging the "
                                   "same application. The crash information could not be fetched."));
    m_usefulnessMeter->setState(BacktraceGenerator::Failed);
    m_usefulnessMeter->setUsefulness(BacktraceParser::Useless);
    ui.m_statusWidget->setIdle(i18nc("@info:status", "The crash information could not be fetched."));
    ui.m_extraDetailsLabel->setVisible(true);
    ui.m_extraDetailsLabel->setText(i18nc("@info/rich", "Another debugging process is attached to "
                                    "the crashed application. Therefore, the DrKonqi debugger cannot "
                                    "fetch the backtrace. Please close the other debugger and "
                                    "click <interface>Reload Crash Information</interface>."));
    ui.m_installDebugButton->setVisible(false);
    ui.m_reloadBacktraceButton->setEnabled(true);
}

void GetBacktraceWidget::loadData()
{
    m_usefulnessMeter->setState(m_btGenerator->state());

    if (m_btGenerator->state() == BacktraceGenerator::Loaded) {
        ui.m_backtraceEdit->setEnabled(true);
        ui.m_backtraceEdit->setPlainText(m_btGenerator->backtrace());

        BacktraceParser * btParser = m_btGenerator->parser();
        m_usefulnessMeter->setUsefulness(btParser->backtraceUsefulness());

        QString usefulnessText;
        switch (btParser->backtraceUsefulness()) {
        case BacktraceParser::ReallyUseful:
            usefulnessText = i18nc("@info", "This crash information is useful");
            break;
        case BacktraceParser::MayBeUseful:
            usefulnessText = i18nc("@info", "This crash information may be useful");
            break;
        case BacktraceParser::ProbablyUseless:
            usefulnessText = i18nc("@info", "This crash information is probably not useful");
            break;
        case BacktraceParser::Useless:
            usefulnessText = i18nc("@info", "This crash information is not useful");
            break;
        default:
            //let's hope nobody will ever see this... ;)
            usefulnessText = i18nc("@info", "The rating of this crash information is invalid. "
                                            "This is a bug in drkonqi itself.");
            break;
        }
        ui.m_statusWidget->setIdle(usefulnessText);

        if (btParser->backtraceUsefulness() != BacktraceParser::ReallyUseful) {
            ui.m_extraDetailsLabel->setVisible(true);
            ui.m_extraDetailsLabel->setText(i18nc("@info/rich", "Please read <link url='%1'>How to "
                                "create useful crash reports</link> to learn how to get a useful "
                                "backtrace. After you install the needed packages, click the "
                                "<interface>Reload Crash Information</interface> button.",
                                QLatin1String(TECHBASE_HOWTO_DOC)));
            ui.m_installDebugButton->setVisible(true);
            
            QStringList missingLibraries = btParser->librariesWithMissingDebugSymbols().toList();
            m_debugPackageInstaller->setMissingLibraries(missingLibraries);
        }

        ui.m_copyButton->setEnabled(true);
        ui.m_saveButton->setEnabled(true);
    } else if (m_btGenerator->state() == BacktraceGenerator::Failed) {
        m_usefulnessMeter->setUsefulness(BacktraceParser::Useless);

        ui.m_statusWidget->setIdle(i18nc("@info:status", "The debugger has quit unexpectedly."));

        ui.m_backtraceEdit->setPlainText(i18nc("@info:status",
                                               "The crash information could not be generated."));

        ui.m_extraDetailsLabel->setVisible(true);
        ui.m_extraDetailsLabel->setText(i18nc("@info/rich", "You could try to regenerate the "
                                            "backtrace by clicking the <interface>Reload Crash "
                                            "Information</interface> button."));
    } else if (m_btGenerator->state() == BacktraceGenerator::FailedToStart) {
        m_usefulnessMeter->setUsefulness(BacktraceParser::Useless);

        ui.m_statusWidget->setIdle(i18nc("@info:status", "The debugger application is missing or "
                                                         "could not be launched."));

        ui.m_backtraceEdit->setPlainText(i18nc("@info:status",
                                               "The crash information could not be generated."));
        ui.m_extraDetailsLabel->setVisible(true);
        ui.m_extraDetailsLabel->setText(i18nc("@info/rich", "You need to install the debugger "
                                              "package (%1) and click the <interface>Reload Crash "
                                              "Information</interface> button.",
                                              DrKonqi::instance()->krashConfig()->debuggerName()));
    }

    ui.m_reloadBacktraceButton->setEnabled(true);
    emit stateChanged();
}

void GetBacktraceWidget::backtraceNewLine(const QString & line)
{
    ui.m_backtraceEdit->append(line.trimmed());
}

void GetBacktraceWidget::copyClicked()
{
    ui.m_backtraceEdit->selectAll();
    ui.m_backtraceEdit->copy();
}

void GetBacktraceWidget::saveClicked()
{
    DrKonqi::saveReport(m_btGenerator->backtrace(), this);
}

void GetBacktraceWidget::hilightExtraDetailsLabel(bool hilight)
{
    QString stylesheet;
    if (hilight) {
        stylesheet = QLatin1String("border-width: 2px; "
                                    "border-style: solid; "
                                    "border-color: red;");
    } else {
        stylesheet = QLatin1String("border-width: 0px;");
    }
    stylesheet += QLatin1String(extraDetailsLabelMargin);
    ui.m_extraDetailsLabel->setStyleSheet(stylesheet);
}

void GetBacktraceWidget::focusReloadButton()
{
    ui.m_reloadBacktraceButton->setFocus();
    //FIXME change message and force m_installDebugButton focus
}

void GetBacktraceWidget::installDebugPackages()
{
    ui.m_installDebugButton->setVisible(false);
    m_debugPackageInstaller->installDebugPackages();
}

void GetBacktraceWidget::debugPackageError(const QString & errorMessage)
{
    ui.m_installDebugButton->setVisible(true);
    KMessageBox::error(this, errorMessage, i18nc("@title:window", "Error during the installation of"
                                                                                " debug symbols"));
}

void GetBacktraceWidget::debugPackageCanceled()
{
    ui.m_installDebugButton->setVisible(true);    
}
