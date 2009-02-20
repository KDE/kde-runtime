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
    m_backtraceInfo = new BacktraceInfo();
    m_backtraceGenerator = 0;
    m_userCanDetail = false;
    m_userCanReproduce = false;
    m_userGetCompromise = false;
    m_bugzilla = new BugzillaManager();
}

CrashInfo::~CrashInfo()
{
    delete m_backtraceInfo;
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
        delete m_backtraceGenerator;
        
    m_backtraceGenerator = BackTrace::create( m_crashConfig, 0 );
    connect(m_backtraceGenerator, SIGNAL(done(QString)), this, SLOT(backtraceGeneratorFinished(QString)));
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
    
    //Parse backtrace to get its information
    m_backtraceInfo->setBacktraceData( data.toLocal8Bit() );
    m_backtraceInfo->parse();
    
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
    return  link.contains('@') && link != QLatin1String( "submit@bugs.kde.org" );
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
    if( m_OS.isEmpty() )
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

QString CrashInfo::getDebugger()
{
    return m_crashConfig->tryExec();
}

QString CrashInfo::generateReportTemplate()
{
    QString report;
    
    report.append( QString("KDE Version: %1<br />Qt Version: %2<br />Operating System: %3<br />Application that crashed: %4"
    "<br />Version of the application: %5<br />").arg( getKDEVersion(), getQtVersion(), getOS(), getProductName(), getProductVersion() )  );
    
    if( m_userCanDetail )
        report.append( QString("<br />What I was doing when the application crashed:<br />[Insert the details here]<br />") );

    if( m_userCanReproduce )
        report.append( QString("<br />How to reproduce the crash:<br />[Insert the steps here]<br />") );
        
    if( m_backtraceInfo->getUsefulness() != BacktraceInfo::Unuseful )
    {
        QString formattedBacktrace = m_backtraceOutput;
        formattedBacktrace.replace('\n', "<br />");
        formattedBacktrace = formattedBacktrace.trimmed();
        
        report.append( QString("<br />Backtrace:<br />----<br /><br />%1<br />--------").arg(formattedBacktrace) );
    }


    return report;
}
