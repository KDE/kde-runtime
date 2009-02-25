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
IntroductionPage::IntroductionPage() : DrKonqiAssistantPage()
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
CrashInformationPage::CrashInformationPage( CrashInfo * crashInfo) 
    : DrKonqiAssistantPage()
{
    m_backtraceWidget = new GetBacktraceWidget( crashInfo );
    
    //connect backtraceWidget signals
    connect( m_backtraceWidget, SIGNAL(setBusy()) , this, SLOT(setBusy()) );
    connect( m_backtraceWidget, SIGNAL(setIdle(bool)) , this, SLOT(setIdle(bool)) );
    
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
BugAwarenessPage::BugAwarenessPage(CrashInfo * info) 
    : DrKonqiAssistantPage(),
    m_crashInfo(info)
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

    m_canDetailCheckBox = new QCheckBox( "I can detail what I was doing when the application crashed" );
  
    //Compromise
    QLabel * compromiseLabel = 
        new QLabel(
        "Sometimes the developers need more information from the reporter in order to triage the bug."
        );
    compromiseLabel->setWordWrap( true );
    
    m_compromiseCheckBox = 
        new QCheckBox(
        "I get compromise to help the developers to triage the bug providing further information"
        );
    
    //Reproduce
    QGroupBox * canReproduceBox = new QGroupBox("Advanced Users Only");
    
    QVBoxLayout * canReproduceLayout = new QVBoxLayout();
    
    m_canReproduceCheckBox = new QCheckBox( "I can reproduce the crash and I can provide steps or a testcase" );
    canReproduceLayout->addWidget( m_canReproduceCheckBox);
    
    canReproduceBox->setLayout( canReproduceLayout );
    
    QVBoxLayout * layout = new QVBoxLayout();
    layout->addWidget( detailLabel );
    layout->addWidget( m_canDetailCheckBox );
    layout->addWidget( compromiseLabel );
    layout->addWidget( m_compromiseCheckBox );
    layout->addStretch();
    layout->addWidget( canReproduceBox );
    setLayout( layout );

}

void BugAwarenessPage::aboutToHide()
{
    //Save data
    m_crashInfo->setUserCanDetail( m_canDetailCheckBox->checkState() == Qt::Checked );
    m_crashInfo->setUserCanReproduce( m_canReproduceCheckBox->checkState() == Qt::Checked );
    m_crashInfo->setUserGetCompromise( m_compromiseCheckBox->checkState() == Qt::Checked );
}

//Results page
ConclusionPage::ConclusionPage(CrashInfo * info) 
    : DrKonqiAssistantPage(),
    m_crashInfo(info)
{
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
        //setFinishButton( true );
    }
    else
    {
        KToolInvocation::invokeBrowser( m_crashInfo->getReportLink() );
        //setFinishButton( true );
    }
        
}

void  ConclusionPage::aboutToShow()
{
    QString report;
    
    bool needToReport = false;
    
    BacktraceParser::Usefulness use = m_crashInfo->getBacktraceParser()->backtraceUsefulness();
    bool canDetails = m_crashInfo->getUserCanDetail();
    bool canReproduce = m_crashInfo->getUserCanReproduce();
    bool getCompromise = m_crashInfo->getUserGetCompromise();
    
    switch( use )
    {
        case BacktraceParser::ReallyUseful:
        {
            needToReport = canDetails; //true ?
            report = "* The crash information is really useful and worth reporting";
            break;
        }
        case BacktraceParser::MayBeUseful:
        {
            needToReport = ( canReproduce || canDetails || getCompromise );
            report = "* The crash information lacks some details but may be useful";
            break;
        }
        case BacktraceParser::ProbablyUseless:
        {
            needToReport = ( canReproduce || ( canDetails && getCompromise ) );
            report = "* The crash information lacks a lot of important details and it is probably useless";
            break;
        }           
        case BacktraceParser::Useless:
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
        report += "<br /><strong>The crash is worth reporting</strong><br /><br />You need to file a new bug report with the following information:<br />--------<br /><br />";
        report += m_crashInfo->generateReportTemplate();
        
        bool isBKO = m_crashInfo->isKDEBugzilla();
        
        m_reportButton->setVisible( !isBKO );
        setNextButton( isBKO );
        
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
        
        setNextButton( false );
    }
    
    m_reportEdit->setHtml( report );
}


