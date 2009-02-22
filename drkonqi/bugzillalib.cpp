#include "bugzillalib.h"

#include <QtCore/QTextStream>
#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <QtXml/QDomNode>
#include <QtXml/QDomNodeList>
#include <QtXml/QDomElement>
#include <QtXml/QDomNamedNodeMap>

//#include <QtDebug>

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kurl.h>

static const char columns[] = "bug_severity,priority,bug_status,product,short_desc"; //resolution,

BugzillaManager::BugzillaManager():
    QObject(),
    m_logged(false)
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
        QByteArray postData("GoAheadAndLogIn=1&Bugzilla_login=%%USERNAME%%&Bugzilla_password=%%PASSWORD%%&log_in=Log+in");
        postData.replace( QByteArray("%%USERNAME%%"), m_username.toLocal8Bit() );
        postData.replace( QByteArray("%%PASSWORD%%"), m_password.toLocal8Bit() );
        
        KIO::StoredTransferJob * loginJob = 
            KIO::storedHttpPost( postData, KUrl("https://bugs.kde.org/index.cgi"), KIO::HideProgressInfo );
            
        connect( loginJob, SIGNAL(finished(KJob*)) , this, SLOT(loginDone(KJob*)) );
        
        loginJob->addMetaData("content-type", "Content-Type: application/x-www-form-urlencoded");
        loginJob->start();
    }
}

void BugzillaManager::loginDone( KJob* job )
{
    KIO::StoredTransferJob * loginJob = (KIO::StoredTransferJob *)job;
    QByteArray response = loginJob->data();
    
    if( !response.contains( QByteArray("The username or password you entered is not valid") ) )
        m_logged = true;
    else
        m_logged = false;

    emit loginFinished( m_logged );
}

void BugzillaManager::fetchBugReport( int bugnumber )
{
    //qDebug() << "fetch report";
    
    KIO::StoredTransferJob * fetchBugJob = 
        KIO::storedGet( KUrl( QString("https://bugs.kde.org/show_bug.cgi?id=%1&ctype=xml").arg( bugnumber ) ) , KIO::Reload, KIO::HideProgressInfo);
        
    connect( fetchBugJob, SIGNAL(finished(KJob*)) , this, SLOT(fetchBugReportDone(KJob*)) );
}

void BugzillaManager::fetchBugReportDone( KJob* job )
{
    //qDebug() << "fetch report done";
    KIO::StoredTransferJob * fetchBugJob = (KIO::StoredTransferJob *)job;
    
    QByteArray response = fetchBugJob->data();
    
    emit bugReportFetched( new BugReport( response ) );
}

void BugzillaManager::searchBugs( QString words, QString product, QString severity, QString date_start, QString date_end, QString comment )
{
    //qDebug() << "search bugs";
    
    QString url = QString("https://bugs.kde.org/buglist.cgi?query_format=advanced&short_desc_type=anywordssubstr&short_desc=%1&product=%2&long_desc_type=anywordssubstr&long_desc=%3&bug_file_loc_type=allwordssubstr&bug_file_loc=&keywords_type=allwords&keywords=&emailtype1=substring&email1=&emailtype2=substring&email2=&bugidtype=include&bug_id=&votes=&chfieldfrom=%4&chfieldto=%5&chfield=[Bug+creation]&chfieldvalue=&cmdtype=doit&field0-0-0=noop&type0-0-0=noop&value0-0-0=&bug_severity=%6&order=Importance&columnlist=%7&ctype=csv")
        .arg( words.replace(' ' , '+'), product, comment, date_start,  date_end, severity, QString(columns));
    
    KIO::StoredTransferJob * searchBugsJob = KIO::storedGet( KUrl(url) , KIO::Reload, KIO::HideProgressInfo);
    connect( searchBugsJob, SIGNAL(finished(KJob*)) , this, SLOT(searchBugsDone(KJob*)) );
    
}

void BugzillaManager::searchBugsDone( KJob * job )
{
    //qDebug() << "search bugs done";
    KIO::StoredTransferJob * searchBugsJob = (KIO::StoredTransferJob *)job;
    
    QByteArray response = searchBugsJob->data();
    
    BugMapList list;
    
    //Parse buglist CSV
    QTextStream ts( &response );
    ts.readLine(); //Discard headers
    QStringList headers = QString(columns).split(',');
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
    
    emit searchFinished( list );
}

BugReport::BugReport( QByteArray data )
{
    m_isValid = m_xml.setContent( data, true );
    
    if ( m_isValid )
    {
        //Check bug notfound
        QDomNodeList bug_number = m_xml.elementsByTagName( "bug" );
        QDomNode d = bug_number.at(0);
        QDomNamedNodeMap a = d.attributes();
        QDomNode d2 = a.namedItem("error");
        m_isValid = d2.isNull();
        
        if( m_isValid )
        {
            //Get basic fields
            m_dataMap.insert( QLatin1String("bug_id"), getSimpleValue( "bug_id" ) );
            m_dataMap.insert( QLatin1String("short_desc"), getSimpleValue( "short_desc" ) ); 
            m_dataMap.insert( QLatin1String("product"), getSimpleValue( "product" ) );
            m_dataMap.insert( QLatin1String("component"), getSimpleValue( "component" ) );
            m_dataMap.insert( QLatin1String("version"), getSimpleValue( "version" ) );
            m_dataMap.insert( QLatin1String("op_sys"), getSimpleValue( "op_sys" ) );
            m_dataMap.insert( QLatin1String("bug_status"), getSimpleValue( "bug_status" ) );
            m_dataMap.insert( QLatin1String("resolution"), getSimpleValue( "resolution" ) );
            m_dataMap.insert( QLatin1String("priority"), getSimpleValue( "priority" ) );
            m_dataMap.insert( QLatin1String("bug_severity"), getSimpleValue( "bug_severity" ) );
            
            //Parse full content + comments
            QDomNodeList comments = m_xml.elementsByTagName( "long_desc" );
            for( int i = 0; i< comments.count(); i++)
            {
                QDomElement element = comments.at(i).firstChildElement("thetext");
                m_commentList << element.text();
            }
        } //isValid
    } //isValid
}

//Extract an unique tag from XML
QString BugReport::getSimpleValue( QString name )
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

//(?)
FutureReport::FutureReport( QString app, QString ver )
{
    m_application = app;
    m_version = ver;
}

