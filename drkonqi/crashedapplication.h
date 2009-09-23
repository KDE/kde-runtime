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
*/
#ifndef CRASHEDAPPLICATION_H
#define CRASHEDAPPLICATION_H

#include "bugreportaddress.h"
#include <QtCore/QObject>
#include <QtCore/QFileInfo>

class KCrashBackend;

class CrashedApplication : QObject
{
    Q_OBJECT
public:
    virtual ~CrashedApplication();

    /** Returns the crashed program's name, possibly translated (ex. "The KDE crash handler") */
    QString name() const;

    /** Returns a QFileInfo with information about the executable that crashed */
    QFileInfo executable() const;

    /** Returns the version of the crashed program */
    QString version() const;

    /** Returns the address where the bug report for this application should go */
    BugReportAddress bugReportAddress() const;

    /** Returns the pid of the crashed program */
    int pid() const;

    /** Returns the signal number that the crashed program received */
    int signalNumber() const;

    /** Returns the name of the signal (ex. SIGSEGV) */
    QString signalName() const;

    bool hasBeenRestarted() const;

public slots:
    void restart();

protected:
    friend class KCrashBackend;
    CrashedApplication(QObject *parent = 0);

    int m_pid;
    int m_signalNumber;
    QString m_name;
    QFileInfo m_executable;
    QString m_version;
    BugReportAddress m_reportAddress;
    bool m_restarted;
};

#endif // CRASHEDAPPLICATION_H
