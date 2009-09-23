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
#include <QtCore/QDir>
#include <KCmdLineArgs>
#include <KStandardDirs>
#include <KDebug>

#include <config-drkonqi.h>
#ifdef HAVE_STRSIGNAL
# include <clocale>
# include <cstring>
# include <cstdlib>
#endif

struct CrashedApplication::Private
{
    int pid;
    int signalNumber;
    QString name;
    QFileInfo executable;
    QString version;
    BugReportAddress reportAddress;
};

CrashedApplication::~CrashedApplication()
{
    delete d;
}

QString CrashedApplication::name() const
{
    return d->name;
}

QFileInfo CrashedApplication::executable() const
{
    return d->executable;
}

QString CrashedApplication::version() const
{
    return d->version;
}

BugReportAddress CrashedApplication::bugReportAddress() const
{
    return d->reportAddress;
}

int CrashedApplication::pid() const
{
    return d->pid;
}

int CrashedApplication::signalNumber() const
{
    return d->signalNumber;
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
    const char *name = strsignal(d->signalNumber);
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

// private members

CrashedApplication::CrashedApplication(Private *dd)
    : d(dd)
{
}

//static
CrashedApplication *CrashedApplication::createFromKCrashData()
{
    CrashedApplication::Private *d = new Private;
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    d->name = args->getOption("programname");
    d->version = args->getOption("appversion").toUtf8();
    d->reportAddress = BugReportAddress(args->getOption("bugaddress").toUtf8());
    d->pid = args->getOption("pid").toInt();
    d->signalNumber = args->getOption("signal").toInt();

    //try to determine the executable that crashed
    if ( QFileInfo(QString("/proc/%1/exe").arg(d->pid)).exists() ) {
        //on linux, the fastest and most reliable way is to get the path from /proc
        kDebug() << "Using /proc to determine executable path";
        d->executable.setFile(QFile::symLinkTarget(QString("/proc/%1/exe").arg(d->pid)));
    } else {
        if ( args->isSet("kdeinit") ) {
            d->executable = QFileInfo(KStandardDirs::findExe("kdeinit4"));
        } else {
            QFileInfo execPath(args->getOption("appname"));
            if ( execPath.isAbsolute() ) {
                d->executable = execPath;
            } else if ( !args->getOption("apppath").isEmpty() ) {
                QDir execDir(args->getOption("apppath"));
                d->executable = execDir.absoluteFilePath(execPath.fileName());
            } else {
                d->executable = QFileInfo(KStandardDirs::findExe(execPath.fileName()));
            }
        }
    }

    kDebug() << "Executable is:" << d->executable.absoluteFilePath();
    kDebug() << "Executable exists:" << d->executable.exists();

    return new CrashedApplication(d);
}
