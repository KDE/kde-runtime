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

#include <QtCore/QProcess>
#include <kdeversion.h>

ReportInfo::ReportInfo()
{
    m_userCanDetail = false;
    m_userCanReproduce = false;
    m_userIsWillingToHelp = false;
    m_bugzilla = new BugzillaManager();
    m_report = new BugReport();
}

ReportInfo::~ReportInfo()
{
    delete m_bugzilla;
    delete m_report;
}

QString ReportInfo::getKDEVersion() const { return KDE::versionString(); }
QString ReportInfo::getQtVersion() const { return qVersion(); }

QString ReportInfo::getOS() const
{
    if( m_OS.isEmpty() ) //Fetch OS name & ver
    {
        QProcess process;
        process.start("uname -srom");
        process.waitForFinished( 5000 );
        QByteArray os = process.readAllStandardOutput();
        os.chop(1);
        m_OS = QString(os);
    }
    return m_OS;
}

QString ReportInfo::getLSBRelease() const
{
    if( m_LSBRelease.isEmpty() )
    {
        //Get base OS
        QProcess processOS;
        processOS.start("uname -s");
        processOS.waitForFinished( 5000 );
        QByteArray os = processOS.readAllStandardOutput();
        os.chop(1);

        if( QString(os) == QLatin1String( "Linux" ) )
        {
            QProcess process;
            process.start("lsb_release -a");
            process.waitForFinished( 5000 );
            QByteArray lsb = process.readAllStandardOutput();
            if ( !lsb.isEmpty() )
            {
                lsb.chop(1);
                m_LSBRelease = QString(lsb);
            } else {
                m_LSBRelease = i18n( "LSB Release information not found ( no lsb_release command found )" );
            }
        }
        else
        {
            m_LSBRelease = i18n( "Not a GNU/Linux system. LSB Release Information not available" );
        }
    }
    return m_LSBRelease;
}

QString ReportInfo::generateReportTemplate( bool bugzilla ) const
{
    QString lineBreak = QLatin1String("<br />");
    if ( bugzilla )
        lineBreak = QLatin1String("\n");
        
    QString report;
    KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();
    
    report.append( i18n( "Application and System information -----" ) + lineBreak + lineBreak );
    //Program name and versions 
    report.append( QString("KDE Version: %1").arg( getKDEVersion() ) + lineBreak);
    report.append( QString("Qt Version: %1").arg( getQtVersion() ) + lineBreak );
    report.append( QString("Operating System: %1").arg( getOS() ) + lineBreak );
    report.append( QString("Application that crashed: %1").arg( krashConfig->productName() ) + lineBreak );
    report.append( QString("Version of the application: %1").arg( krashConfig->productVersion() ) + lineBreak );
    
    //LSB output
    QString lsb = getLSBRelease();
    if (!bugzilla) lsb.replace('\n', "<br />");
    report.append( QString(" ----- ") + lineBreak + lsb + lineBreak  + QString("-----") + lineBreak );
    
    //Description (title)
    if ( !m_report->shortDescription().isEmpty() )
        report.append( lineBreak + QString("Title: %1").arg( m_report->shortDescription() ) );
    
    //Details of the crash situation
    if( m_userCanDetail )
    {
        report.append( lineBreak );
        report.append( lineBreak + QString("What I was doing when the application crashed:") + lineBreak);
        if( !m_userDetailText.isEmpty() )
        {
            report.append( m_userDetailText );
        } else {
            report.append( i18n("[[ Insert the details of what were you doing when the application crashed (in ENGLISH) here ]]") );
        }
    }

    //Steps to reproduce the crash
    if( m_userCanReproduce )
    {
        report.append( lineBreak );
        report.append( lineBreak + QString("How to reproduce the crash:") + lineBreak);
        if( !m_userReproduceText.isEmpty() )
        {
            report.append( m_userReproduceText );
        } else {
            report.append( i18n("[[ Insert the steps to reproduce the crash (in ENGLISH) here ]]") );
        }
    }
        
    //Backtrace
    report.append( lineBreak );
    if( DrKonqi::instance()->backtraceGenerator()->parser()->backtraceUsefulness() != BacktraceParser::Useless )
    {
        QString formattedBacktrace = DrKonqi::instance()->backtraceGenerator()->backtrace();
        if (!bugzilla)
            formattedBacktrace.replace('\n', "<br />");
        formattedBacktrace = formattedBacktrace.trimmed();
        
        report.append( lineBreak + QString("Backtrace:") + lineBreak + QString("----") 
        + lineBreak + lineBreak + formattedBacktrace + lineBreak + QString("----") );
    }
    else
    {
        report.append( lineBreak + QString("Useless backtrace generated") + lineBreak );
    }

    //Possible duplicate
    if( !m_possibleDuplicate.isEmpty() )
        report.append( lineBreak + lineBreak + QString("This bug may be duplicate/related to bug %1").arg(m_possibleDuplicate) );
        
    return report;
}

void ReportInfo::commitBugReport()
{
    m_bugzilla->commitReport( m_report );
}

void ReportInfo::setDefaultProductComponent()
{
    m_report->setProduct( QLatin1String("kde") );
    m_report->setComponent( QLatin1String("general") );
}

void ReportInfo::fillReportFields()
{
    KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();
    m_report->setProduct( krashConfig->productName() );
    m_report->setComponent( QLatin1String("general") );
    m_report->setVersion( krashConfig->productVersion() );
    m_report->setOperatingSystem( QLatin1String("unspecified") );
    m_report->setBugStatus( QLatin1String("UNCONFIRMED") );
    m_report->setPriority( QLatin1String("NOR") );
    m_report->setBugSeverity( QLatin1String("crash") );
    m_report->setDescription( generateReportTemplate( true ) );
    m_report->setValid( true );
}
