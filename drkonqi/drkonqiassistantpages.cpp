/*******************************************************************
* drkonqiassistantpages.cpp
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

#include "drkonqiassistantpages.h"

//#include <QtCore/QRegExpValidator>
//#include <QtCore/QFile> 

#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QFormLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>
#include <QtGui/QProgressDialog>

#include <kpushbutton.h>
#include <ktextedit.h>
#include <kinputdialog.h>
#include <krun.h>
#include <ktoolinvocation.h>
#include <kicon.h>
#include <kfiledialog.h>
#include <ksavefile.h>
#include <ktemporaryfile.h>
#include <kmessagebox.h>
#include <klineedit.h>

//Introduction page
IntroductionPage::IntroductionPage() : QWidget()
{
    QLabel * subTitle = new QLabel(
    "This assistant will guide your through the bug reporting process"
    "<br />More text here? [Learn about bug reporting button?]"
    );
    
    QVBoxLayout * layout = new QVBoxLayout( this );
    layout->addWidget( subTitle );
    layout->addStretch();

    setLayout( layout );
}

//Backtrace page
CrashInformationPage::CrashInformationPage( CrashInfo * crashInfo) : QWidget()
{
    backtraceWidget = new GetBacktraceWidget( crashInfo );
    connect( backtraceWidget, SIGNAL(setNextButton(bool)) , this, SIGNAL(setNextButton(bool)) );
    
    QLabel * subTitle = new QLabel(
        "This page will fetch some useful information about the crash and your system to generate a better bug report" 
    );
    subTitle->setWordWrap( true ); 
    subTitle->setAlignment( Qt::AlignHCenter );

    QVBoxLayout * layout = new QVBoxLayout( this );
    layout->addWidget( subTitle );
    layout->addWidget( backtraceWidget );
    
    setLayout( layout );    
}

//Bug Awareness Page
BugAwarenessPage::BugAwarenessPage(CrashInfo * info) : QWidget(), crashInfo(info)
{
    //Details
    QLabel * detailLabel = 
        new QLabel( 
        "Including in your bug report what were you doing when the application crashed would help the developers "
        "to reproduce the situation and fix the bug.<br /><br />"
        "<strong>Important details are:</strong><br />"
        "<i>* Actions you were taking on the application and on the whole system</i><br />"
        "<i>* Documents you were using in the application (or a reference to them and their type)</i>"
        "(<strong>Note:</strong> you can attach files in the bug report once it is filed )");
    
    detailLabel->setWordWrap( true );

    QCheckBox * canDetailCheckBox = new QCheckBox( "I can detail what I was doing when the application crashed" );
    connect( canDetailCheckBox, SIGNAL(stateChanged(int)), this, SLOT(detailStateChanged(int)) );
  
    //Compromise
    QLabel * compromiseLabel = 
        new QLabel(
        "Sometimes the developers need more information from the reporter in order to triage the bug."
        );
    compromiseLabel->setWordWrap( true );
    
    QCheckBox * compromiseCheckBox = 
        new QCheckBox(
        "I get compromise to help the developers to triage the bug providing further information"
        );
    connect( compromiseCheckBox, SIGNAL(stateChanged(int)), this, SLOT(compromiseStateChanged(int)) );
    
    //Reproduce
    QGroupBox * canReproduceBox = new QGroupBox("Advanced Users Only");
    QVBoxLayout * canReproduceLayout = new QVBoxLayout();
    canReproduceBox->setLayout( canReproduceLayout );
  
    QCheckBox * canReproduceCheckBox = new QCheckBox( "I can reproduce the crash and I can provide steps or a testcase" );
    connect( canReproduceCheckBox, SIGNAL(stateChanged(int)), this, SLOT(reproduceStateChanged(int)) );
  
    canReproduceLayout->addWidget( canReproduceCheckBox);
    
    QVBoxLayout * l = new QVBoxLayout();
    l->addWidget( detailLabel );
    l->addWidget( canDetailCheckBox );
    l->addWidget( compromiseLabel );
    l->addWidget( compromiseCheckBox );
    l->addStretch();
    l->addWidget( canReproduceBox );

    l->setSpacing( 6 );
    setLayout( l );

}

void BugAwarenessPage::detailStateChanged( int state )
{
    crashInfo->setUserCanDetail( state == Qt::Checked );
}

void BugAwarenessPage::reproduceStateChanged( int state )
{
    crashInfo->setUserCanReproduce( state == Qt::Checked );
}

void BugAwarenessPage::compromiseStateChanged( int state )
{
    crashInfo->setUserGetCompromise( state == Qt::Checked );
}


//Results page

ConclusionPage::ConclusionPage(CrashInfo * info) : QWidget(), crashInfo(info)
{
    //generated = false;

    //setCommitPage( true );

    //setTitle("Results");
    //setSubTitle("This page will tell you if you need to report the crash and how to do it");

    conclusionLabel = new QLabel("Conclusions: ");

    reportEdit = new KTextEdit();
    reportEdit->setReadOnly( true );

    saveReportButton = new KPushButton( KIcon("document-save-as"), "&Save Report to File"  );
    saveReportButton->setEnabled( false );
    connect( saveReportButton, SIGNAL(clicked()), this, SLOT(saveReport()) );
    
    reportButton = new KPushButton( KIcon("document-new"), "&Report bug to the application maintainer" );
    reportButton->setEnabled( false );
    connect( reportButton, SIGNAL(clicked()), this , SLOT(reportButtonClicked()) );

    explanationLabel = new QLabel();
    explanationLabel->setWordWrap( true );
    
    QHBoxLayout *bLayout = new QHBoxLayout();
    bLayout->addWidget( saveReportButton );
    bLayout->addWidget( reportButton );

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing( 10 );
    layout->addWidget( conclusionLabel );
    layout->addWidget( reportEdit );
    layout->addWidget( explanationLabel );
    layout->addLayout( bLayout );

    setLayout( layout );

}

void ConclusionPage::saveReport()
{
    if (crashInfo->getCrashConfig()->safeMode())
    {
        KTemporaryFile tf;
        tf.setPrefix("/tmp/");
        tf.setSuffix(".report");
        tf.setAutoRemove(false);
        
        if (tf.open())
        {
            QTextStream textStream( &tf );
            textStream << reportEdit->toPlainText();
            textStream.flush();
            KMessageBox::information(this, i18n("Report saved to <filename>%1</filename>.", tf.fileName()));
        }
        else
        {
            KMessageBox::sorry(this, i18n("Cannot create a file to save the report in"));
        }
    }
    else
    {
        QString defname = crashInfo->getCrashConfig()->execName() + ".report";
        if( defname.contains( '/' ))
            defname = defname.mid( defname.lastIndexOf( '/' ) + 1 );
        QString filename = KFileDialog::getSaveFileName( defname, QString(), this, i18n("Select Filename"));
        if (!filename.isEmpty())
        {
            QFile f(filename);

            if (f.exists()) {
                if (KMessageBox::Cancel ==
                    KMessageBox::warningContinueCancel( 0,
                        i18n( "A file named <filename>%1</filename> already exists. "
                                "Are you sure you want to overwrite it?", filename ),
                        i18n( "Overwrite File?" ),
                    KGuiItem( i18n( "&Overwrite" ) ) ))
                    return;
            }

            if (f.open(QIODevice::WriteOnly))
            {
                QTextStream ts(&f);
                ts << reportEdit->toPlainText();
                f.close();
            }
            else
            {
                KMessageBox::sorry(this, i18n("Cannot open file <filename>%1</filename> for writing.", filename));
            }
        }
    }
}

void ConclusionPage::reportButtonClicked()
{
    /*
    if ( crashInfo->isKDEBugzilla() )
    {
        QString reportLink =
            QString("https://bugs.kde.org/wizard.cgi?step=keywords&package=%1&os=%2&kdeVersion=%3&appVersion=%4&keywords_text=%%KEYWORDS%%&distribution=%5").
            arg( crashInfo->getProductName(), crashInfo->getOS() , crashInfo->getKDEVersion() ,
            crashInfo->getProductVersion(), "Unlisted Binary Package");
            
        bool ok = false;
        //QRegExpValidator * validator = new QRegExpValidator( QRegExp("/(\b[a-z0-9]+\b.*){4,}/i") , 0);
        
        QString keywords = KInputDialog::getText(
            "File a new bug report",
            "Please, introduce at least four words to describe the crash. This is needed in order to search for"
            " similar crashes already reported in the bug tracker","", &ok, this);
        
        //int words = keywords.trimmed().split(' ').size();
        
        if( ok )
        {
            reportLink.replace("%%KEYWORDS%%", keywords, Qt::CaseSensitive);
            KToolInvocation::invokeBrowser( reportLink );
            emit setFinishButton( true );
            
        }
        
        //delete validator;
    
    } else {
    */
    if( crashInfo->isReportMail() )
    {
        QString reportLink = QString("mailto:%1");
        //TODO
        new KRun( KUrl( reportLink ), this );
        emit setFinishButton( true );
        //invokeMailer ???
    }
    else
    {
        KToolInvocation::invokeBrowser( crashInfo->getReportLink() );
        emit setFinishButton( true );
    }
        
}

