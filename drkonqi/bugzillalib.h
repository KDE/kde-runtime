#ifndef BUGZILLALIB__H
#define BUGZILLALIB__H

#include <QtNetwork>
#include <QtXml>

class BugList: public QObject
{
    Q_OBJECT
    
    public:
        BugList( QByteArray );
        QList< QMap<QString, QString> > getList() { return list; }

    private:
        QList< QMap<QString, QString> > list;

};

class BugReport: public QObject
{
    Q_OBJECT
    
    public:
    
        BugReport(QByteArray);
        
        QString getData( QString key ) { return dataMap.value(key); }
        QStringList getComments() { return commentList; }
        
    private:
        
        bool isValid;
        
        QMap<QString, QString> dataMap;
        QStringList commentList;
        
        QDomDocument xml;
        
        //Helper functions
        QString getSimpleValue( QString );
        void parseComments();
};


class BugzillaManager : public QObject
{
    Q_OBJECT
    
    public:
        BugzillaManager();
        void setLoginData( QString, QString );
        void tryLogin();
        
        void fetchBugReport( int );
        void searchBugs( QString words, QString product, QString severity, QString date_start, QString date_end , QString comment);
        
        bool getLogged() { return logged; }
        QString getUsername() { return username; }
        
    private:
        QNetworkAccessManager *manager;
        
        QString username;
        QString password;
        bool logged;

    private Q_SLOTS:
        void replyFinished(QNetworkReply*);
        
    Q_SIGNALS:
        void bugReportFetched( BugReport* );
        void loginFinished( bool );
        void bugList( BugList* );
};

#endif