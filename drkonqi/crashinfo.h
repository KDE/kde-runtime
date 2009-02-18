#ifndef CRASH__H
#define CRASH__H

#include "backtraceinfo.h"
#include "krashconf.h"
#include "backtrace.h"
#include <kdeversion.h>

class CrashInfo : public QObject
{
    Q_OBJECT
    
  public:
    
    enum BacktraceGenState { DebuggerFailed, NonLoaded, Loading, Loaded, Failed };
    
    CrashInfo( KrashConfig* );
    ~CrashInfo();
    
    const KrashConfig * getCrashConfig() { return m_crashConfig; }
    const QString getCrashTitle();
    
    void generateBacktrace();
    
    BacktraceInfo * getBacktraceInfo() { return m_backtraceInfo; }
    const QString getBacktraceOutput() { return m_backtraceOutput; } 
    BacktraceGenState getBacktraceState() { return m_backtraceState; }
    
    void setUserCanDetail( bool canDetail ) { m_userCanDetail = canDetail; }
    void setUserCanReproduce ( bool canReproduce ) { m_userCanReproduce = canReproduce; }
    void setUserGetCompromise ( bool getCompromise ) { m_userGetCompromise = getCompromise; }
    
    bool getUserCanDetail() { return m_userCanDetail; }
    bool getUserCanReproduce() { return m_userCanReproduce; }
    bool getUserGetCompromise() { return m_userGetCompromise; }
    
    bool isKDEBugzilla();
    bool isReportMail();
    QString getReportLink();
    
    QString getKDEVersion() { return KDE_VERSION_STRING; } 
    QString getQtVersion() { return QT_VERSION_STR; }
    QString getOS();
    QString getProductName();
    QString getProductVersion();
    
    QString getDebugger();
    
    QString generateReportTemplate();
    
  Q_SIGNALS:
    void backtraceGenerated();
  
  private Q_SLOTS:
    void backtraceGeneratorFinished( const QString & );
    void backtraceGeneratorAppend( const QString & );
    void backtraceGeneratorFailed();
    
  private:
  
    KrashConfig *       m_crashConfig;
    
    BackTrace *         m_backtraceGenerator;
    BacktraceInfo *     m_backtraceInfo;
    QString             m_backtraceOutput;
    BacktraceGenState   m_backtraceState;
    
    bool                m_userCanDetail;
    bool                m_userCanReproduce;
    bool                m_userGetCompromise;
  
    QString             m_OS;
};

#endif