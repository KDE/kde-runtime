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
#include <QtCore/QDate>

#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QFormLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>
//#include <QtGui/QProgressDialog>
#include <QtGui/QTreeWidget>
#include <QtGui/QHeaderView>

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
    m_backtraceWidget = new GetBacktraceWidget( crashInfo );
    connect( m_backtraceWidget, SIGNAL(setNextButton(bool)) , this, SIGNAL(setNextButton(bool)) );
    
    QLabel * subTitle = new QLabel(
        "This page will fetch some useful information about the crash and your system to generate a better bug report" 
    );
    subTitle->setWordWrap( true ); 
    subTitle->setAlignment( Qt::AlignHCenter );

    QVBoxLayout * layout = new QVBoxLayout( this );
    layout->addWidget( subTitle );
    layout->addWidget( m_backtraceWidget );
    
    setLayout( layout );    
}

//Bug Awareness Page
BugAwarenessPage::BugAwarenessPage(CrashInfo * info) : QWidget(), m_crashInfo(info)
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
    m_crashInfo->setUserCanDetail( state == Qt::Checked );
}

void BugAwarenessPage::reproduceStateChanged( int state )
{
    m_crashInfo->setUserCanReproduce( state == Qt::Checked );
}

void BugAwarenessPage::compromiseStateChanged( int state )
{
    m_crashInfo->setUserGetCompromise( state == Qt::Checked );
}


//Results page

ConclusionPage::ConclusionPage(CrashInfo * info) : QWidget(), m_crashInfo(info)
{
    m_conclusionLabel = new QLabel("Conclusions: ");

    m_reportEdit = new KTextBrowser();
    m_reportEdit->setReadOnly( true );

    m_saveReportButton = new KPushButton( KIcon("document-save-as"), "&Save to File"  );
    connect(m_saveReportButton, SIGNAL(clicked()), this, SLOT(saveReport()) );
    
    m_reportButton = new KPushButton( KIcon("document-new"), "&Report bug to the application maintainer" );
    m_reportButton->setVisible( false );
    connect( m_reportButton, SIGNAL(clicked()), this , SLOT(reportButtonClicked()) );

    m_explanationLabel = new QLabel();
    m_explanationLabel->setWordWrap( true );
    
    QHBoxLayout *bLayout = new QHBoxLayout();
    bLayout->addStretch();
    bLayout->addWidget( m_saveReportButton );
    bLayout->addWidget( m_reportButton );

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing( 10 );
    //layout->addWidget( m_conclusionLabel );
    layout->addWidget( m_reportEdit );
    layout->addWidget( m_explanationLabel );
    layout->addLayout( bLayout );

    setLayout( layout );

}

