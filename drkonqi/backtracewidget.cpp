/*******************************************************************
* backtracewidget.cpp
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

#include "backtracewidget.h"
#include "backtraceratingwidget.h"

#include "drkonqi.h"
#include "backtracegenerator.h"
#include "backtraceparser.h"
#include "drkonqi_globals.h"
#include "debuggermanager.h"

#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>

#include <KIcon>
#include <KMessageBox>
#include <KLocale>

const char *extraDetailsLabelMargin = " margin: 5px; ";

BacktraceWidget::BacktraceWidget(BacktraceGenerator *generator, QWidget *parent) :
        QWidget(parent),
        m_btGenerator(generator)
{
    ui.setupUi(this);

    //Debug package installer
    m_debugPackageInstaller = new DebugPackageInstaller(this);
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

    m_backtraceRatingWidget = new BacktraceRatingWidget(ui.m_statusWidget);
    ui.m_statusWidget->addCustomStatusWidget(m_backtraceRatingWidget);

    ui.m_statusWidget->setIdle(QString());
}

void BacktraceWidget::setAsLoading()
{
    ui.m_backtraceEdit->setText(i18nc("@info:status", "Loading..."));
    ui.m_backtraceEdit->setEnabled(false);

    ui.m_statusWidget->setBusy(i18nc("@info:status",
                                     "Loading crash information... (this may take some time)"));
    m_backtraceRatingWidget->setUsefulness(BacktraceParser::Useless);
    m_backtraceRatingWidget->setState(BacktraceGenerator::Loading);

    ui.m_extraDetailsLabel->setVisible(false);
    ui.m_extraDetailsLabel->clear();

    ui.m_installDebugButton->setVisible(false);
    ui.m_reloadBacktraceButton->setEnabled(false);

    ui.m_copyButton->setEnabled(false);
    ui.m_saveButton->setEnabled(false);
}

//Force backtrace generation
void BacktraceWidget::regenerateBacktrace()
{
    setAsLoading();

    if (!DrKonqi::debuggerManager()->debuggerIsRunning()) {
        m_btGenerator->start();
    } else {
        anotherDebuggerRunning();
    }

    emit stateChanged();
}

void BacktraceWidget::generateBacktrace()
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

void BacktraceWidget::anotherDebuggerRunning()
{
    ui.m_backtraceEdit->setEnabled(false);
    ui.m_backtraceEdit->setText(i18nc("@info", "Another debugger is currently debugging the "
                                   "same application. The crash information could not be fetched."));
    m_backtraceRatingWidget->setState(BacktraceGenerator::Failed);
    m_backtraceRatingWidget->setUsefulness(BacktraceParser::Useless);
    ui.m_statusWidget->setIdle(i18nc("@info:status", "The crash information could not be fetched."));
    ui.m_extraDetailsLabel->setVisible(true);
    ui.m_extraDetailsLabel->setText(i18nc("@info/rich", "Another debugging process is attached to "
                                    "the crashed application. Therefore, the DrKonqi debugger cannot "
                                    "fetch the backtrace. Please close the other debugger and "
                                    "click <interface>Reload</interface>."));
    ui.m_installDebugButton->setVisible(false);
    ui.m_reloadBacktraceButton->setEnabled(true);
}

void BacktraceWidget::loadData()
{
    m_backtraceRatingWidget->setState(m_btGenerator->state());

    if (m_btGenerator->state() == BacktraceGenerator::Loaded) {
        ui.m_backtraceEdit->setEnabled(true);
        ui.m_backtraceEdit->setPlainText(m_btGenerator->backtrace());

        BacktraceParser * btParser = m_btGenerator->parser();
        m_backtraceRatingWidget->setUsefulness(btParser->backtraceUsefulness());

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
            //FIXME 4.5 Split this text, the button should only appear if the installdbgsymbols
            //script is available. the text about the textbase article should appear always, or
            //at least, when the script is not available.
            ui.m_extraDetailsLabel->setVisible(true);
            ui.m_extraDetailsLabel->setText(i18nc("@info/rich", "You can click the <interface>"
                                "Install Debug Symbols</interface> button in order to automatically "
                                "install the missing debugging information packages. If this method "
                                "does not work: please read <link url='%1'>How to "
                                "create useful crash reports</link> to learn how to get a useful "
                                "backtrace; install the needed packages and click the "
                                "<interface>Reload</interface> button.",
                                QLatin1String(TECHBASE_HOWTO_DOC)));
            ui.m_installDebugButton->setVisible(true);
            //Show the "Install dbg symbols" button disabled if the script is not available
            ui.m_installDebugButton->setEnabled(canInstallDebugPackages());
            
            QStringList missingLibraries = btParser->librariesWithMissingDebugSymbols().toList();
            m_debugPackageInstaller->setMissingLibraries(missingLibraries);
        }

        ui.m_copyButton->setEnabled(true);
        ui.m_saveButton->setEnabled(true);
    } else if (m_btGenerator->state() == BacktraceGenerator::Failed) {
        m_backtraceRatingWidget->setUsefulness(BacktraceParser::Useless);

        ui.m_statusWidget->setIdle(i18nc("@info:status", "The debugger has quit unexpectedly."));

        ui.m_backtraceEdit->setPlainText(i18nc("@info:status",
                                               "The crash information could not be generated."));

        ui.m_extraDetailsLabel->setVisible(true);
        ui.m_extraDetailsLabel->setText(i18nc("@info/rich", "You could try to regenerate the "
                                            "backtrace by clicking the <interface>Reload"
                                            "</interface> button."));
    } else if (m_btGenerator->state() == BacktraceGenerator::FailedToStart) {
        m_backtraceRatingWidget->setUsefulness(BacktraceParser::Useless);

        ui.m_statusWidget->setIdle(i18nc("@info:status", "The debugger application is missing or "
                                                         "could not be launched."));

        ui.m_backtraceEdit->setPlainText(i18nc("@info:status",
                                               "The crash information could not be generated."));
        ui.m_extraDetailsLabel->setVisible(true);
        ui.m_extraDetailsLabel->setText(i18nc("@info/rich", "You need to install the debugger "
                                              "package (%1) and click the <interface>Reload"
                                              "</interface> button.",
                                              m_btGenerator->debugger().name()));
    }

    ui.m_reloadBacktraceButton->setEnabled(true);
    emit stateChanged();
}

void BacktraceWidget::backtraceNewLine(const QString & line)
{
    ui.m_backtraceEdit->append(line.trimmed());
}

void BacktraceWidget::copyClicked()
{
    ui.m_backtraceEdit->selectAll();
    ui.m_backtraceEdit->copy();
}

void BacktraceWidget::saveClicked()
{
    DrKonqi::saveReport(m_btGenerator->backtrace(), this);
}

void BacktraceWidget::hilightExtraDetailsLabel(bool hilight)
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

void BacktraceWidget::focusImproveBacktraceButton()
{
    ui.m_installDebugButton->setFocus();
}

void BacktraceWidget::installDebugPackages()
{
    ui.m_installDebugButton->setVisible(false);
    m_debugPackageInstaller->installDebugPackages();
}

void BacktraceWidget::debugPackageError(const QString & errorMessage)
{
    ui.m_installDebugButton->setVisible(true);
    KMessageBox::error(this, errorMessage, i18nc("@title:window", "Error during the installation of"
                                                                                " debug symbols"));
}

void BacktraceWidget::debugPackageCanceled()
{
    ui.m_installDebugButton->setVisible(true);    
}

bool BacktraceWidget::canInstallDebugPackages() const
{
    return m_debugPackageInstaller->canInstallDebugPackages();
}