BugzillaLoginPage::BugzillaLoginPage( CrashInfo * info) : 
    DrKonqiAssistantPage(),
    m_wallet(0),
    m_crashInfo(info)
{
    connect( m_crashInfo->getBZ(), SIGNAL(loginFinished(bool)), this, SLOT(loginFinished(bool)));
    
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

    QVBoxLayout * layout = new QVBoxLayout();
    layout->addWidget( m_subTitle );
    layout->addLayout( m_form );
    layout->addWidget( m_loginLabel );
    layout->addLayout( buttonLayout );
    layout->addWidget( noticeLabel );
    layout->addStretch();
    setLayout( layout );
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
        
        setIdle( true );
        
    }
    else
    {
        setIdle( false );
        bool login = false;
        if ( !m_wallet )
        {
            if( KWallet::Wallet::walletList().contains("drkonqi") ) //Do not create the wallet on the first login
            {
                m_wallet = KWallet::Wallet::openWallet("drkonqi", 0 );
                
                if( m_wallet )
                {
                    //Use wallet data to try login
                    QByteArray username;
                    m_wallet->readEntry("username", username);
                    QString password;
                    m_wallet->readPassword("password", password);
                    
                    if( !username.isEmpty() && !password.isEmpty() )
                    {
                        login = true;
                        m_userEdit->setText( QString(username) );
                        m_passwordEdit->setText( password );
                        loginClicked();
                    }
                }
            }
        }
        //if( !login )
            
    }
}

void BugzillaLoginPage::loginClicked()
{
    if( !( m_userEdit->text().isEmpty() && m_passwordEdit->text().isEmpty() ) )
    {
        setBusy();
        
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
    if( logged )
    {
        aboutToShow();
        if( m_wallet )
            if( m_wallet->isOpen() )
                m_wallet->lockWallet();
    } 
    else
    {
        setIdle( false );
        
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
    DrKonqiAssistantPage(),
    m_crashInfo(info)
{
    QLabel * detailsLabel = new QLabel(
    "Please, enter at least 4 (four) words to describe the crash. This is needed in order to find for similar already reported bugs (duplicates)"
    );
    detailsLabel->setWordWrap( true );
    
    m_keywordsEdit = new KLineEdit();
    connect( m_keywordsEdit, SIGNAL(textEdited(QString)), this, SLOT(textEdited(QString)) );
    
    QVBoxLayout * layout = new QVBoxLayout();
    layout->addWidget( detailsLabel );
    layout->addWidget( m_keywordsEdit );
    layout->addStretch();
    setLayout( layout );
}

void BugzillaKeywordsPage::textEdited( QString newText )
{
    QStringList list = newText.split(' ', QString::SkipEmptyParts);
    bool e = (list.count() >= 4);
    //Check words size??
    
    setNextButton( e );
}

void BugzillaKeywordsPage::aboutToShow()
{
    textEdited( m_keywordsEdit->text() );
}

void BugzillaKeywordsPage::aboutToHide()
{
    //Save keywords (short description)
    m_crashInfo->getReport()->setShortDescription( m_keywordsEdit->text() );
}

BugzillaDuplicatesPage::BugzillaDuplicatesPage(CrashInfo * info):
    DrKonqiAssistantPage(),
    m_searching(false),
    m_infoDialog(0),
    m_infoDialogBrowser(0),
    m_infoDialogLink(0),
    m_mineMayBeDuplicateButton(0),
    m_currentBugNumber(0),
    m_crashInfo(info)
{
    m_endDate = QDate::currentDate();
    m_startDate = m_endDate.addYears( -1 );
    
    connect( m_crashInfo->getBZ(), SIGNAL( searchFinished(BugMapList) ), this, SLOT( searchFinished(BugMapList) ) );
    connect( m_crashInfo->getBZ(), SIGNAL( searchError(QString) ), this, SLOT( searchError(QString) ) );
    connect( m_crashInfo->getBZ(), SIGNAL( bugReportFetched(BugReport*) ), this, SLOT( bugFetchFinished(BugReport*) ) );
    connect( m_crashInfo->getBZ(), SIGNAL( bugReportError(QString) ), this, SLOT( bugFetchError(QString) ) );
        
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
    
    m_foundDuplicateCheckBox = new QCheckBox("My crash may be a duplicate of bug number:");
    connect( m_foundDuplicateCheckBox , SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)) );
    
    m_possibleDuplicateEdit = new KLineEdit();
    m_possibleDuplicateEdit->setInputMask( "000000" );
    m_possibleDuplicateEdit->setEnabled( false );

    QHBoxLayout * lay = new QHBoxLayout();
    lay->addWidget( m_foundDuplicateCheckBox );
    lay->addStretch(); 
    lay->addWidget( m_possibleDuplicateEdit );
    
    QVBoxLayout * layout = new QVBoxLayout();
    layout->addWidget( new QLabel("Needed explanation about this step") ); //TODO
    layout->addWidget( m_searchingLabel );
    layout->addWidget( m_bugListWidget );
    layout->addLayout( buttonLayout );
    layout->addLayout( lay );
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

void BugzillaDuplicatesPage::enableControls( bool enable )
{
    m_bugListWidget->setEnabled( enable );
    m_searchMoreButton->setEnabled( enable );
    m_foundDuplicateCheckBox->setEnabled( enable );
    m_possibleDuplicateEdit->setEnabled( enable );
    if ( enable )
        checkBoxChanged( m_foundDuplicateCheckBox->checkState() );
}

void BugzillaDuplicatesPage::searchError( QString err )
{
    //TODO check this
    enableControls( true );
    
    m_searchingLabel->setText( "Error fetching the bug report list" );
    m_searching = false;
    setIdle( true );
    
    KMessageBox::error( this , QString( "Error fetching the bug report list<br />" + err + "<br />Please, wait some time and try again" ) );
}

void BugzillaDuplicatesPage::bugFetchError( QString err )
{
    if( m_infoDialog )
    {
        if ( m_infoDialog->isVisible() )
        {
            KMessageBox::error( this , QString( "Error fetching the bug report<br />" + err + "<br />Please, wait some time and try again" ) );
            m_mineMayBeDuplicateButton->setEnabled( false );
            m_infoDialogBrowser->setText( "Error fetching the bug report" );
        }
    }
}

void BugzillaDuplicatesPage::checkBoxChanged( int newState )
{
    m_possibleDuplicateEdit->setEnabled( newState == Qt::Checked );
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
        
        m_mineMayBeDuplicateButton = new KPushButton( "My crash may be a duplicate of this report" );
        connect( m_mineMayBeDuplicateButton, SIGNAL(clicked()) , this, SLOT(mayBeDuplicateClicked()) );
        
        QWidget * widget = new QWidget( m_infoDialog );
        QVBoxLayout * layout = new QVBoxLayout();
        layout->addWidget( m_infoDialogBrowser );
        layout->addWidget( m_infoDialogLink );
        layout->addWidget( m_mineMayBeDuplicateButton );
        widget->setMinimumSize( QSize(400,300) );
        widget->setLayout( layout );
        
        m_infoDialog->setMainWidget( widget );
    }
    
    m_currentBugNumber = item->text(0).toInt();
    
    m_crashInfo->getBZ()->fetchBugReport( m_currentBugNumber );

    m_mineMayBeDuplicateButton->setEnabled( false );
    m_infoDialogBrowser->setText( QString("Loading information about bug %1 from bugs.kde.org ...").arg( m_currentBugNumber ) );
    m_infoDialogLink->setText( QString("<a href=\"%1\">%2</a>").arg( "https://bugs.kde.org/" + QString::number(m_currentBugNumber), "Bug report page at KDE Bugtracker" ) );
    
    m_infoDialog->show();
}

void BugzillaDuplicatesPage::mayBeDuplicateClicked()
{
    m_foundDuplicateCheckBox->setCheckState( Qt::Checked );
    m_possibleDuplicateEdit->setText( QString::number( m_currentBugNumber ) );
    m_infoDialog->close();
}

void BugzillaDuplicatesPage::bugFetchFinished( BugReport* report )
{
    if( report->isValid() )
    {
        if( m_infoDialog )
        {
            if ( m_infoDialog->isVisible() )
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
                m_mineMayBeDuplicateButton->setEnabled( true );
            }
        }
    }
    else
    {
        bugFetchError( QString("Invalid report") );
    }
    
    delete report;
}

