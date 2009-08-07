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
#include "productmapping.h"

#include "backtraceparser.h"
#include "backtracegenerator.h"

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
    
    m_compiledSources = false;

    m_productMapping = new ProductMapping(DrKonqi::instance()->krashConfig()->productName(), this);
    
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
    } else {
        m_bugzillaPlatform = QLatin1String("unspecified");
    }
}

void ReportInfo::lsbReleaseFinished()
{
    KProcess *process = qobject_cast<KProcess*>(sender());
    Q_ASSERT(process);
    m_lsbRelease = QString::fromLocal8Bit(process->readAllStandardOutput().trimmed());
    process->deleteLater();
    
    //Guess distro string
    if ( m_lsbRelease.contains("suse",Qt::CaseInsensitive) ) {
        m_bugzillaPlatform = QLatin1String("SuSE RPMs");
    } else if ( m_lsbRelease.contains("ubuntu",Qt::CaseInsensitive) ) {
        m_bugzillaPlatform = QLatin1String("Ubuntu Packages");
    } else if ( m_lsbRelease.contains("ubuntu",Qt::CaseInsensitive) ) {
        m_bugzillaPlatform = QLatin1String("Ubuntu Packages");
    } else if ( m_lsbRelease.contains("fedora",Qt::CaseInsensitive) ) {
        m_bugzillaPlatform = QLatin1String("Fedora RPMs");
    } else if ( m_lsbRelease.contains("redhat",Qt::CaseInsensitive) ) {
        m_bugzillaPlatform = QLatin1String("RedHat RPMs");
    } else if ( m_lsbRelease.contains("mandriva",Qt::CaseInsensitive) ) {
        m_bugzillaPlatform = QLatin1String("Mandriva RPMs");
    } else if ( m_lsbRelease.contains("slack",Qt::CaseInsensitive) ) {
        m_bugzillaPlatform = QLatin1String("Slackware Packages");
    } else if ( m_lsbRelease.contains("pardus",Qt::CaseInsensitive) ) {
        m_bugzillaPlatform = QLatin1String("Pardus Packages");
    } else if ( m_lsbRelease.contains("freebsd",Qt::CaseInsensitive) ) {
        m_bugzillaPlatform = QLatin1String("FreeBSD Ports");
    } else if ( m_lsbRelease.contains("netbsd",Qt::CaseInsensitive) ) {
        m_bugzillaPlatform = QLatin1String("NetBSD pkgsrc");
    } else if ( m_lsbRelease.contains("openbsd",Qt::CaseInsensitive) ) {
        m_bugzillaPlatform = QLatin1String("OpenBSD Packages");
    } else if ( m_lsbRelease.contains("solaris",Qt::CaseInsensitive) ) {
        m_bugzillaPlatform = QLatin1String("Solaris Packages");
    } else if ( m_lsbRelease.contains("arch",Qt::CaseInsensitive) ) {
        m_bugzillaPlatform = QLatin1String("Archlinux Packages");
    } else if ( m_lsbRelease.contains("debian",Qt::CaseInsensitive) ) {
        if ( m_lsbRelease.contains("stable",Qt::CaseInsensitive) ) {
            m_bugzillaPlatform = QLatin1String("Debian stable");
        } else if ( m_lsbRelease.contains("testing",Qt::CaseInsensitive) ) {
            m_bugzillaPlatform = QLatin1String("Debian testing");
        } else {
            m_bugzillaPlatform = QLatin1String("Debian unstable");
        }
    } else {
        m_bugzillaPlatform = QLatin1String("unspecified");
    }
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

QString ReportInfo::backtrace() const
{
    return m_backtrace;
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

QString ReportInfo::title() const
{
    return m_reportTitle;
}
    
void ReportInfo::setTitle(const QString & text)
{
    m_reportTitle = text;
}

void ReportInfo::setDetailText(const QString & text)
{
    m_reportDetailText = text;
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
    result += ' ';
    result += QString::fromLocal8Bit(buf.release);
    result += ' ';
    result += QString::fromLocal8Bit(buf.machine);
#endif
    return result;
}

QString ReportInfo::generateReport(bool drKonqiStamp) const
{
    //Note: no translations must be done in this function's strings
    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();

    QString report;

    //Program name and versions
    report.append(QString("Application that crashed: %1\n").arg(krashConfig->productName()));
    report.append(QString("Version of the application: %1\n").arg(krashConfig->productVersion()));
    report.append(QString("KDE Version: %1\n").arg(KDE::versionString()));
    report.append(QString("Qt Version: %1\n").arg(qVersion()));
    report.append(QString("Operating System: %1\n").arg(osString()));

    //LSB output or manually selected distro
    if ( !m_lsbRelease.isEmpty() ) {
        report.append(QString("Distribution: %1\n").arg(m_lsbRelease));
    } else if ( !m_bugzillaPlatform.isEmpty() && 
                                m_bugzillaPlatform != QLatin1String("unspecified")) {
        report.append(QString("Distribution (Platform): %1\n").arg(m_bugzillaPlatform));
    }
    
    //Compiled from sources
    if ( m_compiledSources ) {
        report.append(QString("KDE compiled from sources\n"));
    }

    report.append(QLatin1String("\n"));
    
    //Details of the crash situation
    if (m_userCanDetail) {
        report.append(QString("What I was doing when the application crashed:\n"));
        if (!m_reportDetailText.isEmpty()) {
            report.append(m_reportDetailText);
        } else {
            report.append(i18nc("@info/plain","<placeholder>In detail, tell us what you were doing "
                                              " when the application crashed.</placeholder>"));
        }
        report.append(QLatin1String("\n\n"));
    }

    //Backtrace
    report.append(QString(" -- Backtrace:\n"));
    if (!m_backtrace.isEmpty()) {
        QString formattedBacktrace = m_backtrace.trimmed();
        report.append(formattedBacktrace + QLatin1String("\n"));
    } else {
        report.append(QString("An useful backtrace could not be generated\n"));
    }

    //Possible duplicate
    if (!m_possibleDuplicate.isEmpty()) {
        report.append(QLatin1String("\n"));
        report.append(QString("This bug may be a duplicate of or related to bug %1\n")
                        .arg(m_possibleDuplicate));
    }

    if (drKonqiStamp) {
        report.append(QLatin1String("\nReported using DrKonqi"));
    }
    
    return report;
}

//this function maps the operating system to an OS value that is accepted by bugs.kde.org.
//if the values change on the server side, they need to be updated here as well.
static inline QString bugzillaOs()
{
//krazy:excludeall=cpp
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
    report.setProduct(m_productMapping->bugzillaProduct());
    report.setComponent(m_productMapping->bugzillaComponent());
    report.setVersion(krashConfig->productVersion());
    report.setOperatingSystem(bugzillaOs());
    if (m_compiledSources) {
        report.setPlatform(QLatin1String("Compiled Sources"));
    } else {
        report.setPlatform(m_bugzillaPlatform);
    }
    report.setPriority(QLatin1String("NOR"));
    report.setBugSeverity(QLatin1String("crash"));
    report.setShortDescription(m_reportTitle);
    return report;
}

void ReportInfo::sendBugReport(BugzillaManager *bzManager) const
{
    BugReport report = newBugReportTemplate();
    report.setDescription(generateReport(true));
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
    report.setDescription(generateReport(true));
    report.setValid(true);
    bzManager->sendReport(report);
}

QStringList ReportInfo::relatedBugzillaProducts() const
{
    return m_productMapping->relatedBugzillaProducts();
}

bool ReportInfo::isWorthReporting() const
{
    bool needToReport = false;
    
    BacktraceParser::Usefulness use =
                DrKonqi::instance()->backtraceGenerator()->parser()->backtraceUsefulness();
   
    switch (use) {
    case BacktraceParser::ReallyUseful: {
        needToReport = (m_userCanDetail || m_developersCanContactReporter);
        break;
    }
    case BacktraceParser::MayBeUseful: {
        needToReport = (m_userCanDetail || m_developersCanContactReporter);
        break;
    }
    case BacktraceParser::ProbablyUseless: {
        needToReport = (m_userCanDetail && m_developersCanContactReporter);
        break;
    }
    case BacktraceParser::Useless:
    case BacktraceParser::InvalidUsefulness: {
        needToReport =  false;
    }
    }
    
    return needToReport;
}

QString ReportInfo::bugzillaPlatform() const
{
    return m_bugzillaPlatform;
}

void ReportInfo::setBugzillaPlatform(const QString & platform)
{
    m_bugzillaPlatform = platform;
}

void ReportInfo::setCompiledSources(bool compiled)
{
    m_compiledSources = compiled;
}

#include "reportinfo.moc"
