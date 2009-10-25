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
    switch (m_signalnum) {
    case 4: return QString("SIGILL");
    case 6: return QString("SIGABRT");
    case 8: return QString("SIGFPE");
    case 11: return QString("SIGSEGV");
    default: return QString("Unknown");
    }
#endif
}

bool CrashedApplication::hasBeenRestarted() const
{
    return m_restarted;
}

void CrashedApplication::restart()
{
    if (!m_restarted) {
        m_restarted = true;

        //start the application via kdeinit, as it needs to have a pristine environment and
        //KProcess::startDetached() can't start a new process with custom environment variables.
        KToolInvocation::kdeinitExec(m_executable.absoluteFilePath());
        emit restarted();
    }
}

#include "crashedapplication.moc"