void BugzillaDuplicatesPage::aboutToShow()
{
    //This shouldn't happen as I can't move page when I'm searching
    Q_ASSERT( !m_searching );
    
    //If I never searched before, performSearch
    if ( m_bugListWidget->topLevelItemCount() == 0)
        performSearch();
    else
        setIdle( true );
}

void BugzillaDuplicatesPage::aboutToHide()
{
    if( (m_foundDuplicateCheckBox->checkState() == Qt::Checked)
        && !m_possibleDuplicateEdit->text().isEmpty() ) 
    {
        m_crashInfo->setPossibleDuplicate( m_possibleDuplicateEdit->text() );
    }

}

void BugzillaDuplicatesPage::performSearch()
{
    setBusy(); //Don't allow go back/next

    m_searching = true;

    enableControls( false );
    
    QString startDateStr = m_startDate.toString("yyyy-MM-dd");
    QString endDateStr = m_endDate.toString("yyyy-MM-dd");
    
    m_searchingLabel->setText( QString("Searching for duplicates (from %1 to %2) ...").arg( startDateStr, endDateStr ) );
    
    m_crashInfo->getBZ()->searchBugs( m_crashInfo->getReport()->shortDescription(), m_crashInfo->getProductName(), "crash", startDateStr, endDateStr , m_crashInfo->getBacktraceParser()->firstValidFunctions() );
   
   //Test search TODO change to original 
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

        enableControls( true );
        
        if( m_startDate.year() < 2002 )
            m_searchMoreButton->setEnabled( false );
        
        setIdle( true );
        
    } else {

        if( m_startDate.year() > 2002 )
        {
            setIdle( false );
            searchMore();
        }
        else
        {
            setIdle( true );
            m_searchingLabel->setText("Search Finished. No more possible date ranges to search"); //???TODO
            
            enableControls( true );
            m_searchMoreButton->setEnabled( false );
        }
    }
}

