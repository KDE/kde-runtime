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
#include "debuggerconfig.h"
#include "crashedapplication.h"

#include <KConfig>
#include <KConfigGroup>
#include <KGlobal>
#include <KMacroExpanderBase>

struct DebuggerConfigPrivate : public QSharedData
{
    QString debuggerName;
    QString debuggerCommand;
    QString debuggerBatchCommand;
    QString tryExec;
    QString backtraceCommand;
    bool runInTerminal;
};

//static
DebuggerConfig DebuggerConfig::loadFromKConfig(const KConfig & config)
{
    DebuggerConfigPrivate *d = new DebuggerConfigPrivate;
    const KConfigGroup generalGroup = config.group("General");
    d->debuggerCommand = generalGroup.readPathEntry("Exec", QString());
    d->debuggerBatchCommand = generalGroup.readPathEntry("ExecBatch", QString());
    d->tryExec = generalGroup.readPathEntry("TryExec", QString());
    d->backtraceCommand = generalGroup.readEntry("BacktraceCommand");
    d->runInTerminal = generalGroup.readEntry("Terminal", false);
    return DebuggerConfig(d);
}

DebuggerConfig::DebuggerConfig()
    : d(new DebuggerConfigPrivate)
{
}

DebuggerConfig::DebuggerConfig(DebuggerConfigPrivate *dd)
    : d(dd)
{
}

DebuggerConfig::DebuggerConfig(const DebuggerConfig & other)
    : d(other.d)
{
}

DebuggerConfig & DebuggerConfig::operator=(const DebuggerConfig & other)
{
    d = other.d;
    return *this;
}

DebuggerConfig::~DebuggerConfig()
{
}

bool DebuggerConfig::isValid() const
{
    return !d->debuggerName.isEmpty();
}

QString DebuggerConfig::debuggerName() const
{
    return d->debuggerName;
}

QString DebuggerConfig::externalDebuggerCommand() const
{
    return d->debuggerCommand;
}

bool DebuggerConfig::externalDebuggerRunInTerminal() const
{
    return d->runInTerminal;
}

QString DebuggerConfig::debuggerBatchCommand() const
{
    return d->debuggerBatchCommand;
}

QString DebuggerConfig::backtraceBatchCommands() const
{
    return d->backtraceCommand;
}

QString DebuggerConfig::tryExec() const
{
    return d->tryExec;
}

//static
void DebuggerConfig::expandString(QString & str, const CrashedApplication & appInfo,
                                  ExpandStringUsage usage, const QString & tempFile)
{
    QHash<QString, QString> map;
    map[QLatin1String("progname")] = appInfo.name();
    map[QLatin1String("execname")] = appInfo.executable().fileName();
    map[QLatin1String("execpath")] = appInfo.executable().absoluteFilePath();
    map[QLatin1String("signum")] = QString::number(appInfo.signalNumber());
    map[QLatin1String("signame")] = appInfo.signalName();
    map[QLatin1String("pid")] = QString::number(appInfo.pid());
    map[QLatin1String("tempfile")] = tempFile;

    if (usage == ExpansionUsageShell) {
        str = KMacroExpander::expandMacrosShellQuote(str, map);
    } else {
        str = KMacroExpander::expandMacros(str, map);
    }
}

