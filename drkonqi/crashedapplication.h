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
#include <QtCore/QFileInfo>

class DrKonqi;

class CrashedApplication
{
    Q_DISABLE_COPY(CrashedApplication);
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

protected:
    struct Private;
    friend class DrKonqi;
    CrashedApplication(Private *dd);
    static CrashedApplication *createFromKCrashData();

    Private *const d;
};

#endif // CRASHEDAPPLICATION_H