void  ConclusionPage::generateResult()
{
    if ( true )
    {
        QString report;
        
        bool needToReport = false;
        
        BacktraceInfo::Usefulness use = crashInfo->getBacktraceInfo()->getUsefulness();
        bool canDetails = crashInfo->getUserCanDetail();
        bool canReproduce = crashInfo->getUserCanReproduce();
        bool getCompromise = crashInfo->getUserGetCompromise();
        
        switch( use )
        {
            case BacktraceInfo::ReallyUseful:
            {
                needToReport = canDetails; //true ?
                report = "* The crash information is really useful and worth reporting";
                break;
            }
            case BacktraceInfo::MayBeUseful:
            {
                needToReport = ( canReproduce || canDetails || getCompromise );
                report = "* The crash information lacks some details but may be useful";
                break;
            }
            case BacktraceInfo::ProbablyUnuseful:
            {
                needToReport = ( canReproduce || ( canDetails && getCompromise ) );
                report = "* The crash information lacks a lot of important details and it is probably unuseful";
                break;
            }           
            case BacktraceInfo::Unuseful:
            {
                needToReport = ( canReproduce || ( canDetails && getCompromise ) );
                report = "* The crash information is completely useless";
                break;
            }
        }
        
        if( canDetails )
            report += "<br />* You can detail what were you doing when the application crashed";
        if( canReproduce )
            report += "<br />* You can reproduce the crash at will and you can provide steps or a testcase";
           
        saveReportButton->setEnabled( needToReport );
        
        if ( needToReport )
        {
            conclusionLabel->setText("<strong>Conclusions:</strong> Please report this crash");
            
            report += "<br /><strong>The crash is worth reporting</strong><br /><br />You need to file a new bug report with the following information:<br />--------<br /><br />";
            report += crashInfo->generateReportTemplate();
            
            bool isBKO = crashInfo->isKDEBugzilla();
            
            reportButton->setVisible( ! isBKO );
            emit setNextButton( isBKO );
            
            if ( isBKO )
            {
                explanationLabel->setText( "This application is supported in the KDE Bugtracker, press Next to start the report");
            }
            else
            {
                explanationLabel->setText( "This application isn't supported in the KDE Bugtracker, you need to report the bug to the maintainer");
                reportButton->setEnabled( true );
            }
            
        }
        else
        {
            conclusionLabel->setText("<strong>Conclusions:</strong> The crash is not worth reporting");
            
            report += "<br /><strong>The crash is not worth reporting</strong>";
            
            emit setNextButton( false );
        }
        
        reportEdit->setHtml( report );
    }
}


