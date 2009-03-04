/*******************************************************************
* bugzillalib.cpp
* Copyright 2000-2003 Hans Petter Bieker <bieker@kde.org>     
*           2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
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

#include "bugzillalib.h"

#include <QtCore/QTextStream>
#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <QtXml/QDomNode>
#include <QtXml/QDomNodeList>
#include <QtXml/QDomElement>
#include <QtXml/QDomNamedNodeMap>

#include <QtDebug>

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kurl.h>
#include <klocale.h>

static const char bugtrackerBaseUrl[] = "http://bugstest.kde.org/"; //TODO change to correct HTTPS BKO

static const char columns[] = "bug_severity,priority,bug_status,product,short_desc"; //resolution,

//BKO URLs
static const char loginUrl[] = "index.cgi";
static const char loginParams[] = "GoAheadAndLogIn=1&Bugzilla_login=%1&Bugzilla_password=%2&log_in=Log+in";

static const char searchUrl[] = "buglist.cgi?query_format=advanced&short_desc_type=anywordssubstr&short_desc=%1&product=%2&long_desc_type=anywordssubstr&long_desc=%3&bug_file_loc_type=allwordssubstr&bug_file_loc=&keywords_type=allwords&keywords=&emailtype1=substring&email1=&emailtype2=substring&email2=&bugidtype=include&bug_id=&votes=&chfieldfrom=%4&chfieldto=%5&chfield=[Bug+creation]&chfieldvalue=&cmdtype=doit&field0-0-0=noop&type0-0-0=noop&value0-0-0=&bug_severity=%6&order=Importance&columnlist=%7&ctype=csv";
// short_desc, product, long_desc(possible backtraces lines), searchFrom, searchTo, severity, columnList

static const char fetchBugUrl[] = "show_bug.cgi?id=%1&ctype=xml";

static const char commitReportUrl[] = "post_bug.cgi";
static const char commitReportParams[] = "product=%1&version=unspecified&component=%2&bug_severity=%3&rep_platform=%4&op_sys=%5&priority=%6&bug_status=%7&short_desc=%8&comment=%9";
//version=%3&

BugzillaManager::BugzillaManager():
    QObject(),
    m_logged(false),
    fetchBugJob(0)
{
}

void BugzillaManager::setLoginData( QString _username, QString _password )
{
    m_username = _username;
    m_password = _password;
    m_logged = false;
}

void BugzillaManager::tryLogin()
{
    if ( !m_logged )
    {
        QString params = QString( loginParams ).arg( m_username, m_password );
        QByteArray postData = params.toUtf8();
        
        KIO::StoredTransferJob * loginJob = 
            KIO::storedHttpPost( postData, KUrl( QString(bugtrackerBaseUrl) + QString(loginUrl) ), KIO::HideProgressInfo );
            
        connect( loginJob, SIGNAL(finished(KJob*)) , this, SLOT(loginDone(KJob*)) );
        
        loginJob->addMetaData("content-type", "Content-Type: application/x-www-form-urlencoded");
        loginJob->start();
    }
}

void BugzillaManager::loginDone( KJob* job )
{
    if( !job->error() )
    {
        KIO::StoredTransferJob * loginJob = (KIO::StoredTransferJob *)job;
        QByteArray response = loginJob->data();
        
        //TODO detect errors here ?
        
        if( !response.contains( QByteArray("The username or password you entered is not valid") ) )
            m_logged = true;
        else
            m_logged = false;

        emit loginFinished( m_logged );
    }
    else
    {
        emit loginError( job->errorString() );
    }
}

void BugzillaManager::fetchBugReport( int bugnumber )
{
    KUrl url = KUrl( QString(bugtrackerBaseUrl) + QString(fetchBugUrl).arg( bugnumber ) );
    
    if ( fetchBugJob ) //Stop previous fetchBugJob
    {
        disconnect( fetchBugJob );
        fetchBugJob->kill();
        fetchBugJob = 0;
    }

    fetchBugJob = KIO::storedGet( url, KIO::Reload, KIO::HideProgressInfo);
    connect( fetchBugJob, SIGNAL(finished(KJob*)) , this, SLOT(fetchBugReportDone(KJob*)) );
}

void BugzillaManager::fetchBugReportDone( KJob* job )
{
    if( !job->error() )
    {
        KIO::StoredTransferJob * fetchBugJob = (KIO::StoredTransferJob *)job;
        
        BugReportXMLParser * parser = new BugReportXMLParser( fetchBugJob->data() );
        BugReport * report = parser->parse();
        
        if( parser->isValid() )
        {
            emit bugReportFetched( report );
        } else {
            emit bugReportError( i18n( "Invalid bug report: corrupted data" ) );
        }
        
        delete parser;
    }
    else
    {
        emit bugReportError( job->errorString() );
    }
    
    //delete fetchBugJob;
    fetchBugJob = 0;
}

void BugzillaManager::searchBugs( QString words, QString product, QString severity, QString date_start, QString date_end, QString comment )
{
    QString url = QString(bugtrackerBaseUrl) + QString(searchUrl).arg( words.replace(' ' , '+'), product, comment, date_start,  date_end, severity, QString(columns));
    
    KIO::StoredTransferJob * searchBugsJob = KIO::storedGet( KUrl(url) , KIO::Reload, KIO::HideProgressInfo);
    connect( searchBugsJob, SIGNAL(finished(KJob*)) , this, SLOT(searchBugsDone(KJob*)) );
}

void BugzillaManager::searchBugsDone( KJob * job )
{
    if( !job->error() )
    {
        KIO::StoredTransferJob * searchBugsJob = (KIO::StoredTransferJob *)job;
        
        BugListCSVParser * parser = new BugListCSVParser( searchBugsJob->data() );
        BugMapList list = parser->parse();

        if( parser->isValid() )
            emit searchFinished( list );
        else
            emit searchError( i18n( "Invalid bug list: corrupted data" ) );
        
        delete parser;
    }
    else
    {
        emit searchError( job->errorString() );
    }
}

void BugzillaManager::commitReport( BugReport * report )
{
    QString postDataStr = QString( commitReportParams );
    postDataStr = postDataStr.arg( report->product(), report->component(), report->bugSeverity(),
        QString("Unlisted Binaries"), report->operatingSystem(), report->priority(), report->bugStatus(), report->shortDescription(),
        report->description() );
        
    //QString("unspecified"), 
    
    /*
    postData.replace( QByteArray("%1"), report->product().toUtf8() );
    postData.replace( QByteArray("%2"), report->component().toUtf8() );
    postData.replace( QByteArray("%3"), QString("unspecified").toUtf8() ); //way to get valid versions
    postData.replace( QByteArray("%4"), report->bugSeverity().toUtf8() );
    postData.replace( QByteArray("%5"), QString("Unlisted Binaries").toUtf8() ); // detect distro ?
    postData.replace( QByteArray("%6"), report->operatingSystem().toUtf8() );
    postData.replace( QByteArray("%7"), report->priority().toUtf8() );
    postData.replace( QByteArray("%8"), report->bugStatus().toUtf8() );
    postData.replace( QByteArray("%9"), report->shortDescription().toUtf8() );
    postData.replace( QByteArray("%D"), report->getDescription().toUtf8() );
    */
    
    QByteArray postData = postDataStr.toUtf8();
    
    QString url = QString( bugtrackerBaseUrl ) + QString( commitReportUrl );
    
    KIO::StoredTransferJob * commitJob = 
            KIO::storedHttpPost( postData, KUrl( url ), KIO::HideProgressInfo );
            
    commitJob->addMetaData("content-type", "Content-Type: application/x-www-form-urlencoded");
    commitJob->start();
    
    connect( commitJob, SIGNAL(finished(KJob*)) , this, SLOT(commitReportDone(KJob*)) );
}


