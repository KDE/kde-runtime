/*******************************************************************
* reportinterface.cpp
* Copyright 2009,2010    Dario Andres Rodriguez <andresbajotierra@gmail.com>
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

#include "reportinterface.h"

#include "drkonqi.h"
#include "bugzillalib.h"
#include "productmapping.h"
#include "systeminformation.h"
#include "crashedapplication.h"
#include "debuggermanager.h"
#include "parser/backtraceparser.h"
#include "backtracegenerator.h"
#include "applicationdetailsexamples.h"

#include <KDebug>
#include <KLocale>

ReportInterface::ReportInterface(QObject *parent)
    : QObject(parent)
{
    m_bugzillaManager = new BugzillaManager(this);

#if 0
    //Set a custom tracker for testing purposes
    //m_bugzillaManager->setCustomBugtrackerUrl("http://bugstest.kde.org/");
#endif

    m_productMapping = new ProductMapping(DrKonqi::crashedApplication()->fakeExecutableBaseName(),
                                          m_bugzillaManager, this);

    m_appDetailsExamples = new ApplicationDetailsExamples(this);

    //Information the user can provide about the crash
    m_userRememberCrashSituation = false;
    m_reproducible = ReproducibleUnsure;
    m_provideActionsApplicationDesktop = false;
    m_provideUnusualBehavior = false;
    m_provideApplicationConfigurationDetails = false;

    //Do not attach the bug report to any other existent report (create a new one)
    m_attachToBugNumber = 0;
}

void ReportInterface::setBugAwarenessPageData(bool rememberSituation,
                                                   Reproducible reproducible, bool actions, 
                                                   bool unusual, bool configuration)
{
    //Save the information the user can provide about the crash from the assistant page
    m_userRememberCrashSituation = rememberSituation;
    m_reproducible = reproducible;
    m_provideActionsApplicationDesktop = actions;
    m_provideUnusualBehavior = unusual;
    m_provideApplicationConfigurationDetails = configuration;
}

bool ReportInterface::isBugAwarenessPageDataUseful() const
{
    //Determine if the assistant should proceed, considering the amount of information
    //the user can provide
    int rating = selectedOptionsRating();

    //Minimum information required even for a good backtrace.
    bool useful = m_userRememberCrashSituation &&
                  (rating >= 2 || (m_reproducible==ReproducibleSometimes ||
                                 m_reproducible==ReproducibleEverytime));
    return useful;
}

int ReportInterface::selectedOptionsRating() const
{
    //Check how many information the user can provide and generate a rating
    int rating = 0;
    if (m_provideActionsApplicationDesktop) {
        rating += 3;
    }
    if (m_provideApplicationConfigurationDetails) {
        rating += 2;
    }
    if (m_provideUnusualBehavior) {
        rating += 1;
    }
    return rating;
}

QString ReportInterface::backtrace() const
{
    return m_backtrace;
}

void ReportInterface::setBacktrace(const QString & backtrace)
{
    m_backtrace = backtrace;
}

QStringList ReportInterface::firstBacktraceFunctions() const
{
    return m_firstBacktraceFunctions;
}

void ReportInterface::setFirstBacktraceFunctions(const QStringList & functions)
{
    m_firstBacktraceFunctions = functions;
}

QString ReportInterface::title() const
{
    return m_reportTitle;
}
    
void ReportInterface::setTitle(const QString & text)
{
    m_reportTitle = text;
}

void ReportInterface::setDetailText(const QString & text)
{
    m_reportDetailText = text;
}

void ReportInterface::setPossibleDuplicates(const QStringList & list)
{
    m_possibleDuplicates = list;
}

QString ReportInterface::generateReport(bool drKonqiStamp) const
{
    //Note: no translations must be done in this function's strings
    const CrashedApplication * crashedApp = DrKonqi::crashedApplication();
    const SystemInformation * sysInfo = DrKonqi::systemInformation();
    
    QString report;

    //Program name and versions
    report.append(QString("Application: %1 (%2)\n").arg(crashedApp->fakeExecutableBaseName(),
                                                        crashedApp->version()));
    report.append(QString("KDE Platform Version: %1").arg(sysInfo->kdeVersion()));
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
    if (isBugAwarenessPageDataUseful()) {
        report.append(QString("-- Information about the crash:\n"));
        if (!m_reportDetailText.isEmpty()) {
            report.append(m_reportDetailText.trimmed());
        } else {
            //If the user manual reports this crash, he/she should know what to put in here.
            //This message is the only one translated in this function
            report.append(i18nc("@info/plain","<placeholder>In detail, tell us what you were doing "
                                              " when the application crashed.</placeholder>"));
        }
        report.append(QLatin1String("\n\n"));
    }

    //Crash reproducibility (only if useful)
    if (m_reproducible !=  ReproducibleUnsure) {
        if (m_reproducible == ReproducibleEverytime) {
            report.append(QString("The crash can be reproduced every time.\n\n"));
        } else if (m_reproducible == ReproducibleSometimes) {
            report.append(QString("The crash can be reproduced some of the time.\n\n"));
        } else if (m_reproducible == ReproducibleNever) {
            report.append(QString("The crash does not seem to be reproducible.\n\n"));
        }
    }

    //Backtrace
    report.append(QString("-- Backtrace:\n"));
    if (!m_backtrace.isEmpty()) {
        QString formattedBacktrace = m_backtrace.trimmed();
        report.append(formattedBacktrace + QLatin1String("\n"));
    } else {
        report.append(QString("A useful backtrace could not be generated\n"));
    }

    //Possible duplicates (selected by the user)
    if (!m_possibleDuplicates.isEmpty()) {
        report.append(QLatin1String("\n"));
        QString duplicatesString;
        Q_FOREACH(const QString & dupe, m_possibleDuplicates) {
            duplicatesString += QLatin1String("bug ") + dupe + QLatin1String(", ");
        }
        duplicatesString = duplicatesString.left(duplicatesString.length()-2) + '.';
        report.append(QString("This bug may be a duplicate of or related to %1\n")
                        .arg(duplicatesString));
    }

    //Several possible duplicates (by bugzilla query)
    if (!m_allPossibleDuplicatesByQuery.isEmpty()) {
        report.append(QLatin1String("\n"));
        QString duplicatesString;
        int count = m_allPossibleDuplicatesByQuery.count();
        for(int i=0; i < count && i < 5; i++) {
            duplicatesString += QLatin1String("bug ") + m_allPossibleDuplicatesByQuery.at(i) +
                                QLatin1String(", ");
        }
        duplicatesString = duplicatesString.left(duplicatesString.length()-2) + '.';
        report.append(QString("Possible duplicates by query: %1\n").arg(duplicatesString));
    }

    if (drKonqiStamp) {
        report.append(QLatin1String("\nReported using DrKonqi"));
    }
    
    return report;
}

QString ReportInterface::generateReportForAttachmentDescription() const
{
    QString description;

    //Note: no translations must be done in this function's strings
    const CrashedApplication * crashedApp = DrKonqi::crashedApplication();
    const SystemInformation * sysInfo = DrKonqi::systemInformation();

    //Program name and versions
    description.append(QString("%1 (%2) on KDE Platform %3 using Qt %4\n\n")
                       .arg(crashedApp->fakeExecutableBaseName())
                       .arg(crashedApp->version())
                       .arg(sysInfo->kdeVersion())
                       .arg(sysInfo->qtVersion()));

    //Details of the crash situation
    if (isBugAwarenessPageDataUseful()) {
        description.append(QString("%1\n\n").arg(m_reportDetailText.trimmed()));
    }

    //Backtrace (only 6 lines)
    description.append(QString("-- Backtrace (Reduced):\n"));
    QString reducedBacktrace =
                DrKonqi::debuggerManager()->backtraceGenerator()->parser()->simplifiedBacktrace();
    description.append(reducedBacktrace.trimmed());

    return description;
}

BugReport ReportInterface::newBugReportTemplate() const
{
    //Generate a new bug report template with some values on it
    BugReport report;
    
    const SystemInformation * sysInfo = DrKonqi::systemInformation();
    
    report.setProduct(m_productMapping->bugzillaProduct());
    report.setComponent(m_productMapping->bugzillaComponent());
    report.setVersion(m_productMapping->bugzillaVersion());
    report.setOperatingSystem(sysInfo->bugzillaOperatingSystem());
    if (sysInfo->compiledSources()) {
        report.setPlatform(QLatin1String("Compiled Sources"));
    } else {
        report.setPlatform(sysInfo->bugzillaPlatform());
    }
    report.setPriority(QLatin1String("NOR"));
    report.setBugSeverity(QLatin1String("crash"));

    /*
    Disable the backtrace functions on title for RELEASE.
    It also needs a bit of polishment

    QString title = m_reportTitle;

    //If there are not too much possible duplicates by query then there are more possibilities
    //that this report is unique. Let's add the backtrace functions to the title
    if (m_allPossibleDuplicatesByQuery.count() <= 2) {
        if (!m_firstBacktraceFunctions.isEmpty()) {
            title += (QLatin1String(" [") + m_firstBacktraceFunctions.join(", ").trimmed()
                                                                            + QLatin1String("]"));
        }
    }
    */

    report.setShortDescription(m_reportTitle);
    return report;
}

