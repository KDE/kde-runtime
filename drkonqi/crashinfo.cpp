/*******************************************************************
* crashinfo.cpp
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

#include "crashinfo.h"

#include <QtDebug>
#include <QtCore/QProcess>

CrashInfo::CrashInfo( KrashConfig * cfg )
{
    m_crashConfig = cfg;
    m_backtraceState = NonLoaded;
    m_backtraceParser = BacktraceParser::newParser(cfg->debuggerName());
    m_backtraceGenerator = 0;
    m_userCanDetail = false;
    m_userCanReproduce = false;
    m_userGetCompromise = false;
    m_bugzilla = new BugzillaManager();
    m_report = new BugReport();
}

CrashInfo::~CrashInfo()
{
    delete m_backtraceParser;
    delete m_bugzilla;
    if( m_backtraceGenerator )
        delete m_backtraceGenerator;
}

const QString CrashInfo::getCrashTitle()
{
    QString title = m_crashConfig->errorDescriptionText();
    m_crashConfig->expandString( title, KrashConfig::ExpansionUsageRichText );
    return title;
}

void CrashInfo::generateBacktrace()
{
    if( m_backtraceGenerator )
    {
        disconnect( m_backtraceGenerator );
        delete m_backtraceGenerator;
        m_backtraceGenerator = 0;
    }   
    
    m_backtraceGenerator = new BackTrace( m_crashConfig, 0 );
    m_backtraceParser->connectToDebugger(m_backtraceGenerator);
    connect(m_backtraceParser, SIGNAL(done(QString)), this, SLOT(backtraceGeneratorFinished(QString)));
    connect(m_backtraceGenerator, SIGNAL(someError()), this, SLOT(backtraceGeneratorFailed()));
    connect(m_backtraceGenerator, SIGNAL(append(QString)), this, SLOT(backtraceGeneratorAppend(QString)));
    m_backtraceOutput.clear();
    m_backtraceState = Loading;
    bool result = m_backtraceGenerator->start();
    if (!result) //Debugger not found or couldn't be launched
    {
        m_backtraceState = DebuggerFailed;
        emit backtraceGenerated();
    }
}

void CrashInfo::stopBacktrace()
{
    //Don't remove already (completely) loaded backtrace
    if( m_backtraceState != Loaded )
    {
        if( m_backtraceGenerator )
        {
            disconnect( m_backtraceGenerator );
            delete m_backtraceGenerator;
            m_backtraceGenerator = 0;
        }
    
        m_backtraceOutput.clear();
        m_backtraceState = NonLoaded;   
    }
}

void CrashInfo::backtraceGeneratorAppend( const QString & data )
{
    emit backtraceNewData( data.trimmed() );
}

void CrashInfo::backtraceGeneratorFinished( const QString & data )
{
    QString tmp = i18n( "Application: %progname (%execname), signal %signame" ) + '\n';
    m_crashConfig->expandString( tmp, KrashConfig::ExpansionUsagePlainText );
    
    m_backtraceOutput = tmp + data;
    m_backtraceState = Loaded;

    emit backtraceGenerated();
}

void CrashInfo::backtraceGeneratorFailed()
{
    m_backtraceState = Failed;
    emit backtraceGenerated();
}

QString CrashInfo::getReportLink()
{
    return m_crashConfig->aboutData()->bugAddress();
}

bool CrashInfo::isKDEBugzilla()
{
    return getReportLink() == QLatin1String( "submit@bugs.kde.org" );
}

bool CrashInfo::isReportMail()
{
    QString link = getReportLink();
    return link.contains('@') && link != QLatin1String( "submit@bugs.kde.org" );
}

QString CrashInfo::getProductName()
{
    return m_crashConfig->aboutData()->productName();
}

QString CrashInfo::getProductVersion()
{
    return m_crashConfig->aboutData()->version();
}

QString CrashInfo::getOS()
{
    if( m_OS.isEmpty() ) //Fetch OS name & ver
    {
        QProcess process;
        process.start("uname -srom");
        process.waitForFinished(-1);
        QByteArray os = process.readAllStandardOutput();
        os.chop(1);
        m_OS = QString(os);
    }
    return m_OS;
}

QString CrashInfo::getBaseOS()
{
    if( m_baseOS.isEmpty() ) //Fetch OS name (for bug report)
    {
        QProcess process;
        process.start("uname -s");
        process.waitForFinished(-1);
        QByteArray os = process.readAllStandardOutput();
        os.chop(1);
        m_baseOS = QString(os);
    }
    return m_baseOS;
}

QString CrashInfo::getDebugger()
{
    return m_crashConfig->tryExec();
}

QString CrashInfo::generateReportTemplate( bool bugzilla )
{
    QString lineBreak = QLatin1String("<br />");
    if ( bugzilla )
        lineBreak = QLatin1String("\n");
        
    QString report;
    
    report.append( QString("KDE Version: %1").arg( getKDEVersion() ) + lineBreak);
    report.append( QString("Qt Version: %1").arg( getQtVersion() ) + lineBreak );
    report.append( QString("Operating System: %1").arg( getOS() ) + lineBreak );
    report.append( QString("Application that crashed: %1").arg( getProductName() ) + lineBreak );
    report.append( QString("Version of the application: %1").arg( getProductVersion() ) + lineBreak );
    
    if ( !m_report->shortDescription().isEmpty() )
        report.append( lineBreak + lineBreak + QString("Title: %1").arg( m_report->shortDescription() ) );
    
    report.append( lineBreak );
    
    if( m_userCanDetail )
        report.append( lineBreak + QString("What I was doing when the application crashed:") + lineBreak);
        if( !m_userDetailText.isEmpty() )
        {
            report.append( m_userDetailText );
        } else {
            report.append( QString("[Insert the details here]") );
        }

    report.append( lineBreak ) ;
    
    if( m_userCanReproduce )
        report.append( lineBreak + QString("How to reproduce the crash:") + lineBreak);
        if( !m_userReproduceText.isEmpty() )
        {
            report.append( m_userReproduceText );
        } else {
            report.append( QString("[Insert the steps here]") );
        }
        
    report.append( lineBreak ) ;
    
    if( m_backtraceParser->backtraceUsefulness() != BacktraceParser::Useless )
    {
        QString formattedBacktrace = m_backtraceOutput;
        if (!bugzilla)
            formattedBacktrace.replace('\n', "<br />");
        formattedBacktrace = formattedBacktrace.trimmed();
        
        report.append( lineBreak + QString("Backtrace:") + lineBreak + QString("----") 
        + lineBreak + lineBreak + formattedBacktrace + lineBreak + QString("----") );
    }

    if( !m_possibleDuplicate.isEmpty() )
        report.append( lineBreak + lineBreak + QString("This bug may be duplicate/related to bug %1").arg(m_possibleDuplicate) );
        
    return report;
}

void CrashInfo::commitBugReport()
{
    //Commit
    m_bugzilla->commitReport( m_report );
}

void CrashInfo::fillReportFields()
{
    m_report->setProduct( getProductName() );
    m_report->setComponent( QLatin1String("general") );
    m_report->setVersion( getProductVersion() );
    m_report->setOperatingSystem( getBaseOS() );
    m_report->setBugStatus( QLatin1String("UNCONFIRMED") );
    m_report->setPriority( QLatin1String("NOR") );
    m_report->setBugSeverity( QLatin1String("crash") );
    m_report->setDescription( generateReportTemplate( true ) );
    m_report->setValid( true );
    
    
}