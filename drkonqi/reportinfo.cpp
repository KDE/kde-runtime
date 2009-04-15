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
                //no translation, string appears in the report
                m_LSBRelease = QString("LSB Release information not found "
                                       "( no lsb_release command found )");
            }
        } else {
            //no translation, string appears in the report
            m_LSBRelease = QString("Not a GNU/Linux system. LSB Release Information not available");
        }
    }
    return m_LSBRelease;
}

QString ReportInfo::generateReportBugzilla() const
{
    //Note: no translations must be done in this function's strings
    QString dottedLine = QLatin1String("------");
    QString lineBreak = QLatin1String("\n");

    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();

    QString report;
    
    report.append(QString("Application and System information:"));
    report.append(lineBreak + dottedLine + lineBreak);
    //Program name and versions
    report.append(QString("KDE Version: %1").arg(getKDEVersion()) + lineBreak);
    report.append(QString("Qt Version: %1").arg(getQtVersion()) + lineBreak);
    report.append(QString("Operating System: %1").arg(getOS()) + lineBreak);
    report.append(QString("Application that crashed: %1").arg(krashConfig->productName())
                    + lineBreak);
    report.append(QString("Version of the application: %1").arg(krashConfig->productVersion()));
    
    //LSB output
    report.append(lineBreak + dottedLine + lineBreak);
    report.append(getLSBRelease());
    report.append(lineBreak + dottedLine + lineBreak);

    //Description (title)
    report.append(lineBreak + QString("Title: %1").arg(m_report.shortDescription()));
    
    //Details of the crash situation
    if (m_userCanDetail) {
        report.append(lineBreak + lineBreak);
        report.append(QString("What I was doing when the application crashed:"));
        report.append(lineBreak);
        report.append(m_userDetailText);
    }

    //Backtrace
    report.append(lineBreak + lineBreak);
    BacktraceParser::Usefulness use =
            DrKonqi::instance()->backtraceGenerator()->parser()->backtraceUsefulness();

    if (use != BacktraceParser::Useless && use != BacktraceParser::InvalidUsefulness) {
        QString formattedBacktrace = DrKonqi::instance()->backtraceGenerator()->backtrace();
        formattedBacktrace = formattedBacktrace.trimmed();
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

QString ReportInfo::generateReportHtml() const
{
    //Note: no translations must be done in this function's strings (except the placeholder)
    QString dottedLine = QLatin1String("------");
    QString lineBreak = QLatin1String("<br />");

    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();

    QString report;
    
    report.append(QLatin1String("<hr />"));
    
    report.append(QLatin1String("<p>"));
    report.append(QString("Application and System information:"));
    
    report.append(lineBreak + dottedLine + lineBreak);
    
    //Program name and versions
    report.append(QString("KDE Version: %1").arg(getKDEVersion()) + lineBreak);
    report.append(QString("Qt Version: %1").arg(getQtVersion()) + lineBreak);
    report.append(QString("Operating System: %1").arg(getOS()) + lineBreak);
    report.append(QString("Application that crashed: %1").arg(krashConfig->productName())
                    + lineBreak);
    report.append(QString("Version of the application: %1").arg(krashConfig->productVersion()));
    
    //LSB output
    report.append(lineBreak + dottedLine + lineBreak);
    report.append(getLSBRelease().replace('\n', "<br />"));
    report.append(lineBreak + dottedLine);
    
    report.append(QLatin1String("</p>"));
    
    //Details of the crash situation
    if (m_userCanDetail) {
        report.append(QLatin1String("<p>"));
        report.append(QString("What I was doing when the application crashed:"));
        report.append(lineBreak);
        report.append(i18nc("@info","<placeholder>In detail, tell us what were you doing when "
                                "the application crashed.</placeholder>"));
        report.append(QLatin1String("</p>"));
    }

    //Backtrace
    report.append(QLatin1String("<p>"));
    BacktraceParser::Usefulness use =
            DrKonqi::instance()->backtraceGenerator()->parser()->backtraceUsefulness();

    if (use != BacktraceParser::Useless && use != BacktraceParser::InvalidUsefulness) {
        QString formattedBacktrace = DrKonqi::instance()->backtraceGenerator()->backtrace();
        formattedBacktrace = formattedBacktrace.mid(0, formattedBacktrace.length()-1).trimmed();
        formattedBacktrace.replace('\n', "<br />");
        report.append(QString("Backtrace:") + lineBreak + dottedLine + lineBreak
                        + formattedBacktrace);
    } else {
        report.append(QString("An useful backtrace could not be generated"));
    }
    report.append(QLatin1String("</p>"));
    
    report.append(QLatin1String("<hr />"));
    
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
    m_report.setDescription(generateReportBugzilla());
    m_report.setValid(true);
}