void ReportInterface::sendBugReport() const
{
    if (m_attachToBugNumber > 0)
    {
        //We are going to attach the report to an existent one
        connect(m_bugzillaManager, SIGNAL(addMeToCCFinished(int)), this, SLOT(addedToCC()));
        connect(m_bugzillaManager, SIGNAL(addMeToCCError(QString)), this, SIGNAL(sendReportError(QString)));
        //First add the user to the CC list, then attach
        m_bugzillaManager->addMeToCC(m_attachToBugNumber);
    } else {
        //Creating a new bug report
        BugReport report = newBugReportTemplate();
        report.setDescription(generateReport(true));
        report.setValid(true);

        connect(m_bugzillaManager, SIGNAL(sendReportErrorInvalidValues()), this, SLOT(sendUsingDefaultProduct()));
        connect(m_bugzillaManager, SIGNAL(reportSent(int)), this, SIGNAL(reportSent(int)));
        connect(m_bugzillaManager, SIGNAL(sendReportError(QString)), this, SIGNAL(sendReportError(QString)));
        m_bugzillaManager->sendReport(report);
    }
}

void ReportInterface::sendUsingDefaultProduct() const
{
    //Fallback function: if some of the custom values fail, we need to reset all the fields to the default
    //(and valid) bugzilla values; and try to resend
    BugReport report = newBugReportTemplate();
    report.setProduct(QLatin1String("kde"));
    report.setComponent(QLatin1String("general"));
    report.setPlatform(QLatin1String("unspecified"));
    report.setDescription(generateReport(true));
    report.setValid(true);
    m_bugzillaManager->sendReport(report);
}

