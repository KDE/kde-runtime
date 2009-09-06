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
#include "systeminformation.h"

#include "backtraceparser.h"
#include "backtracegenerator.h"

#include <KProcess>
#include <KStandardDirs>
#include <KDebug>


ReportInfo::ReportInfo(QObject *parent)
    : QObject(parent)
{
    m_userCanDetail = false;
    m_developersCanContactReporter = false;
    
    m_productMapping = new ProductMapping(DrKonqi::instance()->krashConfig()->productName(), this);
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

void ReportInfo::setPossibleDuplicates(const QStringList & list)
{
    m_possibleDuplicates = list;
}

QString ReportInfo::generateReport(bool drKonqiStamp) const
{
    //Note: no translations must be done in this function's strings
    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();
    const SystemInformation * sysInfo = DrKonqi::instance()->systemInformation();
    
    QString report;

    //Program name and versions
    report.append(QString("Application: %1 (%2)\n").arg(krashConfig->productName(),
                                                                    krashConfig->productVersion()));
    report.append(QString("KDE Version: %1").arg(sysInfo->kdeVersion()));
    if ( sysInfo->compiledSources() ) {
        report.append(QString(" (Compiled from sources)\n"));
    } else {
        report.append(QString("\n"));
    }
    report.append(QString("Qt Version: %1\n").arg(sysInfo->qtVersion()));
    report.append(QString("Operating System: %1\n").arg(sysInfo->operatingSystem()));

    //LSB output or manually selected distro
    if ( !sysInfo->lsbRelease().isEmpty() ) {
        report.append(QString("Distribution: %1\n").arg(sysInfo->lsbRelease()));
    } else if ( !sysInfo->bugzillaPlatform().isEmpty() && 
                        sysInfo->bugzillaPlatform() != QLatin1String("unspecified")) {
        report.append(QString("Distribution (Platform): %1\n").arg(
                                                        sysInfo->bugzillaPlatform()));
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
    if (!m_possibleDuplicates.isEmpty()) {
        report.append(QLatin1String("\n"));
        QString duplicatesString;
        Q_FOREACH(const QString & dupe, m_possibleDuplicates) {
            duplicatesString += QLatin1String("bug ") + dupe + QLatin1String(", ");
        }
        duplicatesString = duplicatesString.left(duplicatesString.length()-2) + ".";
        report.append(QString("This bug may be a duplicate of or related to %1\n")
                        .arg(duplicatesString));
    }

    if (drKonqiStamp) {
        report.append(QLatin1String("\nReported using DrKonqi"));
    }
    
    return report;
}

BugReport ReportInfo::newBugReportTemplate() const
{
    BugReport report;
    
    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();
    const SystemInformation * sysInfo = DrKonqi::instance()->systemInformation();
    
    report.setProduct(m_productMapping->bugzillaProduct());
    report.setComponent(m_productMapping->bugzillaComponent());
    report.setVersion(krashConfig->productVersion());
    report.setOperatingSystem(sysInfo->bugzillaOperatingSystem());
    if (sysInfo->compiledSources()) {
        report.setPlatform(QLatin1String("Compiled Sources"));
    } else {
        report.setPlatform(sysInfo->bugzillaPlatform());
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
    connect(bzManager, SIGNAL(sendReportErrorInvalidValues()), this, SLOT(sendUsingDefaultProduct()));
    bzManager->sendReport(report);
}

void ReportInfo::sendUsingDefaultProduct() const
{
    BugzillaManager *bzManager = qobject_cast<BugzillaManager*>(sender());
    Q_ASSERT(bzManager);
    BugReport report = newBugReportTemplate();
    report.setProduct(QLatin1String("kde"));
    report.setComponent(QLatin1String("general"));
    report.setPlatform(QLatin1String("unspecified"));
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
        needToReport = (m_userCanDetail && m_developersCanContactReporter);
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

#include "reportinfo.moc"