BugzillaInformationPage::BugzillaInformationPage( CrashInfo * info )
    : DrKonqiAssistantPage(),
    m_crashInfo( info )
{
    m_titleLabel = new QLabel("Title of the bug report");
    m_titleEdit = new KLineEdit();
    connect( m_titleEdit, SIGNAL(textChanged(QString)), this, SLOT(checkTexts()) );
    
    m_detailsLabel = new QLabel( "Details of the crash situation" );
    m_detailsEdit = new KTextEdit();
    connect( m_detailsEdit, SIGNAL(textChanged()), this, SLOT(checkTexts()) );
    
    m_reproduceLabel = new QLabel( "Steps to reproduce" );
    m_reproduceEdit = new KTextEdit();
    connect( m_reproduceEdit, SIGNAL(textChanged()), this, SLOT(checkTexts()) );

    QVBoxLayout * layout = new QVBoxLayout();
    layout->addWidget( new QLabel( "Some explanation about this step ??" ) );
    layout->addWidget( m_titleLabel );
    layout->addWidget( m_titleEdit );
    layout->addWidget( m_detailsLabel );
    layout->addWidget( m_detailsEdit );
    layout->addWidget( m_reproduceLabel );
    layout->addWidget( m_reproduceEdit );
    layout->addStretch();
    layout->addWidget( new QLabel("<strong>Notice:</strong> The crash information will be automatically integrated into the bug report" ));
    
    setLayout( layout );
}

void BugzillaInformationPage::aboutToShow()
{
    if( m_titleEdit->text().isEmpty() )
        m_titleEdit->setText( m_crashInfo->getReport()->shortDescription() );
        
    bool canDetail = m_crashInfo->getUserCanDetail();
    
    m_detailsLabel->setVisible( canDetail );
    m_detailsEdit->setVisible( canDetail );
    
    bool canReproduce = m_crashInfo->getUserCanReproduce();
    
    m_reproduceLabel->setVisible( canReproduce );
    m_reproduceEdit->setVisible( canReproduce );
    
    checkTexts();
}
    
void BugzillaInformationPage::checkTexts()
{
    bool detailsEmpty = m_detailsEdit->isVisible() ? m_detailsEdit->toPlainText().isEmpty() : false;
    bool reproduceEmpty = m_reproduceEdit->isVisible() ? m_reproduceEdit->toPlainText().isEmpty() : false;
    bool empty = (m_titleEdit->text().isEmpty() || detailsEmpty || reproduceEmpty );
    
    //TODO check texts lenght ?
    // discuss lenghts
    
    setNextButton( !empty );
}

void BugzillaInformationPage::aboutToHide()
{
    //Save fields data
    m_crashInfo->getReport()->setShortDescription( m_titleEdit->text() );
    m_crashInfo->setDetailText( m_detailsEdit->toHtml() );
    m_crashInfo->setReproduceText( m_reproduceEdit->toHtml() );
}


BugzillaCommitPage::BugzillaCommitPage( CrashInfo * info )
    : DrKonqiAssistantPage(),
    m_crashInfo( info )
{
    connect( m_crashInfo->getBZ(), SIGNAL(reportCommited()), this, SLOT(commited()) );
    
    m_statusBrowser = new KTextBrowser();
    
    QVBoxLayout * layout = new QVBoxLayout();
    layout->addWidget( m_statusBrowser );
    setLayout( layout );
}

void BugzillaCommitPage::aboutToShow()
{
    m_statusBrowser->setText("Generating report");
    m_crashInfo->fillReportFields();
    m_statusBrowser->setText("Commiting report");
    m_crashInfo->commitBugReport();
    m_statusBrowser->setText("NOT YET!! :( . Press Cancel");
    
    
    m_statusBrowser->setText( "This is the result report:<br /><br />" + m_crashInfo->getReport()->toHtml() );
}

void BugzillaCommitPage::commited()
{
    m_statusBrowser->setText("Report commited !!!");
    //TODO show url
    // enable finish button
}
