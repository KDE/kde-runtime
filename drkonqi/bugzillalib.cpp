#include "bugzillalib.h"

static const char columns[] = "bug_severity,priority,bug_status,product,short_desc"; //resolution,

BugzillaManager::BugzillaManager()
{
    logged = false;
    username.clear();
    password.clear();
    manager = new QNetworkAccessManager();
    connect(manager, SIGNAL(finished(QNetworkReply*)),
         this, SLOT(replyFinished(QNetworkReply*)));
}

void BugzillaManager::setLoginData( QString _username, QString _password )
{
    username = _username;
    password = _password;
    logged = false;
}

void BugzillaManager::tryLogin()
{
    QByteArray postData("GoAheadAndLogIn=1&Bugzilla_login=USERNAME&Bugzilla_password=PASSWORD&log_in=Log+in");
    postData.replace( QByteArray("USERNAME"), username.toLocal8Bit() );
    postData.replace( QByteArray("PASSWORD"), password.toLocal8Bit() );
    manager->post(QNetworkRequest(QUrl("https://bugs.kde.org/index.cgi")) , postData );
}

void BugzillaManager::searchBugs( QString words, QString product, QString severity, QString date_start, QString date_end, QString comment )
{
    words = words.replace(' ' , '+');
    
    qDebug() << "search bugs";
    
    QString url = QString("https://bugs.kde.org/buglist.cgi?query_format=advanced&short_desc_type=anywordssubstr&short_desc=%1&product=%2&long_desc_type=anywordssubstr&long_desc=%3&bug_file_loc_type=allwordssubstr&bug_file_loc=&keywords_type=allwords&keywords=&emailtype1=substring&email1=&emailtype2=substring&email2=&bugidtype=include&bug_id=&votes=&chfieldfrom=%4&chfieldto=%5&chfield=[Bug+creation]&chfieldvalue=&cmdtype=doit&field0-0-0=noop&type0-0-0=noop&value0-0-0=&bug_severity=%6&order=Importance&columnlist=%7&ctype=csv")
        .arg( words, product, comment, date_start,  date_end, severity, QString(columns));
    
    manager->get(QNetworkRequest(QUrl(url)));
}

void BugzillaManager::fetchBugReport( int bugnumber )
{
    qDebug() << "fetch report";
    manager->get(QNetworkRequest(QUrl("https://bugs.kde.org/show_bug.cgi?id=" + QString::number(bugnumber) + "&ctype=xml")));
}

void BugzillaManager::replyFinished(QNetworkReply* rep)
{
    QByteArray repData = rep->readAll();
    
    qDebug() << "finished for URL:" << rep->url();
    
    //Get bug info
    if( rep->url().path().contains( QByteArray("show_bug.cgi") ) )
    {
        BugReport * report = new BugReport( repData );
        emit bugReportFetched( report );
    }
    //Login 
    else if ( rep->url().path().contains( QByteArray("index.cgi") ) )
    {
        //qDebug() << repData;
        if( !repData.contains( QByteArray("The username or password you entered is not valid") ) )
            logged = true;
        else
            logged = false;
        emit loginFinished( logged );
    }
    //Bug list
    else if ( rep->url().path().contains( QByteArray("buglist.cgi") ) )
    {
        BugList * list = new BugList( repData );
        emit bugList( list );
        
    }
}

BugList::BugList( QByteArray data)
{
    QTextStream ts(&data);
    ts.readLine(); //Discard headers
    QStringList headers = QString(columns).split(',');
    qDebug() << headers;
    while( !ts.atEnd() )
    {
        QMap<QString,QString> bug;
        
        QString line = ts.readLine();

        //Get bug_id
        int bug_id_index = line.indexOf(',');
        QString bug_id = line.left( bug_id_index );
        bug.insert( "bug_id", bug_id);
        
        line = line.mid( bug_id_index + 2 );
        
        QStringList fields = line.split(",\"");
        
        //qDebug() << fields.count();
        //qDebug() << fields;
        //qDebug() << headers.count();
        
        for(int i = 0;i< headers.count() ; i++)
        {
            QString field = fields.at(i);
            field = field.left( field.size() - 1 ) ; //Remove trailing "
            bug.insert( headers.at(i), field );
        }
        
        list.append( bug );
        
    }
    
    //qDebug() << list;
    
}

BugReport::BugReport( QByteArray data )
{
    isValid = xml.setContent( data, true );
    
    if ( isValid )
    {
        //Check bug notfound
        QDomNodeList bug_number = xml.elementsByTagName( "bug" );
        QDomNode d = bug_number.at(0);
        QDomNamedNodeMap a = d.attributes();
        QDomNode d2 = a.namedItem("error");
        isValid = d2.isNull();
        
        if( isValid )
        {
            dataMap.insert( QLatin1String("number"), getSimpleValue( "bug_id" ) );
            dataMap.insert( QLatin1String("short_description"), getSimpleValue( "short_desc" ) ); 
            dataMap.insert( QLatin1String("product"), getSimpleValue( "product" ) );
            dataMap.insert( QLatin1String("component"), getSimpleValue( "component" ) );
            dataMap.insert( QLatin1String("version"), getSimpleValue( "version" ) );
            dataMap.insert( QLatin1String("os"), getSimpleValue( "op_sys" ) );
            dataMap.insert( QLatin1String("status"), getSimpleValue( "bug_status" ) );
            dataMap.insert( QLatin1String("resolution"), getSimpleValue( "resolution" ) );
            dataMap.insert( QLatin1String("priority"), getSimpleValue( "priority" ) );
            dataMap.insert( QLatin1String("severity"), getSimpleValue( "bug_severity" ) );
            
            //qDebug() << dataMap;
            
            parseComments();
        }
    }
    
}

void BugReport::parseComments()
{
    QDomNodeList comments = xml.elementsByTagName( "long_desc" );
    for( int i = 0; i< comments.count(); i++)
    {
        QDomElement element = comments.at(i).firstChildElement("thetext");
        commentList << element.text();
    }
}

QString BugReport::getSimpleValue( QString name )
{
    //Extract an unique tag from XML
    QString ret;
    
    QDomNodeList bug_number = xml.elementsByTagName( name );
    if (bug_number.count() == 1)
    {
        QDomNode node = bug_number.at(0);
        ret = node.toElement().text();
    }
    return ret;
}