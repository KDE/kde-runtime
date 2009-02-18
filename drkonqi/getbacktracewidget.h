#ifndef GETBACKTRACEWIDGET_H
#define GETBACKTRACEWIDGET_H

#include <QtGui/QWidget>

class CrashInfo;
class QTextEdit;
class QLabel;
class KPushButton;
class UsefulnessMeter;

class GetBacktraceWidget: public QWidget
{
    Q_OBJECT
  
    public:
    
        GetBacktraceWidget( CrashInfo* );
        
        void generateBacktrace();

    Q_SIGNALS:
    
        void setNextButton( bool );
        
    private Q_SLOTS:
    
        void regenerateBacktrace();
  
    private:
        
        QTextEdit * backtraceEdit;
        
        QLabel * statusLabel;
        UsefulnessMeter * usefulnessMeter;
        
        QLabel * extraDetailsLabel;
        
        KPushButton * installDebugButton;
        KPushButton * reloadBacktraceButton;
        KPushButton * copyButton;
        KPushButton * saveButton;

        CrashInfo * crashInfo;
        
    private Q_SLOTS:
    
        void backtraceGenerated();
        void saveClicked();
        void copyClicked();

};

#endif