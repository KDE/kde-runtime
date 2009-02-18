#include "aboutbugreportingdialog.h"

#include <klocale.h>
#include <ktextbrowser.h>

AboutBugReportingDialog::AboutBugReportingDialog(QWidget * parent):
    KDialog(parent)
{
    setWindowIcon( KIcon("help-hint") );
    setCaption( i18n("About Bug Reporting") );
    setModal( true );
    
    setButtons( KDialog::Close );
    setDefaultButton( KDialog::Close );
        
    textBrowser = new KTextBrowser( this );
    textBrowser->setMinimumSize( QSize(500,300) );

    QString text =
    "You can help us to improve this software filing a bug report.<br /><br />"
    "In order to generate an useful bug report we need to fetch some information about the crash and your system.<br /><br />"
    "We also need you to specify some information about the crash.<br /><br />"
    "<strong>Notice:</strong> You are not forced to file a bug report if you don't want to.<br />If you have never seen this dialog before and you don't know what to do you can close it"
    ;
        
    textBrowser->setText( text );
    
    setMainWidget( textBrowser );
}

AboutBugReportingDialog::~AboutBugReportingDialog()
{
    delete textBrowser;
}