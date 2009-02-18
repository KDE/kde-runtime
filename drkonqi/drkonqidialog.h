#ifndef DRKONQIDIALOG__H
#define DRKONQIDIALOG__H

#include <kdialog.h>

class QStackedWidget;
class KrashConfig;
class CrashInfo;
class GetBacktraceWidget;
class KPushButton;
class AboutBugReportingDialog;

class DrKonqiDialog: public KDialog
{
    Q_OBJECT
    
    public:
        DrKonqiDialog(KrashConfig * conf, QWidget * parent = 0);
        ~DrKonqiDialog();
    
    private Q_SLOTS:
    
        void aboutBugReporting();
        void toggleBacktrace();
        void reportBugAssistant();
        
    private:
    
        AboutBugReportingDialog *       aboutBugReportingDialog;
        
        QStackedWidget *        stackedWidget;
        QWidget *               introWidget;
        GetBacktraceWidget * backtraceWidget;
        
        KPushButton *   aboutBugReportingButton;
        CrashInfo *     crashInfo;
    
};

#endif