void ReportInterface::addedToCC()
{
    //The user was added to the CC list, proceed with the attachment
    connect(m_bugzillaManager, SIGNAL(attachToReportSent(int, int)), this, SLOT(attachSent(int, int)));
    connect(m_bugzillaManager, SIGNAL(attachToReportError(QString)), this,
                                                            SIGNAL(sendReportError(QString)));
    BugReport report = newBugReportTemplate();

    QString reportText = generateReport(true);
    QString description = generateReportForAttachmentDescription();

    //Attach the report. The description of the attachment also includes the bug description
    m_bugzillaManager->attachTextToReport(reportText, QLatin1String("/tmp/drkonqireport"), //Fake path
                                  QLatin1String("New crash information added by DrKonqi"),
                                  m_attachToBugNumber,
                                  description);
}

void ReportInterface::attachSent(int attachId, int bugId)
{
    Q_UNUSED(attachId);
    //The bug was attached, consider it "sent"
    emit reportSent(bugId);
}

QStringList ReportInterface::relatedBugzillaProducts() const
{
    return m_productMapping->relatedBugzillaProducts();
}

bool ReportInterface::isWorthReporting() const
{
    //Evaluate if the provided information is useful enough to enable the automatic report
    bool needToReport = false;

    if (!m_userRememberCrashSituation) {
        //This should never happen... but...
        return false;
    }

    int rating = selectedOptionsRating();

    BacktraceParser::Usefulness use =
                DrKonqi::debuggerManager()->backtraceGenerator()->parser()->backtraceUsefulness();
   
    switch (use) {
    case BacktraceParser::ReallyUseful: {
        //Perfect backtrace: require at least one option or a 100%-50% reproducible crash
        needToReport = (rating >=2) ||
            (m_reproducible == ReproducibleEverytime || m_reproducible == ReproducibleSometimes);
        break;
    }
    case BacktraceParser::MayBeUseful: {
        //Not perfect backtrace: require at least two options or a 100% reproducible crash
        needToReport = (rating >=3) || (m_reproducible == ReproducibleEverytime);
        break;
    }
    case BacktraceParser::ProbablyUseless:
        //Bad backtrace: require at least two options and always reproducible (strict)
        needToReport = (rating >=5) && (m_reproducible == ReproducibleEverytime);
        break;
    case BacktraceParser::Useless:
    case BacktraceParser::InvalidUsefulness: {
        needToReport =  false;
    }
    }
    
    return needToReport;
}

void ReportInterface::setAttachToBugNumber(uint bugNumber)
{
    //If bugNumber>0, the report is going to be attached to bugNumber
    m_attachToBugNumber = bugNumber;
}

uint ReportInterface::attachToBugNumber() const
{
    return m_attachToBugNumber;
}

void ReportInterface::setPossibleDuplicatesByQuery(const QStringList & list)
{
    m_allPossibleDuplicatesByQuery = list;
}

BugzillaManager * ReportInterface::bugzillaManager() const
{
    return m_bugzillaManager;
}

ApplicationDetailsExamples * ReportInterface::appDetailsExamples() const
{
    return m_appDetailsExamples;
}

#include "reportinterface.moc"
