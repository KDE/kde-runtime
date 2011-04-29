/*******************************************************************
* systeminformation.cpp
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
* Copyright 2009    George Kiagiadakis <gkiagia@users.sourceforge.net>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************/

#include "systeminformation.h"

#include <KStandardDirs>
#include <KProcess>
#include <KDebug>
#include <KConfig>
#include <KConfigGroup>
#include <kdeversion.h>

#include <config-drkonqi.h>
#ifdef HAVE_UNAME
# include <sys/utsname.h>
#endif

SystemInformation::SystemInformation(QObject * parent)
    : QObject(parent)
{
    fetchOSInformation();
    runLsbRelease();
    
    KConfigGroup config(KGlobal::config(), "SystemInformation");
    m_compiledSources = config.readEntry("CompiledSources", false);
}

void SystemInformation::runLsbRelease()
{
    //Run lsbrelease async
    QString lsb_release = KStandardDirs::findExe(QLatin1String("lsb_release"));
    if ( !lsb_release.isEmpty() ) {
        kDebug() << "found lsb_release";
        KProcess *process = new KProcess();
        process->setOutputChannelMode(KProcess::OnlyStdoutChannel);
        process->setEnv("LC_ALL", "C");
        *process << lsb_release << "-sd";
        connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(lsbReleaseFinished()));
        process->start();
    } else {
        setBugzillaPlatform(QLatin1String("unspecified"));
    }
}

void SystemInformation::lsbReleaseFinished()
{
    KProcess *process = qobject_cast<KProcess*>(sender());
    Q_ASSERT(process);
    m_lsbRelease = QString::fromLocal8Bit(process->readAllStandardOutput().trimmed());
    process->deleteLater();
    
    //Guess distro string
    //TODO cache this value on KConfig ?
    if ( m_lsbRelease.contains("suse",Qt::CaseInsensitive) ) {
        setBugzillaPlatform(QLatin1String("openSUSE RPMs"));
    } else if ( m_lsbRelease.contains("ubuntu",Qt::CaseInsensitive) ) {
        setBugzillaPlatform(QLatin1String("Ubuntu Packages"));
    } else if ( m_lsbRelease.contains("fedora",Qt::CaseInsensitive) ) {
        setBugzillaPlatform(QLatin1String("Fedora RPMs"));
    } else if ( m_lsbRelease.contains("redhat",Qt::CaseInsensitive) ) {
        setBugzillaPlatform(QLatin1String("RedHat RPMs"));
    } else if ( m_lsbRelease.contains("mandriva",Qt::CaseInsensitive) ) {
        setBugzillaPlatform(QLatin1String("Mandriva RPMs"));
    } else if ( m_lsbRelease.contains("mageia",Qt::CaseInsensitive) ) {
        setBugzillaPlatform(QLatin1String("Mageia RPMs"));
    } else if ( m_lsbRelease.contains("slack",Qt::CaseInsensitive) ) {
        setBugzillaPlatform(QLatin1String("Slackware Packages"));
    } else if ( m_lsbRelease.contains("pardus",Qt::CaseInsensitive) ) {
        setBugzillaPlatform(QLatin1String("Pardus Packages"));
    } else if ( m_lsbRelease.contains("freebsd",Qt::CaseInsensitive) ) {
        setBugzillaPlatform(QLatin1String("FreeBSD Ports"));
    } else if ( m_lsbRelease.contains("netbsd",Qt::CaseInsensitive) ) {
        setBugzillaPlatform(QLatin1String("NetBSD pkgsrc"));
    } else if ( m_lsbRelease.contains("openbsd",Qt::CaseInsensitive) ) {
        setBugzillaPlatform(QLatin1String("OpenBSD Packages"));
    } else if ( m_lsbRelease.contains("solaris",Qt::CaseInsensitive) ) {
        setBugzillaPlatform(QLatin1String("Solaris Packages"));
    } else if ( m_lsbRelease.contains("arch",Qt::CaseInsensitive) ) {
        setBugzillaPlatform(QLatin1String("Archlinux Packages"));
    } else if ( m_lsbRelease.contains("debian",Qt::CaseInsensitive) ) {
        if ( m_lsbRelease.contains("unstable",Qt::CaseInsensitive) ) {
            setBugzillaPlatform(QLatin1String("Debian unstable"));
        } else if ( m_lsbRelease.contains("testing",Qt::CaseInsensitive) ) {
            setBugzillaPlatform(QLatin1String("Debian testing"));
        } else {
            setBugzillaPlatform(QLatin1String("Debian stable"));
        }
    } else {
        setBugzillaPlatform(QLatin1String("unspecified"));
    }
}

