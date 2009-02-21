#ifndef BUGZILLALIB__H
#define BUGZILLALIB__H

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QList>

#include <QtXml/QDomDocument>

#include <kio/job.h>

class KJob;
class QString;
class QByteArray;

//Typedefs for Bug Report Listing
typedef QMap<QString,QString>   BugMap; //Report basic fields map
typedef QList<BugMap>           BugMapList; //List of reports

//Main bug report data, full fields + comments
class BugReport: public QObject
{
    Q_OBJECT
    
    public:
    
        BugReport(QByteArray);
        
        QString getData( QString key ) { return m_dataMap.value(key); }
        QString getDescription() { return m_commentList.at(0); }
        QStringList getComments() { return m_commentList; }
        
    private:
        
        bool        m_isValid;
        
        BugMap      m_dataMap;
        QStringList m_commentList;
        
        QDomDocument m_xml;
        //Helper functions
        QString getSimpleValue( QString );
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
        
        bool getLogged() { return m_logged; }
        QString getUsername() { return m_username; }
        
    private Q_SLOTS:
        void loginDone(KJob*);
        void fetchBugReportDone(KJob*);
        void searchBugsDone(KJob*);
        
    Q_SIGNALS:
        void loginFinished( bool );
        void bugReportFetched( BugReport* );
        void searchFinished( BugMapList );

    private:
        QString     m_username;
        QString     m_password;
        bool        m_logged;

};

//?? To set future reports fields and commit ????
class FutureReport
{
    public:
    
        FutureReport( QString app, QString ver);
        
        void setWords( QString words ){ m_words = words; }
        void setFullDescription( QString desc ){ m_fullDescription = desc; }
        
        QString getWords() { return m_words; }
        QString getProduct() { return m_application; }
    private:
    
        QString m_application;
        QString m_version;
        QString m_words;
        QString m_fullDescription;

};

#endif