void ConclusionPage::saveReport()
{
    if ( m_crashInfo->getCrashConfig()->safeMode() )
    {
        KTemporaryFile tf;
        tf.setPrefix("/tmp/");
        tf.setSuffix(".report");
        tf.setAutoRemove(false);
        
        if (tf.open())
        {
            QTextStream textStream( &tf );
            textStream << m_reportEdit->toPlainText();
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
        QString defname = m_crashInfo->getCrashConfig()->execName() + "-" + QDate::currentDate().toString("yyyyMMdd") + ".report";
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
                ts << m_reportEdit->toPlainText();
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
    if( m_crashInfo->isReportMail() )
    {
        QString subject = "Automatic crash report created by DrKonqi for " + m_crashInfo->getProductName();
        QString body = m_crashInfo->generateReportTemplate();
        KToolInvocation::invokeMailer( m_crashInfo->getReportLink(), "","" ,subject, body);
        emit setFinishButton( true );
    }
    else
    {
        KToolInvocation::invokeBrowser( m_crashInfo->getReportLink() );
        emit setFinishButton( true );
    }
        
}

void  ConclusionPage::generateResult()
{
    if ( true )
    {
        QString report;
        
        bool needToReport = false;
        
        BacktraceInfo::Usefulness use = m_crashInfo->getBacktraceInfo()->getUsefulness();
        bool canDetails = m_crashInfo->getUserCanDetail();
        bool canReproduce = m_crashInfo->getUserCanReproduce();
        bool getCompromise = m_crashInfo->getUserGetCompromise();
        
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
        else
            report += "<br />* You can't detail what were you doing when the application crashed";
        
        if( canReproduce )
            report += "<br />* You can reproduce the crash at will and you can provide steps or a testcase";
        else
            report += "<br />* You can't reproduce the crash at will and you can provide steps or a testcase";
           
        if ( needToReport )
        {
            m_conclusionLabel->setText("<strong>Conclusions:</strong> Please report this crash");
            
            report += "<br /><strong>The crash is worth reporting</strong><br /><br />You need to file a new bug report with the following information:<br />--------<br /><br />";
            report += m_crashInfo->generateReportTemplate();
            
            bool isBKO = m_crashInfo->isKDEBugzilla();
            
            m_reportButton->setVisible( !isBKO );
            emit setNextButton( isBKO );
            
            if ( isBKO )
            {
                m_explanationLabel->setText( "This application is supported in the KDE Bugtracker, press Next to start the report");
            }
            else
            {
                m_explanationLabel->setText( "<strong>Notice:</strong> This application isn't supported in the KDE Bugtracker, you need to report the bug to the maintainer");
            }
            
        }
        else
        {
            m_reportButton->setVisible( false );
            
            m_conclusionLabel->setText("<strong>Conclusions:</strong> The crash is not worth reporting");
            
            report += "<br /><strong>The crash is not worth reporting</strong><br /><br />However you can report it on your own if you want, using the following information:<br />--------<br /><br />";
            report += m_crashInfo->generateReportTemplate();
            
            if ( m_crashInfo->isKDEBugzilla() )
            {
                report += "<br /><br />Report to http://bugs.kde.org";
                m_explanationLabel->setText( "This application is supported in the KDE Bugtracker, you can report this bug at https://bugs.kde.org");
            }
            else
            {
                report += "<br /><br />Report to " + m_crashInfo->getReportLink();
                m_explanationLabel->setText( QString("This application isn't supported in the KDE Bugtracker, you need to report the bug to the maintainer : <i>%1</i>").arg( m_crashInfo->getReportLink() ) );
            }
            
            emit setNextButton( false );
        }
        
        m_reportEdit->setHtml( report );
    }
}


BugzillaLoginPage::BugzillaLoginPage( CrashInfo * info) : 
    QWidget(),
    m_crashInfo(info)
{
    m_wallet = 0;
    
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
    
    QHBoxLayout * buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget( m_loginButton);
    
    layout->addWidget( m_subTitle );
    layout->addLayout( m_form );
    layout->addWidget( m_loginLabel );
    layout->addLayout( buttonLayout );
    //layout->addWidget( noticeLabel );
    layout->addStretch();
    
    //m_progressDialog = new QProgressDialog( "Login", "", 0 ,0 ,this );
    // ?
    
    setLayout( layout );

    connect( m_crashInfo->getBZ(), SIGNAL(loginFinished(bool)), this, SLOT(loginFinished(bool)));
}

void BugzillaLoginPage::aboutToShow()
{
    if( m_crashInfo->getBZ()->getLogged() )
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
        
        m_loginLabel->setText( "Logged at KDE Bugtracker (bugs.kde.org) as: " + m_crashInfo->getBZ()->getUsername() );
        
        emit setNextButton( true );
    }
    else
    {
        if ( !m_wallet )
        {
            if( KWallet::Wallet::walletList().contains("drkonqi") ) //Do not create the wallet on the first login
            {
                m_wallet = KWallet::Wallet::openWallet("drkonqi", 0 );
                
                //Use wallet data to try login
                
                QByteArray username;
                m_wallet->readEntry("username", username);
                QString password;
                m_wallet->readPassword("password", password);
                
                if( !username.isEmpty() && !password.isEmpty() )
                {
                    m_userEdit->setText( QString(username) );
                    m_passwordEdit->setText( password );
                    loginClicked();
                }
            }
        }
        
        emit setNextButton( false );
    }
    
}

void BugzillaLoginPage::loginClicked()
{
    QApplication::setOverrideCursor(Qt::BusyCursor);
    if( !( m_userEdit->text().isEmpty() && m_passwordEdit->text().isEmpty() ) )
    {
        m_loginLabel->setText( "Performing Bugzilla Login ..." );
        
        m_loginButton->setEnabled( false );
        
        //If the wallet wasn't initialized at startup, launch it now to save the data
        if( !m_wallet )
            m_wallet = KWallet::Wallet::openWallet("drkonqi", 0 );
            
        m_userEdit->setEnabled( false );
        m_passwordEdit->setEnabled( false );
        
        if( m_wallet )
        {
            m_wallet->writeEntry( "username", m_userEdit->text().toLocal8Bit() );
            m_wallet->writePassword( "password", m_passwordEdit->text() );
        }
        
        m_crashInfo->getBZ()->setLoginData( m_userEdit->text(), m_passwordEdit->text() );
        m_crashInfo->getBZ()->tryLogin();    
    }
    else
    {
        loginFinished( false );
    }
}

void BugzillaLoginPage::loginFinished( bool logged )
{
    QApplication::restoreOverrideCursor();
    
    if( logged )
    {
        aboutToShow();
        if( m_wallet )
            if( m_wallet->isOpen() )
                m_wallet->lockWallet();
    } 
    else
    {
        m_loginLabel->setText( "Invalid username or password" );
        
        m_loginButton->setEnabled( true );
    
        m_userEdit->setEnabled( true );
        m_passwordEdit->setEnabled( true );
        
        m_userEdit->setFocus( Qt::OtherFocusReason );
    }
}

BugzillaLoginPage::~BugzillaLoginPage()
{
    if( m_wallet )
        delete m_wallet;
}

BugzillaKeywordsPage::BugzillaKeywordsPage(CrashInfo * info) : 
    QWidget(),
    m_crashInfo(info)
{
    QVBoxLayout * layout = new QVBoxLayout();
    
    QLabel * detailsLabel = new QLabel(
    "Please, enter at least 4 (four) words to describe the crash. This is needed in order to find for similar already reported bugs (duplicates)"
    );
    detailsLabel->setWordWrap( true );
    
    m_keywordsEdit = new KLineEdit();
    connect( m_keywordsEdit, SIGNAL(textEdited(QString)), this, SLOT(textEdited(QString)) );
    
    layout->addWidget( detailsLabel );
    layout->addWidget( m_keywordsEdit );
    layout->addStretch();
    
    setLayout( layout );
}

void BugzillaKeywordsPage::textEdited( QString newText )
{
    //better way to do this?
    QStringList list = newText.split(' ', QString::SkipEmptyParts);
    if( list.count() >= 4 ) //4 words?
    {
        m_crashInfo->getReport()->setShortDescription( newText );
        emit setNextButton( true );    
    }
    else
    {
        emit setNextButton( false );
    }
}

void BugzillaKeywordsPage::aboutToShow()
{
    textEdited( m_keywordsEdit->text() );
}


BugzillaDuplicatesPage::BugzillaDuplicatesPage(CrashInfo * info):
    QWidget(),
    m_crashInfo(info)
{
    m_searching = false;
    m_infoDialog = 0;
    m_infoDialogBrowser = 0;
    m_infoDialogLink = 0;
    
    m_endDate = QDate::currentDate();
    m_startDate = m_endDate.addYears( -1 );
    
    connect( m_crashInfo->getBZ(), SIGNAL( searchFinished(BugMapList) ), this, SLOT( searchFinished(BugMapList) ) );
    connect( m_crashInfo->getBZ(), SIGNAL( bugReportFetched(BugReport*) ), this, SLOT( bugFetchFinished(BugReport*) ) );
        
    QVBoxLayout * layout = new QVBoxLayout();
    
    m_searchingLabel = new QLabel("Searching for duplicates ...");
    
    m_bugListWidget = new QTreeWidget();
    m_bugListWidget->setRootIsDecorated( false );
    m_bugListWidget->setWordWrap( true );
    m_bugListWidget->setHeaderLabels( QStringList() << "Bug ID" << "Description" );
    m_bugListWidget->setAlternatingRowColors( true );
    
    connect( m_bugListWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(itemClicked(QTreeWidgetItem*,int)) );
    
    QHeaderView * header = m_bugListWidget->header();
    header->setResizeMode( 0, QHeaderView::ResizeToContents );
    header->setResizeMode( 1, QHeaderView::Interactive );
    
    m_searchMoreButton = new KPushButton( KGuiItem("Search more reports", KIcon("edit-find")) );
    connect( m_searchMoreButton, SIGNAL(clicked()), this, SLOT(searchMore()) );
    m_searchMoreButton->setEnabled( false );
    
    QHBoxLayout * buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget( m_searchMoreButton );
    
    QGroupBox * box = new QGroupBox("Found Duplicates:") ;
    
    QHBoxLayout * lay = new QHBoxLayout();
    box->setLayout( lay );
    
    m_foundDuplicateCheckBox = new QCheckBox("I have found an already reported crash that is the same as mine at report N°");
    
    QLineEdit * e = new QLineEdit();
    
    lay->addWidget( m_foundDuplicateCheckBox );
    lay->addWidget( e );
    
    layout->addWidget( new QLabel("Needed explanation about this step") ); //TODO
    layout->addWidget( m_searchingLabel );
    layout->addWidget( m_bugListWidget );
    layout->addLayout( buttonLayout );
    layout->addWidget( box );
    
    setLayout( layout );
    
}

BugzillaDuplicatesPage::~BugzillaDuplicatesPage()
{
    if( m_infoDialog )
    {
        delete m_infoDialog;
        delete m_infoDialogBrowser;
    }
}

void BugzillaDuplicatesPage::itemClicked( QTreeWidgetItem * item, int col )
{
    Q_UNUSED( col );
    
    if( !m_infoDialog ) //Build info dialog
    {
        m_infoDialog = new KDialog( this );
        m_infoDialog->setButtons( KDialog::Close );
        m_infoDialog->setDefaultButton( KDialog::Close );
        m_infoDialog->setCaption( i18n("Bug Description") );
        m_infoDialog->setModal( true );
        
        m_infoDialogBrowser = new KTextBrowser(); 
        
        m_infoDialogLink = new QLabel();
        m_infoDialogLink->setOpenExternalLinks( true );
        
        KPushButton * m_mineIsDuplicateButton = new KPushButton( "My report is a duplicate of this" );
        
        QWidget * widget = new QWidget( m_infoDialog );
        QVBoxLayout * layout = new QVBoxLayout();
        layout->addWidget( m_infoDialogBrowser );
        layout->addWidget( m_infoDialogLink );
        layout->addWidget( m_mineIsDuplicateButton );
        widget->setMinimumSize( QSize(400,300) );
        widget->setLayout( layout );
        
        m_infoDialog->setMainWidget( widget );
    }
    
    int bug_number = item->text(0).toInt();
    
    m_crashInfo->getBZ()->fetchBugReport( bug_number );
    
    m_infoDialogBrowser->setText( QString("Loading information about bug %1 from bugs.kde.org ...").arg( bug_number ) );
    m_infoDialogLink->setText( QString("<a href=\"%1\">%2</a>").arg( "https://bugs.kde.org/" + QString::number(bug_number), "Bug report page at KDE Bugtracker" ) );
    
    m_infoDialog->show();
}

void BugzillaDuplicatesPage::bugFetchFinished( BugReport* report )
{
    if( report->isValid() )
    {
        if( m_infoDialog )
        {
            QString comments;
            QStringList commentList = report->getComments();
            for(int i = 0;i < commentList.count(); i++)
            {
                QString comment = commentList.at(i);
                comment.replace('\n', "<br />");
                comments += "<br /><strong>----</strong><br />" + comment;
            }
            
            QString text = 
            QString("<strong>Bug ID:</strong> %1<br /><strong>Product:</strong> %2/%3<br />"
            "<strong>Short Description:</strong> %4<br /><strong>Status:</strong> %5<br />"
            "<strong>Resolution:</strong> %6<br /><strong>Full Description:</strong><br /> %7<br /><br />"
            "<strong>Comments:</strong> %8")
            .arg( report->bugNumber(), report->product(), report->component(),
            report->shortDescription(), report->bugStatus(),
            report->resolution(), report->getDescription().replace('\n',"<br />"), comments );
            
            m_infoDialogBrowser->setText( text );
        }
    }
    else
    {
        m_infoDialogBrowser->setText( "Error on fetching bug report" );
    }
    
    delete report;
}

void BugzillaDuplicatesPage::searchDuplicates()
{
    if( !m_searching )
    {
        m_bugListWidget->clear();
        qDebug() << "starting search";
        performSearch();
    } 
}

void BugzillaDuplicatesPage::performSearch()
{
    m_searching = true;
    m_bugListWidget->setEnabled( false );
    
    QString startDateStr = m_startDate.toString("yyyy-MM-dd");
    QString endDateStr = m_endDate.toString("yyyy-MM-dd");
    
    m_searchingLabel->setText( QString("Searching for duplicates (from %1 to %2) ...").arg( startDateStr, endDateStr ) );
    m_searchMoreButton->setEnabled( false );
    
   m_crashInfo->getBZ()->searchBugs( m_crashInfo->getReport()->shortDescription(), m_crashInfo->getProductName(), "crash", startDateStr, endDateStr , m_crashInfo->getBacktraceInfo()->getFirstValidFunctions() );
   
   //Test search
   //m_crashInfo->getBZ()->searchBugs( "plasma crash folder view", "plasma", "crash", startDateStr, endDateStr , "labelRectangle" );
}

void BugzillaDuplicatesPage::searchMore()
{
    //1 year back
    m_endDate = m_startDate;
    m_startDate = m_endDate.addYears( -1 );
    
    performSearch();
}

void BugzillaDuplicatesPage::searchFinished( const BugMapList & list )
{
    m_searching = false;
    m_bugListWidget->setEnabled( true );
    
    int results = list.count();
    if( results > 0 )
    {
        m_searchingLabel->setText( QString("Search Finished (results from %1 to %2)").arg( m_startDate.toString("yyyy-MM-dd"), QDate::currentDate().toString("yyyy-MM-dd") ) );
        
        for(int i = 0; i< results; i++)
        {
            BugMap bug = list.at(i);
            
            QStringList fields = QStringList() << bug["bug_id"] << bug["short_desc"];
            m_bugListWidget->addTopLevelItem( new QTreeWidgetItem( fields )  );
        }
        
        m_bugListWidget->sortItems( 0 , Qt::DescendingOrder );
        
        if( m_startDate.year() > 1996 )
            m_searchMoreButton->setEnabled( true );
        
    } else {

        if( m_startDate.year() > 1996 )
        {
            searchMore();
        }
        else
        {
            m_searchingLabel->setText("Search Finished. No more possible date ranges to search"); //???TODO
        }
    
    }
}


    /*
    if ( m_crashInfo->isKDEBugzilla() )
    {
        QString reportLink =
            QString("https://bugs.kde.org/wizard.cgi?step=keywords&package=%1&os=%2&kdeVersion=%3&appVersion=%4&keywords_text=%%KEYWORDS%%&distribution=%5").
            arg( m_crashInfo->getProductName(), m_crashInfo->getOS() , m_crashInfo->getKDEVersion() ,
            m_crashInfo->getProductVersion(), "Unlisted Binary Package");
            
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
    