BugzillaLoginPage::BugzillaLoginPage( CrashInfo * info) : 
    QWidget(),
    crashInfo(info)
{
    crashInfo->getBZ()->fetchBugReport( 174380 );
    
    QVBoxLayout * layout = new QVBoxLayout();
    
    m_subTitle = new QLabel(
    "You need to log-in in your bugs.kde.org account in order to proceed"
    );
    
    m_loginButton = new KPushButton( "Login in" );
    connect( m_loginButton, SIGNAL(clicked()) , this, SLOT(loginClicked()) );
    
    m_userEdit = new KLineEdit();
    connect( m_userEdit, SIGNAL(returnPressed()) , this, SLOT(loginClicked()) );
    
    m_passwordEdit = new KLineEdit();
    m_passwordEdit->setEchoMode( QLineEdit::Password );
    connect( m_passwordEdit, SIGNAL(returnPressed()) , this, SLOT(loginClicked()) );
    
    m_form = new QFormLayout();
    m_form->addRow( "Username:" , m_userEdit );
    m_form->addRow( "Password:" , m_passwordEdit );
    
    m_loginLabel = new QLabel();
    m_loginLabel->setMargin(10);
    
    QLabel * noticeLabel = new QLabel("<strong>Notice:</strong> You need a user account at <a href=\"https://bugs.kde.org/\">bugs.kde.org</a> in order to file a bug report because we may need to contact you later for requesting further information. <br />If you don't have one you can <a href=\"https://bugs.kde.org/createaccount.cgi\">freely create one here</a>");
    noticeLabel->setWordWrap( true );
    noticeLabel->setOpenExternalLinks( true );
    
    layout->addWidget( m_subTitle );
    layout->addLayout( m_form );
    layout->addWidget( m_loginLabel );
    layout->addWidget( m_loginButton );
    layout->addWidget( noticeLabel );
    layout->addStretch();
    
    m_progressDialog = new QProgressDialog( "Login", "", 0 ,0 ,this );
    // ?
    
    
    setLayout( layout );
}

