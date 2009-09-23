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
#ifndef DEBUGGERCONFIG_H
#define DEBUGGERCONFIG_H

#include <QtCore/QString>
#include <QtCore/QSharedDataPointer>
class CrashedApplication;
struct DebuggerConfigPrivate;

class DebuggerConfig
{
public:
    DebuggerConfig();
    DebuggerConfig(const DebuggerConfig & other);
    DebuggerConfig & operator=(const DebuggerConfig & other);
    virtual ~DebuggerConfig();

    static DebuggerConfig loadFromConfig(const QString & debuggerName);

    /** Returns true if this DebuggerConfig instance can be used, or false otherwise */
    bool isValid() const;

    /** Returns the internal name of the debugger (eg. "gdb") */
    QString debuggerName() const;

    /** Returns the command that should be run to get an instance of the debugger running externally */
    QString externalDebuggerCommand() const;

    /** Returns whether the external debugger should be run in a terminal */
    bool externalDebuggerRunInTerminal() const;

    /** Returns the batch command that should be run to get a backtrace for use in drkonqi */
    QString debuggerBatchCommand() const;

    /** Returns the commands that should be given to the debugger when
     * run in batch mode in order to generate a backtrace
     */
    QString backtraceBatchCommands() const;

    /** Returns the executable name that drkonqi should check if it exists
     * to determine whether the debugger is installed
     */
    QString tryExec() const;

    enum ExpandStringUsage {
        ExpansionUsagePlainText,
        ExpansionUsageShell
    };

    static void expandString(QString & str, const CrashedApplication & appInfo,
                             ExpandStringUsage usage = ExpansionUsagePlainText,
                             const QString & tempFile = QString());

private:
    DebuggerConfig(DebuggerConfigPrivate *dd);
    QSharedDataPointer<DebuggerConfigPrivate> d;
};

#endif // DEBUGGERCONFIG_H
