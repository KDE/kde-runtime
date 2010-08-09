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

#include "crashedapplication.h"
#include <KToolInvocation>

#include <config-drkonqi.h>
#ifdef HAVE_STRSIGNAL
# include <clocale>
# include <cstring>
# include <cstdlib>
#else
# include <signal.h>
#endif

CrashedApplication::CrashedApplication(QObject *parent)
    : QObject(parent), m_restarted(false)
{
}

CrashedApplication::~CrashedApplication()
{
}

QString CrashedApplication::name() const
{
    return m_name;
}

QFileInfo CrashedApplication::executable() const
{
    return m_executable;
}

QString CrashedApplication::fakeExecutableBaseName() const
{
    if (!m_fakeBaseName.isEmpty()) {
        return m_fakeBaseName;
    } else {
        return m_executable.baseName();
    }
}

QString CrashedApplication::version() const
{
    return m_version;
}

BugReportAddress CrashedApplication::bugReportAddress() const
{
    return m_reportAddress;
}

int CrashedApplication::pid() const
{
    return m_pid;
}

int CrashedApplication::signalNumber() const
{
    return m_signalNumber;
}

QString CrashedApplication::signalName() const
{
#ifdef HAVE_STRSIGNAL
    const char * oldLocale = std::setlocale(LC_MESSAGES, NULL);
    char * savedLocale;
    if (oldLocale) {
        savedLocale = strdup(oldLocale);
    } else {
        savedLocale = NULL;
    }
    std::setlocale(LC_MESSAGES, "C");
    const char *name = strsignal(m_signalNumber);
    std::setlocale(LC_MESSAGES, savedLocale);
    std::free(savedLocale);
    return QString::fromLocal8Bit(name != NULL ? name : "Unknown");
#else
    switch (m_signalNumber) {
    case SIGILL: return QLatin1String("SIGILL");
    case SIGABRT: return QLatin1String("SIGABRT");
    case SIGFPE: return QLatin1String("SIGFPE");
    case SIGSEGV: return QLatin1String("SIGSEGV");
#ifdef SIGBUS
    case SIGBUS: return QLatin1String("SIGBUS");
#endif
    default: return QLatin1String("Unknown");
    }
#endif
}

bool CrashedApplication::hasBeenRestarted() const
{
    return m_restarted;
}

int CrashedApplication::thread() const
{
    return m_thread;
}

void CrashedApplication::restart()
{
    if (!m_restarted) {
        m_restarted = true;

        //start the application via kdeinit, as it needs to have a pristine environment and
        //KProcess::startDetached() can't start a new process with custom environment variables.
        if (!m_fakeBaseName.isEmpty()) {
            // if m_fakeBaseName is set, this means m_executable is the path to kdeinit4
            // so we need to use the fakeBaseName to restart the app
            KToolInvocation::kdeinitExec(m_fakeBaseName);
        } else {
            KToolInvocation::kdeinitExec(m_executable.absoluteFilePath());
        }
        emit restarted();
    }
}

#include "crashedapplication.moc"
