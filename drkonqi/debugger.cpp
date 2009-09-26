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
#include "debugger.h"
#include "crashedapplication.h"
#include "drkonqi.h"

#include <KConfig>
#include <KConfigGroup>
#include <KGlobal>
#include <KStandardDirs>
#include <KMacroExpanderBase>
#include <KDebug>

//static
QList<Debugger> Debugger::availableInternalDebuggers()
{
    return availableDebuggers("debuggers/internal/*");
}

//static
QList<Debugger> Debugger::availableExternalDebuggers()
{
    return availableDebuggers("debuggers/external/*");
}

bool Debugger::isValid() const
{
    return !m_config.isNull() && !m_backend.isEmpty();
}

QString Debugger::name() const
{
    return m_config->group("General").readEntry("Name");
}

QString Debugger::codeName() const
{
    //fall back to the "TryExec" string if "CodeName" is not specified.
    //for most debuggers those strings should be the same
    return m_config->group("General").readEntry("CodeName", tryExec());
}

QString Debugger::tryExec() const
{
    return m_config->group("General").readEntry("TryExec");
}

QStringList Debugger::supportedBackends() const
{
    return m_config->group("General").readEntry("Backends").split('|', QString::SkipEmptyParts);
}

void Debugger::setUsedBackend(const QString & backendName)
{
    if (supportedBackends().contains(backendName)) {
        m_backend = backendName;
    } else {
        kWarning() << "Invalid backend specified";
    }
}

QString Debugger::command() const
{
    if ( !m_config->hasGroup(m_backend) ) {
        kWarning() << "No backend has been set";
        return QString();
    } else {
        return m_config->group(m_backend).readPathEntry("Exec", QString());
    }
}

QString Debugger::backtraceBatchCommands() const
{
    if ( !m_config->hasGroup(m_backend) ) {
        kWarning() << "No backend has been set";
        return QString();
    } else {
        return m_config->group(m_backend).readEntry("BatchCommands");
    }
}

bool Debugger::runInTerminal() const
{
    if ( !m_config->hasGroup(m_backend) ) {
        kWarning() << "No backend has been set";
        return false;
    } else {
        return m_config->group(m_backend).readEntry("Terminal", false);
    }
}

//static
void Debugger::expandString(QString & str, ExpandStringUsage usage, const QString & tempFile)
{
    const CrashedApplication *appInfo = DrKonqi::crashedApplication();
    QHash<QString, QString> map;
    map[QLatin1String("progname")] = appInfo->name();
    map[QLatin1String("execname")] = appInfo->executable().fileName();
    map[QLatin1String("execpath")] = appInfo->executable().absoluteFilePath();
    map[QLatin1String("signum")] = QString::number(appInfo->signalNumber());
    map[QLatin1String("signame")] = appInfo->signalName();
    map[QLatin1String("pid")] = QString::number(appInfo->pid());
    map[QLatin1String("tempfile")] = tempFile;

    if (usage == ExpansionUsageShell) {
        str = KMacroExpander::expandMacrosShellQuote(str, map);
    } else {
        str = KMacroExpander::expandMacros(str, map);
    }
}

//static
QList<Debugger> Debugger::availableDebuggers(const char *regexp)
{
    KStandardDirs *dirs = KGlobal::dirs();
    QStringList debuggers = dirs->findAllResources("appdata", QLatin1String(regexp),
                                                   KStandardDirs::NoDuplicates);

    QList<Debugger> result;
    foreach (const QString & debuggerFile, debuggers) {
        Debugger debugger;
        debugger.m_config = KSharedConfig::openConfig(debuggerFile);
        QString tryExec = debugger.tryExec();
        if (!tryExec.isEmpty() && !KStandardDirs::findExe(tryExec).isEmpty()) {
            result.append(debugger);
        }
    }
    return result;
}