//this function maps the operating system to an OS value that is accepted by bugs.kde.org.
//if the values change on the server side, they need to be updated here as well.
void SystemInformation::fetchOSInformation()
{
    //krazy:excludeall=cpp
    //Get the base OS string (bugzillaOS)
#if defined(Q_OS_LINUX)
    m_bugzillaOperatingSystem = QLatin1String("Linux");
#elif defined(Q_OS_FREEBSD)
    m_bugzillaOperatingSystem = QLatin1String("FreeBSD");
#elif defined(Q_OS_NETBSD)
    m_bugzillaOperatingSystem = QLatin1String("NetBSD");
#elif defined(Q_OS_OPENBSD)
    m_bugzillaOperatingSystem = QLatin1String("OpenBSD");
#elif defined(Q_OS_AIX)
    m_bugzillaOperatingSystem = QLatin1String("AIX");
#elif defined(Q_OS_HPUX)
    m_bugzillaOperatingSystem = QLatin1String("HP-UX");
#elif defined(Q_OS_IRIX)
    m_bugzillaOperatingSystem = QLatin1String("IRIX");
#elif defined(Q_OS_OSF)
    m_bugzillaOperatingSystem = QLatin1String("Tru64");
#elif defined(Q_OS_SOLARIS)
    m_bugzillaOperatingSystem = QLatin1String("Solaris");
#elif defined(Q_OS_CYGWIN)
    m_bugzillaOperatingSystem = QLatin1String("Cygwin");
#elif defined(Q_OS_DARWIN)
    m_bugzillaOperatingSystem = QLatin1String("OS X");
#elif defined(Q_OS_WIN32)
    m_bugzillaOperatingSystem = QLatin1String("MS Windows");
#else
    m_bugzillaOperatingSystem = QLatin1String("unspecified");
#endif
    
    //Get complete OS string (and fallback to base string)
#ifdef HAVE_UNAME
    QString os;
    struct utsname buf;
    if (uname(&buf) == -1) {
        kDebug() << "call to uname failed" << perror;
        m_operatingSystem = m_bugzillaOperatingSystem;
    } else {
        m_operatingSystem = QString::fromLocal8Bit(buf.sysname) + ' ' 
            + QString::fromLocal8Bit(buf.release) + ' ' 
            + QString::fromLocal8Bit(buf.machine);
    }
#else    
    m_operatingSystem = m_bugzillaOperatingSystem;
#endif
}

QString SystemInformation::operatingSystem() const
{
    return m_operatingSystem;
}

QString SystemInformation::bugzillaOperatingSystem() const
{
    return m_bugzillaOperatingSystem;
}

QString SystemInformation::bugzillaPlatform() const
{
    return m_bugzillaPlatform;
}

void SystemInformation::setBugzillaPlatform(const QString & platform)
{
    m_bugzillaPlatform = platform;
}

QString SystemInformation::lsbRelease() const
{
    return m_lsbRelease;
}

bool SystemInformation::compiledSources() const
{
    return m_compiledSources;
}

void SystemInformation::setCompiledSources(bool compiled)
{
    if (compiled != m_compiledSources) {
        KConfigGroup config(KGlobal::config(), "SystemInformation");
        config.writeEntry("CompiledSources", compiled);    
    }
    m_compiledSources = compiled;    
}

QString SystemInformation::kdeVersion() const
{
    return KDE::versionString();
}

QString SystemInformation::qtVersion() const
{
    return qVersion();
}