void BugzillaManager::commitReportDone( KJob * job )
{
    qDebug() << "done";
    if( !job->error() )
    {
        KIO::StoredTransferJob * commitJob = (KIO::StoredTransferJob *)job;
        QString response = commitJob->data();
  
        qDebug() << response;
        
        //TODO improve  this detection
        if( response.contains("<title>Bug ") )
        {
            QString string1 = QLatin1String("<title>Bug ");
            QString string2 = QLatin1String(" Submitted</title>");
            int index1 = response.indexOf( string1 );
            QString number = response.mid( index1+string1.size()   );
            number = number.mid(0, number.indexOf( string2 ) );
            
            int bug_id = number.toInt();
            
            emit reportCommited( bug_id );
        }
        else
        {
            emit commitReportError( i18n( "There was some error during the report commit. Response:: %1", response ) ); 
            //TODO don't show html, show error (requires parsing)
        }
    }
    else
    {
        emit commitReportError( job->errorString() );
    }
    
}

QString BugzillaManager::urlForBug( int bug_number )
{
    return QString(bugtrackerBaseUrl) + QString::number( bug_number );
}

BugListCSVParser::BugListCSVParser( QByteArray data )
{
    m_data = data;
    m_isValid = false;
}

BugMapList BugListCSVParser::parse()
{
    BugMapList list;
    
    if( !m_data.isEmpty() )
    {
        //Parse buglist CSV
        QTextStream ts( &m_data );
        QString headersLine = ts.readLine().remove( QLatin1Char('\"') ) ; //Discard headers
        QString expectedHeadersLine = QString(columns);
      
        if( headersLine == (QString("bug_id,") + expectedHeadersLine) )
        {
            QStringList headers = expectedHeadersLine.split(',');
            int headersCount = headers.count();
            
            while( !ts.atEnd() )
            {
                BugMap bug; //bug report data map
                
                QString line = ts.readLine();

                //Get bug_id (always at first column)
                int bug_id_index = line.indexOf(',');
                QString bug_id = line.left( bug_id_index );
                bug.insert( "bug_id", bug_id);
                
                line = line.mid( bug_id_index + 2 );
                
                QStringList fields = line.split(",\"");
                
                for(int i = 0; i< headersCount ; i++)
                {
                    QString field = fields.at(i);
                    field = field.left( field.size() - 1 ) ; //Remove trailing "
                    bug.insert( headers.at(i), field );
                }
                
                list.append( bug );
            }
            
            m_isValid = true;
        }
    }
    
    return list;
}
    
