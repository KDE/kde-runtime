#ifndef ABOUTBUGREPORTINGDIALOG__H
#define ABOUTBUGREPORTINGDIALOG__H

#include <QWidget>

#include <kdialog.h>

class KTextBrowser;

class AboutBugReportingDialog: public KDialog
{
    Q_OBJECT

    public:
        AboutBugReportingDialog(QWidget * parent = 0);
        ~AboutBugReportingDialog();

    private:
    
        KTextBrowser * textBrowser;
};

#endif
