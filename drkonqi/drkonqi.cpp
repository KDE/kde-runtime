/*
    Copyright (C) 2009  George Kiagiadakis <gkiagia@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Parts of this code were originally under the following license:

    * Copyright (C) 2000-2003 Hans Petter Bieker <bieker@kde.org>
    *
    * Redistribution and use in source and binary forms, with or without
    * modification, are permitted provided that the following conditions
    * are met:
    *
    * 1. Redistributions of source code must retain the above copyright
    *    notice, this list of conditions and the following disclaimer.
    * 2. Redistributions in binary form must reproduce the above copyright
    *    notice, this list of conditions and the following disclaimer in the
    *    documentation and/or other materials provided with the distribution.
    *
    * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "drkonqi.h"

#include "drkonqi_globals.h"
#include "krashconf.h"
#include "drkonqiadaptor.h"
#include "detachedprocessmonitor.h"
#include "backtracegenerator.h"
#include "systeminformation.h"

#include <QtDBus/QDBusConnection>
#include <QtCore/QTimer>
#include <QtCore/QPointer>

#include <KProcess>
#include <KStandardDirs>
#include <KDebug>
#include <KMessageBox>
#include <KFileDialog>
#include <KTemporaryFile>
#include <KToolInvocation>
#include <KIO/NetAccess>

#include <cstdlib>
#include <cerrno>
#include <sys/types.h>
#include <signal.h>

struct DrKonqi::Private {
    Private() : m_state(ProcessRunning), m_krashConfig(NULL), m_btGenerator(NULL),
    m_systemInformation(NULL), m_applicationRestarted(false) {}

    DrKonqi::State         m_state;
    KrashConfig *          m_krashConfig;
    DetachedProcessMonitor m_debuggerMonitor;
    BacktraceGenerator *   m_btGenerator;
    SystemInformation *    m_systemInformation;
    bool                   m_applicationRestarted;
};

DrKonqi::DrKonqi()
        : QObject(), d(new Private)
{
    QDBusConnection::sessionBus().registerObject("/krashinfo", this);
    new DrKonqiAdaptor(this);
}

DrKonqi::~DrKonqi()
{
    delete d;
}

//static
DrKonqi *DrKonqi::instance()
{
    static DrKonqi *drKonqiInstance = NULL;
    if (!drKonqiInstance) {
        drKonqiInstance = new DrKonqi();
    }
    return drKonqiInstance;
}

bool DrKonqi::init()
{
    //stop the process to avoid high cpu usage by other threads (bug 175362).
    //if the process was started by kdeinit, we need to wait a bit for KCrash
    //to reach the alarm(0); call in kdeui/util/kcrash.cpp line 406 or else
    //if we stop it before this call, pending alarm signals will kill the
    //process when we try to continue it.
    QTimer::singleShot(2000, this, SLOT(stopAttachedProcess()));

    // Make sure that DrKonqi will not be saved in the session
    unsetenv("SESSION_MANAGER");

    //arguments processing is done here
    d->m_krashConfig = new KrashConfig(this);

    //check whether the attached process exists and whether we have permissions to inspect it
    if (d->m_krashConfig->pid() <= 0) {
        kError() << "Invalid pid specified";
        return false;
    }

    if (::kill(d->m_krashConfig->pid(), 0) < 0) {
        switch (errno) {
        case EPERM:
            kError() << "DrKonqi doesn't have permissions to inspect the specified process";
            break;
        case ESRCH:
            kError() << "The specified process does not exist.";
            break;
        default:
            break;
        }
        return false;
    }

    //misc initializations
    d->m_btGenerator = new BacktraceGenerator(d->m_krashConfig, this);
    connect(d->m_btGenerator, SIGNAL(starting()), this, SLOT(debuggerStarting()));
    connect(d->m_btGenerator, SIGNAL(done()), this, SLOT(debuggerStopped()));
    connect(d->m_btGenerator, SIGNAL(someError()), this, SLOT(debuggerStopped()));
    connect(d->m_btGenerator, SIGNAL(failedToStart()), this, SLOT(debuggerStopped()));
    connect(&d->m_debuggerMonitor, SIGNAL(processFinished()), this, SLOT(debuggerStopped()));

    //System information
    d->m_systemInformation = new SystemInformation(this);
    
    return true;
}

void DrKonqi::cleanup()
{
    if (d->m_btGenerator->state() == BacktraceGenerator::Loading) {
        //if the debugger is running, kill it and continue the process.
        delete d->m_btGenerator;
        d->m_state = ProcessStopped;
    }
    continueAttachedProcess();
    delete this;
}

DrKonqi::State DrKonqi::currentState() const
{
    return d->m_state;
}

const KrashConfig *DrKonqi::krashConfig() const
{
    Q_ASSERT(d->m_krashConfig != NULL);
    return d->m_krashConfig;
}

BacktraceGenerator *DrKonqi::backtraceGenerator() const
{
    Q_ASSERT(d->m_btGenerator != NULL);
    return d->m_btGenerator;
}

SystemInformation *DrKonqi::systemInformation() const
{
    Q_ASSERT(d->m_systemInformation != NULL);
    return d->m_systemInformation; 
}

//static
void DrKonqi::saveReport(const QString & reportText, QWidget *parent)
{
    const KrashConfig *krashConfig = instance()->krashConfig();
    if (krashConfig->safeMode()) {
        KTemporaryFile tf;
        tf.setSuffix(".kcrash");
        tf.setAutoRemove(false);

        if (tf.open()) {
            QTextStream textStream(&tf);
            textStream << reportText;
            textStream.flush();
            KMessageBox::information(parent, i18nc("@info",
                                                   "Report saved to <filename>%1</filename>.",
                                                   tf.fileName()));
        } else {
            KMessageBox::sorry(parent, i18nc("@info","Could not create a file in which to save the report."));
        }
    } else {
        QString defname = krashConfig->appName() + '-'
                                + QDate::currentDate().toString("yyyyMMdd") + ".kcrash";
        if (defname.contains('/')) {
            defname = defname.mid(defname.lastIndexOf('/') + 1);
        }
        
        QPointer<KFileDialog> dlg = new KFileDialog(defname, QString(), parent);
        dlg->setSelection(defname);
        dlg->setCaption(i18nc("@title:window","Select Filename"));
        dlg->setOperationMode(KFileDialog::Saving);
        dlg->setMode(KFile::File);
        dlg->setConfirmOverwrite(true);
        dlg->exec();
        
        KUrl fileUrl = dlg->selectedUrl();
        delete dlg;
        if (fileUrl.isValid()) {
            KTemporaryFile tf;
            if (tf.open()) {
                QTextStream ts(&tf);
                ts << reportText;
                ts.flush();
            } else {
                KMessageBox::sorry(parent, i18nc("@info","Cannot open file <filename>%1</filename> "
                                                         "for writing.", tf.fileName()));
                return;
            }

            if (!KIO::NetAccess::upload(tf.fileName(), fileUrl, parent)) {
                KMessageBox::sorry(parent, KIO::NetAccess::lastErrorString());
            }
        }
    }
}

void DrKonqi::restartCrashedApplication()
{
    if (!d->m_applicationRestarted) {
        d->m_applicationRestarted = true;
        QString executable = KStandardDirs::findExe(d->m_krashConfig->executableName());
        kDebug() << "Restarting application" << executable;

        //start the application via kdeinit, as it needs to have a pristine environment and
        //KProcess::startDetached() can't start a new process with custom environment variables.
        KToolInvocation::kdeinitExec(executable);
    }
}

void DrKonqi::startDefaultExternalDebugger()
{
    Q_ASSERT(d->m_state != DebuggerRunning);

    QString str = d->m_krashConfig->externalDebuggerCommand();
    d->m_krashConfig->expandString(str, KrashConfig::ExpansionUsageShell);

    KProcess proc;
    proc.setShellCommand(str);

    debuggerStarting();
    int pid = proc.startDetached();
    d->m_debuggerMonitor.startMonitoring(pid);
}

void DrKonqi::startCustomExternalDebugger()
{
    debuggerStarting();
    emit acceptDebuggingApplication();
}

void DrKonqi::stopAttachedProcess()
{
    if (d->m_state == ProcessRunning) {
        ::kill(d->m_krashConfig->pid(), SIGSTOP);
        d->m_state = ProcessStopped;
    }
}

void DrKonqi::continueAttachedProcess()
{
    if (d->m_state == ProcessStopped) {
        ::kill(d->m_krashConfig->pid(), SIGCONT);
        d->m_state = ProcessRunning;
    }
}

void DrKonqi::debuggerStarting()
{
    continueAttachedProcess();
    d->m_state = DebuggerRunning;
    emit debuggerRunning(true);
}

void DrKonqi::debuggerStopped()
{
    d->m_state = ProcessRunning;
    stopAttachedProcess();
    emit debuggerRunning(false);
}

void DrKonqi::registerDebuggingApplication(const QString& launchName)
{
    emit newDebuggingApplication(launchName);
}

bool DrKonqi::appRestarted() const
{
    return d->m_applicationRestarted;
}

#include "drkonqi.moc"
