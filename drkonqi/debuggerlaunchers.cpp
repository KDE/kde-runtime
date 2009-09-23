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
#include "debuggerlaunchers.h"
#include <KShell>
#include <KDebug>

DefaultDebuggerLauncher::DefaultDebuggerLauncher(const Debugger & debugger, DebuggerManager *parent)
    : AbstractDebuggerLauncher(parent), m_debugger(debugger)
{
}

QString DefaultDebuggerLauncher::name() const
{
    return m_debugger.name();
}

void DefaultDebuggerLauncher::start()
{
    if ( qobject_cast<DebuggerManager*>(parent())->debuggerIsRunning() ) {
        kWarning() << "Another debugger is already running";
        return;
    }

    KProcess *process = new KProcess(this);
    QString str = m_debugger.command();
    Debugger::expandString(str, Debugger::ExpansionUsageShell);
    *process << KShell::splitArgs(str);

    emit starting();
    connect(process, SIGNAL(stateChanged(QProcess::ProcessState)),
            SLOT(processStateChanged(QProcess::ProcessState)));
    process->start();
}

void DefaultDebuggerLauncher::processStateChanged(QProcess::ProcessState newState)
{
    if (newState == QProcess::NotRunning) {
        emit finished();
        delete sender();
    }
}

TerminalDebuggerLauncher::TerminalDebuggerLauncher(const Debugger & debugger, DebuggerManager *parent)
    : DefaultDebuggerLauncher(debugger, parent)
{
}

void TerminalDebuggerLauncher::start()
{
    DefaultDebuggerLauncher::start(); //FIXME
}

#include "debuggerlaunchers.moc"
