/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
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
 *****************************************************************/

#ifndef KRASHCONF_H
#define KRASHCONF_H

#include <QtCore/QObject>
#include <KAboutData>

class KrashConfig : public QObject
{
    Q_OBJECT

public:
    KrashConfig(QObject *parent);
    virtual ~KrashConfig();

public:

    //Infromation about the crashed application
    /** Returns the crashed program's long name (ex. "The KDE crash handler") */
    QString programName() const {
        return m_aboutData->programName();
    }
    /** Returns the crashed program's internal name (ex. "drkonqi") */
    QString appName() const {
        return m_aboutData->appName();
    }
    /** As in KAboutData: "Returns the application's product name, which will be used in KBugReport dialog.
     * By default it returns appName(), otherwise the one which is set with setProductName()" */
    QString productName() const {
        return m_aboutData->productName();
    }
    /** Returns the version of the crashed program */
    QString productVersion() const {
        return m_aboutData->version();
    }
    /** Returns the executable's name (with path to it, optionally) */
    QString executableName() const {
        return m_execname;
    }
    /** Returns the signal number that the crashed program received */
    int signalNumber() const {
        return m_signalnum;
    }
    /** Returns the pid of the crashed program */
    int pid() const {
        return m_pid;
    }
    /** Returns true if the crashed program was started by kdeinit */ //FIXME do we need that?
    bool startedByKdeinit() const {
        return m_startedByKdeinit;
    }

    /** Returns the name of the signal (ex. SIGSEGV) */
    QString signalName() const;

    //drkonqi options
    /** Returns true if drkonqi should not allow arbitrary disk write access */
    bool safeMode() const {
        return m_safeMode;
    }
    /** Returns whether the advanced option to start an external instance of the debugger should be enabled or not */
    bool showDebugger() const {
        return m_showdebugger && !m_debuggerCommand.isNull();
    }

    //Information about the debugger
    /** Returns the internal name of the debugger (eg. "gdb") */
    QString debuggerName() const {
        return m_debuggerName;
    }
    /** Returns the command that should be run to get an instance of the debugger running externally */
    QString externalDebuggerCommand() const {
        return m_debuggerCommand;
    }
    /** Returns the batch command that should be run to get a backtrace for use in drkonqi */
    QString debuggerBatchCommand() const {
        return m_debuggerBatchCommand;
    }
    /** Returns the commands that should be given to the debugger when run in batch mode in order to generate a backtrace */
    QString backtraceBatchCommands() const {
        return m_backtraceCommand;
    }
    /** Returns the executable name that drkonqi should check if it exists to determine if the debugger is installed */
    QString tryExec() const {
        return m_tryExec;
    }

    //Information and methods about a possible bug reporting
    bool isKDEBugzilla() const;
    bool isReportMail() const;
    QString getReportLink() const {
        return m_aboutData->bugAddress();
    }

    //Utility methods
    enum ExpandStringUsage {
        ExpansionUsagePlainText,
        ExpansionUsageRichText,
        ExpansionUsageShell
    };
    void expandString(QString &str, ExpandStringUsage usage, const QString &tempFile = QString()) const;

private:
    void readConfig();

    KAboutData *m_aboutData;
    int m_pid;
    int m_signalnum;
    bool m_showdebugger;
    bool m_startedByKdeinit;
    bool m_safeMode;
    QString m_execname;

    QString m_debuggerName;
    QString m_debuggerCommand;
    QString m_debuggerBatchCommand;
    QString m_tryExec;
    QString m_backtraceCommand;
};

#endif