void BugzillaLoginPage::aboutToShow()
{
    if( crashInfo->getBZ()->getLogged() )
    {
        m_loginButton->setEnabled( false );
    
        m_userEdit->setEnabled( false );
        m_userEdit->setText("");
        m_passwordEdit->setEnabled( false );
        m_passwordEdit->setText("");
        
        m_subTitle->setVisible( false );
        m_loginButton->setVisible( false );
        m_userEdit->setVisible( false );
        m_passwordEdit->setVisible( false );
        m_form->labelForField(m_userEdit)->setVisible( false );
        m_form->labelForField(m_passwordEdit)->setVisible( false );
        
        m_loginLabel->setText( "Logged at KDE Bugtracker (bugs.kde.org) as: " + crashInfo->getBZ()->getUsername() );
        
        emit setNextButton( true );
    }
    else
    {
        emit setNextButton( false );
    }
    
}

void BugzillaLoginPage::loginClicked()
{
    QApplication::setOverrideCursor(Qt::BusyCursor);

    m_loginLabel->setText( "Performing Bugzilla Login ..." );
    
    m_loginButton->setEnabled( false );
    
    m_userEdit->setEnabled( false );
    m_passwordEdit->setEnabled( false );
    
    BugzillaManager * bz = crashInfo->getBZ();
    
    connect( bz, SIGNAL(loginFinished(bool)), this, SLOT(loginFinished(bool)));
    bz->setLoginData( m_userEdit->text(), m_passwordEdit->text() );
    bz->tryLogin();    
}

void BugzillaLoginPage::loginFinished( bool logged )
{
    QApplication::restoreOverrideCursor();
    if( logged )
    {
        m_loginLabel->setText( "Login succesfully" );
        aboutToShow();
        
    } else {

        m_loginButton->setEnabled( true );
    
        m_userEdit->setEnabled( true );
        m_passwordEdit->setEnabled( true );
        
        m_loginLabel->setText( "Invalid username or password" );

    }
}


/*
IntroWizardPage::IntroWizardPage( CrashInfo * info) : QWidget()
{
    crashInfo = info;
    
    QLabel * subTitle = new QLabel( crashInfo->getCrashTitle() );
    subTitle->setWordWrap( true ); 
    subTitle->setAlignment( Qt::AlignHCenter );
    
    //GUI
    QLabel * helpUsLabel = new QLabel("You can help us to improve the software to use by filing a bug report in the <a href=\"https://bugs.kde.org\">KDE Bug Tracker</a>");
    helpUsLabel->setWordWrap( true );
    helpUsLabel->setOpenExternalLinks( true );

    QLabel * thingsLabel = new QLabel("In order to generate an useful bug report we need to fetch some information about the crash and  your system.<br />We also need you to specify some information about the crash.<br />If you want to generate this information to file a new report, please select \"Fetch Crash Information\"");
    thingsLabel->setWordWrap( true );
    
    QLabel * noticeLabel = new QLabel("<strong>Notice:</strong> You are not forced to file a bug report if you don't want to.<br />If you want to, you need some knowledge in order to fill the report wizard forms.<br />If you have never seen this dialog before and you don't know what to do you can close this dialog (or select \"Close\")" );
    noticeLabel->setWordWrap( true );
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin( 5 );
    layout->setSpacing( 10 );
    
    layout->addWidget( subTitle );
    layout->addSpacing( 10 );
    layout->addWidget( helpUsLabel );
    layout->addWidget( thingsLabel );
    layout->addWidget( noticeLabel );
    layout->addStretch();
    
    setLayout(layout);

}
*/



