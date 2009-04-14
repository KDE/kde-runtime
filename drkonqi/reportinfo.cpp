/*******************************************************************
* reportinfo.cpp
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
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
#include "backtracegenerator.h"
#include "backtraceparser.h"
#include "bugzillalib.h"

#include <KProcess>
#include <kdeversion.h>

ReportInfo::ReportInfo()
{
    m_userCanDetail = false;
    m_developersCanContactReporter = false;
    m_bugzilla = new BugzillaManager();
}

ReportInfo::~ReportInfo()
{
    delete m_bugzilla;
}

QString ReportInfo::getKDEVersion() const
{
    return KDE::versionString();
}
QString ReportInfo::getQtVersion() const
{
    return qVersion();
}

QString ReportInfo::getOS() const
{
    if (m_OS.isEmpty()) { //Fetch OS name & ver
        KProcess process;
        process.setOutputChannelMode(KProcess::OnlyStdoutChannel);
        process.setEnv("LC_ALL", "C");
        process << "uname" <<  "-srom";
        process.execute(5000);
        QByteArray os = process.readAllStandardOutput();
        os.chop(1);
        m_OS = QString::fromLocal8Bit(os);
    }
    return m_OS;
}

QString ReportInfo::getLSBRelease() const
{
    if (m_LSBRelease.isEmpty()) {
        //Get base OS
        KProcess process;
        process.setOutputChannelMode(KProcess::OnlyStdoutChannel);
        process.setEnv("LC_ALL", "C");
        process << "uname" << "-s";
        process.execute(5000);
        QByteArray os = process.readAllStandardOutput();
        os.chop(1);

        if (QString::fromLocal8Bit(os) == QLatin1String("Linux")) {
            process.clearProgram();
            process << "lsb_release" << "-idrc";
            process.execute(5000);
            QByteArray lsb = process.readAllStandardOutput();
            if (!lsb.isEmpty()) {
                lsb.chop(1);
                m_LSBRelease = QString::fromLocal8Bit(lsb);
            } else {
                m_LSBRelease = i18nc("@info","LSB Release information not found "
                                    "( no lsb_release command found )");
            }
        } else {
            m_LSBRelease = i18nc("@info","Not a GNU/Linux system. LSB Release Information not available");
        }
    }
    return m_LSBRelease;
}

QString ReportInfo::generateReportTemplate(bool bugzilla) const
{
    //Note: no translations must be done in this function's strings
    QString dottedLine = QString("------");
    QString lineBreak = QLatin1String("<br />");
    if (bugzilla) {
        lineBreak = QLatin1String("\n");
    }

    QString report;
    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();

    report.append(QString("Application and System information :")
                    + lineBreak + dottedLine + lineBreak);
    //Program name and versions
    report.append(QString("KDE Version: %1").arg(getKDEVersion()) + lineBreak);
    report.append(QString("Qt Version: %1").arg(getQtVersion()) + lineBreak);
    report.append(QString("Operating System: %1").arg(getOS()) + lineBreak);
    report.append(QString("Application that crashed: %1").arg(krashConfig->productName())
                    + lineBreak);
    report.append(QString("Version of the application: %1").arg(krashConfig->productVersion())
                    + lineBreak);

    //LSB output
    QString lsb = getLSBRelease();
    if (!bugzilla) {
        lsb.replace('\n', "<br />");
    }
    report.append(dottedLine + lineBreak + lsb + lineBreak  + dottedLine);

    //Description (title)
    if (!m_report.shortDescription().isEmpty()) {
        report.append(lineBreak + QString("Title: %1").arg(m_report.shortDescription()));
    }

    //Details of the crash situation
    if (m_userCanDetail) {
        report.append(lineBreak);
        report.append(lineBreak + QString("What I was doing when the application crashed:")
                        + lineBreak);
        if (!m_userDetailText.isEmpty()) {
            report.append(m_userDetailText);
        } else {
            report.append(i18nc("@info","<placeholder>[[ Insert the details of what were you doing when the "
                                "application crashed (in ENGLISH) here ]]</placeholder>"));
        }
    }

    //Backtrace
    report.append(lineBreak + lineBreak);
    BacktraceParser::Usefulness use =
            DrKonqi::instance()->backtraceGenerator()->parser()->backtraceUsefulness();

    if (use != BacktraceParser::Useless && use != BacktraceParser::InvalidUsefulness) {
        QString formattedBacktrace = DrKonqi::instance()->backtraceGenerator()->backtrace();
        if (!bugzilla) {
            formattedBacktrace.replace('\n', "<br />");
        }
        formattedBacktrace = formattedBacktrace.trimmed();

        report.append(QString("Backtrace:") + lineBreak + dottedLine
                      + lineBreak + formattedBacktrace + dottedLine);
    } else {
        report.append(QString("An useful backtrace could not be generated"));
    }

    //Possible duplicate
    if (!m_possibleDuplicate.isEmpty()) {
        report.append(lineBreak + lineBreak + QString("This bug may be duplicate/related "
                                                      "to bug %1").arg(m_possibleDuplicate));
    }

    return report;
}

void ReportInfo::sendBugReport()
{
    m_bugzilla->sendReport(m_report);
}

void ReportInfo::setDefaultProductComponent()
{
    m_report.setProduct(QLatin1String("kde"));
    m_report.setComponent(QLatin1String("general"));
}

void ReportInfo::fillReportFields()
{
    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();
    m_report.setProduct(krashConfig->productName());
    m_report.setComponent(QLatin1String("general"));
    m_report.setVersion(krashConfig->productVersion());
    m_report.setOperatingSystem(QLatin1String("unspecified"));
    m_report.setPriority(QLatin1String("NOR"));
    m_report.setBugSeverity(QLatin1String("crash"));
    m_report.setDescription(generateReportTemplate(true));
    m_report.setValid(true);
}
