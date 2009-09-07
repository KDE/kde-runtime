/*******************************************************************
* debugpackageinstaller.cpp
* Copyright  2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
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

#include "debugpackageinstaller.h"
#include <config-drkonqi.h>

#include <QtGui/QProgressDialog>

#include <KStandardDirs>
#include <KShell>
#include <KDebug>
#include <KProcess>
#include <KLocale>

DebugPackageInstaller::DebugPackageInstaller(const QString & packageName, QObject *parent)
    : QObject(parent), m_installerProcess(0), m_progressDialog(0)
{
    m_packageName = packageName;
    m_executablePath = KStandardDirs::findExe(DEBUG_PACKAGE_INSTALLER_NAME); //defined from CMakeLists.txt
}

bool DebugPackageInstaller::canInstallDebugPackages() const
{
    return !m_executablePath.isEmpty();
}

void DebugPackageInstaller::setMissingLibraries(const QStringList & libraries)
{
    m_missingLibraries = libraries;
}

void DebugPackageInstaller::installDebugPackages()
{
    Q_ASSERT(canInstallDebugPackages());

    if (!m_installerProcess) {
        //Run process
        m_installerProcess = new KProcess(this);
        connect(m_installerProcess, SIGNAL(finished(int, QProcess::ExitStatus)), 
                                            this, SLOT(processFinished(int, QProcess::ExitStatus)));

        QStringList args = QStringList() << m_packageName << m_missingLibraries;
        m_installerProcess->setProgram(m_executablePath, args);
        m_installerProcess->start();
        
        //Show dialog
        m_progressDialog = new QProgressDialog(qobject_cast<QWidget*>(parent()));
        connect(m_progressDialog, SIGNAL(canceled()), this, SLOT(progressDialogCanceled()));
        m_progressDialog->setRange(0,0);
        m_progressDialog->setWindowTitle(i18nc("@title:window", "Missing debug symbols"));
        m_progressDialog->setLabelText(i18nc("@info:progress", "Requesting installation of missing "
                                                                    "debug symbols packages..."));
        m_progressDialog->show();
    }
}

void DebugPackageInstaller::progressDialogCanceled()
{
    m_progressDialog->deleteLater();
    m_progressDialog = 0;
    
    if (m_installerProcess) {
        if (m_installerProcess->state() == QProcess::Running) {
            disconnect(m_installerProcess, SIGNAL(finished(int, QProcess::ExitStatus)), 
                                            this, SLOT(processFinished(int, QProcess::ExitStatus)));
            m_installerProcess->kill();
            disconnect(m_installerProcess, SIGNAL(finished(int, QProcess::ExitStatus)), 
                                            m_installerProcess, SLOT(deleteLater()));
        }
        m_installerProcess = 0;
    }
    
    emit canceled();
}

void DebugPackageInstaller::processFinished(int exitCode, QProcess::ExitStatus)
{
    switch(exitCode){
        case ResultInstalled: {
            emit packagesInstalled();
            break;
        }
        case ResultNotPresent: {
            emit error(i18nc("@info:status", "The debug symbols packages for this application are "
                                                                                 "not present."));
            break;
        }
        case ResultNotImplemented: {
            emit error(i18nc("@info:status", "Your distribution does not provide a way to install "
                                     "debug symbols packages. (or you compiled KDE by source)."));
            break;
        }
        case ResultError: {
            emit error(i18nc("@info:status", "There was an error, the debug symbols packages could "
                                                                            "not be installed."));
            break;
        }
        default: {
            emit error(i18nc("@info:status", "Unhandled error. The debug symbols packages could "
                                                                            "not be installed."));
        }
    }
    
    m_progressDialog->cancel();
    
    delete m_progressDialog;
    m_progressDialog = 0;
    
    delete m_installerProcess;
    m_installerProcess = 0;
}