/*
FetchInformationWizardPage::FetchInformationWizardPage( CrashInfo * info ) : QWidget()
{
    crashInfo = info;
    connect( crashInfo, SIGNAL(backtraceGenerated()) , this, SLOT(backtraceGenerated()) );
    
    //QLabel * subTitle = new QLabel( "This page will fetch some useful information about the crash and your system to generate a better bug report" );
    //subTitle->setWordWrap( true ); 
    //subTitle->setAlignment( Qt::AlignHCenter );
  
    backtraceEdit = new QTextEdit();
    backtraceEdit->setText("Loading ...");
    backtraceEdit->setReadOnly( true );
     
    statusLabel = new QLabel("Status");
    usefulnessLabel = new UsefulnessLabel( this );

    extraDetailsLabel = new QLabel();
    extraDetailsLabel->setOpenExternalLinks( true );
    extraDetailsLabel->setWordWrap( true );
    
    extraDetailsLabel->setVisible( false );
    
    installDebugButton =  new KPushButton( KIcon("list-add"), "&Install Missing Packages" );
    installDebugButton->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
    installDebugButton->setVisible( false );
    
    reloadBacktraceButton = new KPushButton( KIcon("view-refresh"), "&Reload Crash Information" );
    reloadBacktraceButton->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
    reloadBacktraceButton->setVisible( false );
    connect( reloadBacktraceButton, SIGNAL(clicked()), this, SLOT(regenerateBacktrace()) );

    copyButton = new KPushButton( KIcon("edit-copy") ,"&Copy" );
    connect( copyButton, SIGNAL(clicked()) , this, SLOT(copyClicked()) );
    copyButton->setEnabled( false );

    saveButton = new KPushButton( KIcon("document-save-as"), "&Save" );
    connect( saveButton, SIGNAL(clicked()) , this, SLOT(saveClicked()) );
    saveButton->setEnabled( false );
    
    QHBoxLayout *l2 = new QHBoxLayout;
    l2->setSpacing( 2 );
    //l2->setMargins( 1, 1, 1, 1 );
    //l2->addWidget( reloadBacktraceButton );
    l2->addStretch();
    l2->addWidget( saveButton );
    l2->addWidget( copyButton );
    
    QHBoxLayout * l4 = new QHBoxLayout();
    l4->addWidget( extraDetailsLabel );
    l4->addWidget( installDebugButton );
    l4->addWidget( reloadBacktraceButton );
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing( 5 );
    
    //layout->addWidget( subTitle );
    //layout->addSpacing( 5 );
    
    QHBoxLayout * ll = new QHBoxLayout();
    ll->addWidget( statusLabel ); 
    ll->addStretch();
    ll->addWidget( usefulnessLabel ); 
    
    layout->addLayout( ll );
    layout->addWidget( backtraceEdit );
    layout->addLayout( l2 );
    layout->addLayout(  l4 );

    setLayout(layout);
     
}

void FetchInformationWizardPage::regenerateBacktrace()
{
    backtraceEdit->setText( "Loading ... " );
    
    statusLabel->setText( "Loading crash information ... (this will take some time)" );
    usefulnessLabel->setUsefulness( BacktraceInfo::Unuseful );
    usefulnessLabel->setState( CrashInfo::Loading );
    
    extraDetailsLabel->setVisible( false );
    extraDetailsLabel->setText( "" );
    
    reloadBacktraceButton->setVisible( false );
    
    installDebugButton->setVisible( false );
    
    copyButton->setEnabled( false );
    saveButton->setEnabled( false );
                
    QApplication::setOverrideCursor(Qt::BusyCursor);
    
    crashInfo->generateBacktrace();
    
    emit setNextButton( crashInfo->getBacktraceState() == CrashInfo::Loaded );
}

void FetchInformationWizardPage::generateBacktrace()
{
    if ( crashInfo->getBacktraceState() == CrashInfo::NonLoaded )
    {
        regenerateBacktrace();
    }
}
 
void FetchInformationWizardPage::backtraceGenerated()
{
    QApplication::restoreOverrideCursor();
    usefulnessLabel->setState( crashInfo->getBacktraceState() );
    
    if( crashInfo->getBacktraceState() == CrashInfo::Loaded )
    {
        backtraceEdit->setText( crashInfo->getBacktraceOutput() );
        BacktraceInfo * btInfo = crashInfo->getBacktraceInfo();
        
        usefulnessLabel->setUsefulness( btInfo->getUsefulness() );
        statusLabel->setText( btInfo->getUsefulnessString() );
        
        QStringList missingSymbols = btInfo->usefulFilesWithMissingSymbols();
        
        if( btInfo->getUsefulness() != BacktraceInfo::ReallyUseful )
        {
            extraDetailsLabel->setVisible( true );
            extraDetailsLabel->setText("The crash information lacks of some important details.<br />You may need to read <a href=\"http://techbase.kde.org/Development/Tutorials/Debugging/How_to_create_useful_crash_reports\">How to create useful crash reports</a> in order to know how to get an useful backtrace");
            reloadBacktraceButton->setVisible( true );
            //installDebugButton->setVisible( true );
            
            if (!missingSymbols.isEmpty() ) //Detected missing symbols
            {
                QString details = "You need to install the debug symbols for the following package(s):";
                Q_FOREACH( const QString & pkg, missingSymbols)
                {
                    details += "<i>"+ pkg + "</i> ";
                }
                extraDetailsLabel->setText( extraDetailsLabel->text() + "<br /><br />" + details );
            }
        }
        
        copyButton->setEnabled( true );
        saveButton->setEnabled( true );
    }
    else if( crashInfo->getBacktraceState() == CrashInfo::Failed )
    {
        //usefulnessLabel->setError( true );
        usefulnessLabel->setUsefulness( BacktraceInfo::Unuseful );
        
        statusLabel->setText( "The crash information could not be generated" );
        
        backtraceEdit->setText( "The crash information could not be generated" );
        
        extraDetailsLabel->setVisible( true );
        extraDetailsLabel->setText("Foo bar");
        reloadBacktraceButton->setVisible( true );
        //TODO
        //labelUsefulness->setText("The backtrace lacks some important information.<br /><br />You need to install the following packages:<br /><i>libqt4-dbg kdelibs5-dbg kdebase-workspace-dbg</i>");
        //You need to read <a href=\"http://techbase.kde.org/Development/Tutorials/Debugging/How_to_create_useful_crash_reports\">How to create useful crash reports</a> in order to know how to get an useful backtrace
        
        //installDebugButton->setVisible( true );
    }
    else if( crashInfo->getBacktraceState() == CrashInfo::DebuggerFailed )
    {
        //usefulnessLabel->setError( true );
        usefulnessLabel->setUsefulness( BacktraceInfo::Unuseful );
        
        statusLabel->setText( "The debugger application is missing or could not be launched" );
        backtraceEdit->setText( "The crash information could not be generated" );
        extraDetailsLabel->setVisible( true );
        extraDetailsLabel->setText("You need to install the debugger package (<i>" + crashInfo->getDebugger() + "</i>) and reload the crash information");
        reloadBacktraceButton->setVisible( true );
    }
    
    emit setNextButton( crashInfo->getBacktraceState() == CrashInfo::Loaded );
}

bool FetchInformationWizardPage::isComplete() const
{
    return ( crashInfo->getBacktraceState() == CrashInfo::Loaded );
}

void FetchInformationWizardPage::copyClicked()
{
    backtraceEdit->selectAll();
    backtraceEdit->copy();
}
void FetchInformationWizardPage::saveClicked()
{
  if (crashInfo->getCrashConfig()->safeMode())
  {
    KTemporaryFile tf;
    tf.setPrefix("/tmp/");
    tf.setSuffix(".kcrash");
    tf.setAutoRemove(false);
    
    if (tf.open())
    {
      QTextStream textStream( &tf );
      textStream << crashInfo->getBacktraceOutput();
      textStream.flush();
      KMessageBox::information(this, i18n("Backtrace saved to <filename>%1</filename>.", tf.fileName()));
    }
    else
    {
      KMessageBox::sorry(this, i18n("Cannot create a file to save the backtrace in"));
    }
  }
  else
  {
    QString defname = crashInfo->getCrashConfig()->execName() + ".kcrash";
    if( defname.contains( '/' ))
        defname = defname.mid( defname.lastIndexOf( '/' ) + 1 );
    QString filename = KFileDialog::getSaveFileName( defname, QString(), this, i18n("Select Filename"));
    if (!filename.isEmpty())
    {
      QFile f(filename);

      if (f.exists()) {
        if (KMessageBox::Cancel ==
            KMessageBox::warningContinueCancel( 0,
              i18n( "A file named <filename>%1</filename> already exists. "
                    "Are you sure you want to overwrite it?", filename ),
              i18n( "Overwrite File?" ),
              KGuiItem( i18n( "&Overwrite" ) ) ))
            return;
      }

      if (f.open(QIODevice::WriteOnly))
      {
        QTextStream ts(&f);
        ts << crashInfo->getBacktraceOutput();
        f.close();
      }
      else
      {
        KMessageBox::sorry(this, i18n("Cannot open file <filename>%1</filename> for writing.", filename));
      }
    }
  }
  
}
*/
