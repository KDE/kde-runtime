/*******************************************************************
* reportinfo.cpp
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

#include "reportinfo.h"
#include "drkonqi.h"
#include "krashconf.h"
#include "bugzillalib.h"

#include <KProcess>
#include <KStandardDirs>
#include <KDebug>
#include <kdeversion.h>

#include <config-drkonqi.h>
#ifdef HAVE_UNAME
# include <sys/utsname.h>
#endif

ReportInfo::ReportInfo(QObject *parent)
    : QObject(parent)
{
    m_userCanDetail = false;
    m_developersCanContactReporter = false;

    //if lsb_release needs to run, start it asynchronously because it takes much time...
    QString lsb_release = KStandardDirs::findExe(QLatin1String("lsb_release"));
    if ( !lsb_release.isEmpty() ) {
        kDebug() << "found lsb_release";
        KProcess *process = new KProcess();
        process->setOutputChannelMode(KProcess::OnlyStdoutChannel);
        process->setEnv("LC_ALL", "C");
        *process << lsb_release << "-sd";
        connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(lsbReleaseFinished()));
        process->start();
    }
}

void ReportInfo::lsbReleaseFinished()
{
    KProcess *process = qobject_cast<KProcess*>(sender());
    Q_ASSERT(process);
    m_lsbRelease = QString::fromLocal8Bit(process->readAllStandardOutput().trimmed());
    process->deleteLater();
}

bool ReportInfo::userCanDetail() const
{
    return m_userCanDetail;
}

void ReportInfo::setUserCanDetail(bool canDetail)
{
    m_userCanDetail = canDetail;
}

bool ReportInfo::developersCanContactReporter() const
{
    return m_developersCanContactReporter;
}

void ReportInfo::setDevelopersCanContactReporter(bool canContact)
{
    m_developersCanContactReporter = canContact;
}

QString ReportInfo::reportKeywords() const
{
    return m_reportKeywords;
}

void ReportInfo::setReportKeywords(const QString & keywords)
{
    m_reportKeywords = keywords;
}

void ReportInfo::setBacktrace(const QString & backtrace)
{
    m_backtrace = backtrace;
}

QStringList ReportInfo::firstBacktraceFunctions() const
{
    return m_firstBacktraceFunctions;
}

void ReportInfo::setFirstBacktraceFunctions(const QStringList & functions)
{
    m_firstBacktraceFunctions = functions;
}

void ReportInfo::setDetailText(const QString & text)
{
    m_userDetailText = text;
}

void ReportInfo::setPossibleDuplicate(const QString & bug)
{
    m_possibleDuplicate = bug;
}

QString ReportInfo::osString() const
{
    //FIXME what if we don't have uname?
    QString result;
#ifdef HAVE_UNAME
    struct utsname buf;
    if (uname(&buf) == -1) {
        kDebug() << "call to uname failed" << perror;
        return QString();
    }
    result += QString::fromLocal8Bit(buf.sysname);
    result += " ";
    result += QString::fromLocal8Bit(buf.release);
    result += " ";
    result += QString::fromLocal8Bit(buf.machine);
#endif
    return result;
}

QString ReportInfo::generateReport() const
{
    //Note: no translations must be done in this function's strings
    QString dottedLine = QLatin1String("------");
    QString lineBreak = QLatin1String("\n");

    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();

    QString report;
    
    report.append(QString("Application and System information:"));
    report.append(lineBreak + dottedLine + lineBreak);
    //Program name and versions
    report.append(QString("KDE Version: %1").arg(KDE::versionString()) + lineBreak);
    report.append(QString("Qt Version: %1").arg(qVersion()) + lineBreak);
    report.append(QString("Operating System: %1").arg(osString()) + lineBreak);
    report.append(QString("Application that crashed: %1").arg(krashConfig->productName())
                    + lineBreak);
    report.append(QString("Version of the application: %1").arg(krashConfig->productVersion())
                    + lineBreak);
    
    //LSB output
    if ( !m_lsbRelease.isEmpty() ) {
        report.append(QString("Distribution: %1").arg(m_lsbRelease) + lineBreak);
    }
    report.append(dottedLine + lineBreak);

    //Description (title)
    if (!m_reportKeywords.isEmpty()) {
        report.append(lineBreak + QString("Title: %1").arg(m_reportKeywords));
    }
    
    //Details of the crash situation
    if (m_userCanDetail) {
        report.append(lineBreak + lineBreak);
        report.append(QString("What I was doing when the application crashed:"));
        report.append(lineBreak);
        if (!m_userDetailText.isEmpty()) {
            report.append(m_userDetailText);
        } else {
            report.append(i18nc("@info/plain","<placeholder>In detail, tell us what were you doing when "
                                "the application crashed.</placeholder>"));
        }
    }

    //Backtrace
    report.append(lineBreak + lineBreak);

    if (!m_backtrace.isEmpty()) {
        QString formattedBacktrace = m_backtrace.trimmed();
        report.append(QString("Backtrace:") + lineBreak + dottedLine + lineBreak
                        + formattedBacktrace + lineBreak +dottedLine);
    } else {
        report.append(QString("An useful backtrace could not be generated"));
    }

    //Possible duplicate
    if (!m_possibleDuplicate.isEmpty()) {
        report.append(lineBreak + lineBreak + QString("This bug may be a duplicate of or related "
                                                      "to bug %1").arg(m_possibleDuplicate));
    }

    return report;
}

//this function maps the operating system to an OS value that is accepted by bugs.kde.org.
//if the values change on the server side, they need to be updated here as well.
static inline QString bugzillaOs()
{
#if defined(Q_OS_LINUX)
    return QLatin1String("Linux");
#elif defined(Q_OS_FREEBSD)
    return QLatin1String("FreeBSD");
#elif defined(Q_OS_NETBSD)
    return QLatin1String("NetBSD");
#elif defined(Q_OS_OPENBSD)
    return QLatin1String("OpenBSD");
#elif defined(Q_OS_AIX)
    return QLatin1String("AIX");
#elif defined(Q_OS_HPUX)
    return QLatin1String("HP-UX");
#elif defined(Q_OS_IRIX)
    return QLatin1String("IRIX");
#elif defined(Q_OS_OSF)
    return QLatin1String("Tru64");
#elif defined(Q_OS_SOLARIS)
    return QLatin1String("Solaris");
#elif defined(Q_OS_CYGWIN)
    return QLatin1String("Cygwin");
#elif defined(Q_OS_DARWIN)
    return QLatin1String("OS X");
#elif defined(Q_OS_WIN32)
    return QLatin1String("MS Windows");
#else
    return QLatin1String("unspecified");
#endif
}

BugReport ReportInfo::newBugReportTemplate() const
{
    BugReport report;
    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();
    report.setProduct(krashConfig->productName());
    report.setComponent(QLatin1String("general"));
    report.setVersion(krashConfig->productVersion());
    report.setOperatingSystem(bugzillaOs());
    report.setPriority(QLatin1String("NOR"));
    report.setBugSeverity(QLatin1String("crash"));
    report.setShortDescription(m_reportKeywords);
    return report;
}

void ReportInfo::sendBugReport(BugzillaManager *bzManager) const
{
    BugReport report = newBugReportTemplate();
    report.setDescription(generateReport());
    report.setValid(true);
    connect(bzManager, SIGNAL(sendReportErrorWrongProduct()), this, SLOT(sendUsingDefaultProduct()));
    bzManager->sendReport(report);
}

void ReportInfo::sendUsingDefaultProduct() const
{
    BugzillaManager *bzManager = qobject_cast<BugzillaManager*>(sender());
    Q_ASSERT(bzManager);
    BugReport report = newBugReportTemplate();
    report.setProduct(QLatin1String("kde"));
    report.setComponent(QLatin1String("general"));
    report.setDescription(generateReport());
    report.setValid(true);
    bzManager->sendReport(report);
}

#include "reportinfo.moc"