BugReportXMLParser::BugReportXMLParser( QByteArray data )
{
    m_valid = m_xml.setContent( data, true );
}

BugReport * BugReportXMLParser::parse()
{
    BugReport * report = new BugReport(); //creates an invalid and empty report object
    
    if ( m_valid )
    {
        //Check bug notfound
        QDomNodeList bug_number = m_xml.elementsByTagName( "bug" );
        QDomNode d = bug_number.at(0);
        QDomNamedNodeMap a = d.attributes();
        QDomNode d2 = a.namedItem("error");
        m_valid = d2.isNull();
        
        if( m_valid )
        {
            m_valid = true;
            report->setValid( true );
            
            //Get basic fields
            report->setBugNumber( getSimpleValue( "bug_id" ) );
            report->setShortDescription( getSimpleValue( "short_desc" ) ); 
            report->setProduct( getSimpleValue( "product" ) );
            report->setComponent( getSimpleValue( "component" ) );
            report->setVersion( getSimpleValue( "version" ) );
            report->setOperatingSystem( getSimpleValue( "op_sys" ) );
            report->setBugStatus( getSimpleValue( "bug_status" ) );
            report->setResolution( getSimpleValue( "resolution" ) );
            report->setPriority( getSimpleValue( "priority" ) );
            report->setBugSeverity( getSimpleValue( "bug_severity" ) );
            
            //Parse full content + comments
            QStringList m_commentList;
            QDomNodeList comments = m_xml.elementsByTagName( "long_desc" );
            for( int i = 0; i< comments.count(); i++)
            {
                QDomElement element = comments.at(i).firstChildElement("thetext");
                m_commentList << element.text();
            }
            
            report->setComments( m_commentList );
            
        } //isValid
    } //isValid

    return report;
}

QString BugReportXMLParser::getSimpleValue( QString name ) //Extract an unique tag from XML
{
    QString ret;
    
    QDomNodeList bug_number = m_xml.elementsByTagName( name );
    if (bug_number.count() == 1)
    {
        QDomNode node = bug_number.at(0);
        ret = node.toElement().text();
    }
    return ret;
}

BugReport::BugReport()
{
    m_isValid = false;
}

QString BugReport::toHtml()
{
    QString html;
    Q_FOREACH( const QString & key, m_dataMap.keys() )
    {
        html.append( QString("<strong>%1:</strong> %2<br />").arg( key, m_dataMap.value(key) ) );
    }
    html.append( "<br />" );
    html.append( i18n("Description:<br /><br />%1", description() ) );
    return html;
}
