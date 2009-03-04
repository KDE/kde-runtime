/*******************************************************************
* drkonqiassistantpages_bugzilla.cpp
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

#include "drkonqiassistantpages_bugzilla.h"

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
#include <ktextbrowser.h>
#include <ktextedit.h>
#include <kicon.h>
#include <kmessagebox.h>
#include <klineedit.h>

//BEGIN BugzillaLoginPage

BugzillaLoginPage::BugzillaLoginPage( CrashInfo * info) : 
    DrKonqiAssistantPage(),
    m_wallet(0),
    m_crashInfo(info)
{
    connect( m_crashInfo->getBZ(), SIGNAL(loginFinished(bool)), this, SLOT(loginFinished(bool)));
    connect( m_crashInfo->getBZ(), SIGNAL(loginError(QString)), this, SLOT(loginError(QString)));
    
    m_subTitle = new QLabel(
        i18n( "You need to log-in in your bugs.kde.org account in order to proceed" )
    );
    
    m_loginButton = new KPushButton( KGuiItem( i18nc( "button action", "Login in" ), KIcon( "network-connect" ), i18nc( "button explanation", "Use this button to try to login to a bugs.kde.org Bugzilla account using the provided username and password"),  i18nc( "button explanation", "Use this button to try to login to a bugs.kde.org Bugzilla account using the provided username and password") ) );
    connect( m_loginButton, SIGNAL(clicked()) , this, SLOT(loginClicked()) );
    
    m_userEdit = new KLineEdit();
    connect( m_userEdit, SIGNAL(returnPressed()) , this, SLOT(loginClicked()) );
    
    m_passwordEdit = new KLineEdit();
    m_passwordEdit->setEchoMode( QLineEdit::Password );
    connect( m_passwordEdit, SIGNAL(returnPressed()) , this, SLOT(loginClicked()) );
    
    m_form = new QFormLayout();
    m_form->addRow( i18nc( "label for username lineedit input", "Username:" ) , m_userEdit );
    m_form->addRow( i18nc( "label for password lineedit input", "Password:" ) , m_passwordEdit );
    
    m_loginLabel = new QLabel();
    m_loginLabel->setMargin(10);
    
    QLabel * noticeLabel = new QLabel( i18n("<strong>Notice:</strong> You need a user account at the <link url=\"%1\">KDE BugTracker</link> in order to file a bug report because we may need to contact you later for requesting further information. <br />If you don't have one you can freely <link=\"%2\">create one here</link>", "http://bugs.kde.org", "https://bugs.kde.org/createaccount.cgi"));
    
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

void BugzillaLoginPage::loginError( QString err )
{
    loginFinished( false );
    m_loginLabel->setText( i18n("Error when trying to login: %1", err ) );
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
        
        m_loginLabel->setText( i18n( "Logged at KDE Bugtracker (bugs.kde.org) as: %1", m_crashInfo->getBZ()->getUsername() ) );
        
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
                setBackButton( false );
                
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
                else
                {
                    setBackButton( true );
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
        
        m_loginLabel->setText( i18n( "Performing Bugzilla Login ..." ) );
        
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
        
        m_loginLabel->setText( i18n( "Invalid username or password" ) );
        
        m_loginButton->setEnabled( true );
    
        m_userEdit->setEnabled( true );
        m_passwordEdit->setEnabled( true );
        
        m_userEdit->setFocus( Qt::OtherFocusReason );
    }
}

BugzillaLoginPage::~BugzillaLoginPage()
{
    if( m_wallet )
    {
        if( m_wallet->isOpen() ) //Close wallet if we close the assistant in this step
            m_wallet->lockWallet();
        delete m_wallet;
    }
}

//END BugzillaLoginPage

//BEGIN BugzillaKeywordsPage

BugzillaKeywordsPage::BugzillaKeywordsPage(CrashInfo * info) : 
    DrKonqiAssistantPage(),
    m_crashInfo(info)
{
    QLabel * detailsLabel = new QLabel(
    i18n( "Please, enter at least 4 (four) words to describe the crash. This is needed in order to find for similar already reported bugs (duplicates)" ) //TODO rewrite this text
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
    
    bool ok = (list.count() >= 4); //At least four (valid) words
    
    //Check words size
    if ( ok )
    {
        Q_FOREACH( const QString & word, list)
        {
            if( word.size() <= 3 )
                ok = false;
        }
    }
    
    setNextButton( ok );
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

//END BugzillaKeywordsPage

//BEGIN BugzillaDuplicatesPage

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
        
    m_searchingLabel = new QLabel( i18n( "Searching for duplicates ..." ) );
    
    m_bugListWidget = new QTreeWidget();
    m_bugListWidget->setRootIsDecorated( false );
    m_bugListWidget->setWordWrap( true );
    m_bugListWidget->setHeaderLabels( QStringList() << i18n( "Bug ID" ) << i18n( "Description" ) );
    m_bugListWidget->setAlternatingRowColors( true );
    
    connect( m_bugListWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(itemClicked(QTreeWidgetItem*,int)) );
    
    QHeaderView * header = m_bugListWidget->header();
    header->setResizeMode( 0, QHeaderView::ResizeToContents );
    header->setResizeMode( 1, QHeaderView::Interactive );
    
    m_searchMoreButton = new KPushButton( KGuiItem( i18nc( "button action", "Search more reports" ), KIcon("edit-find"), i18nc( " button explanation", "Use this button to search more similar bug reports on an earlier date"), i18nc( " button explanation", "Use this button to search more similar bug reports on an earlier date") ) );
    connect( m_searchMoreButton, SIGNAL(clicked()), this, SLOT(searchMore()) );
    m_searchMoreButton->setEnabled( false );
    
    QHBoxLayout * buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget( m_searchMoreButton );
    
    m_foundDuplicateCheckBox = new QCheckBox( i18n( "My crash may be a duplicate of bug number:" ) );
    connect( m_foundDuplicateCheckBox , SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)) );
    
    m_possibleDuplicateEdit = new KLineEdit();
    m_possibleDuplicateEdit->setInputMask( "000000" );
    m_possibleDuplicateEdit->setEnabled( false );

    QHBoxLayout * lay = new QHBoxLayout();
    lay->addWidget( m_foundDuplicateCheckBox );
    lay->addStretch(); 
    lay->addWidget( m_possibleDuplicateEdit );
    
    QVBoxLayout * layout = new QVBoxLayout();
    layout->addWidget( new QLabel("Needed explanation about this step") ); //TODO rewrite this
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
    enableControls( true );
    
    m_searchingLabel->setText( i18n( "Error fetching the bug report list" ) );
    m_searching = false;
    setIdle( true );
    
    //1 year forward
    m_startDate = m_endDate;
    m_endDate = m_startDate.addYears( 1 );
    
    KMessageBox::error( this , i18n( "Error fetching the bug report list<br />%1<br />Please, wait some time and try again", err ) );
}

void BugzillaDuplicatesPage::bugFetchError( QString err )
{
    if( m_infoDialog )
    {
        if ( m_infoDialog->isVisible() )
        {
            KMessageBox::error( this , i18n( "Error fetching the bug report<br />%1<br />Please, wait some time and try again", err ) );
            m_mineMayBeDuplicateButton->setEnabled( false );
            m_infoDialogBrowser->setText( i18n( "Error fetching the bug report" ) );
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
        
        m_mineMayBeDuplicateButton = new KPushButton( KGuiItem( i18nc("button action", "My crash may be a duplicate of this report"), KIcon(), i18nc( "button explanation", "Use this button to mark your crash as related to this current bug report. This will be used to determine if they are duplicates or completely unrelated"), i18nc( "button explanation", "Use this button to mark your crash as related to this current bug report. This will be used to determine if they are duplicates or completely unrelated") ) );
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
    m_infoDialogBrowser->setText( i18n("Loading information about bug %1 from bugs.kde.org ...", m_currentBugNumber ) );
    m_infoDialogLink->setText( QString("<a href=\"%1\">%2</a>").arg( m_crashInfo->getBZ()->urlForBug(m_currentBugNumber), i18n("Bug report page at KDE Bugtracker") ) );
    
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
                QStringList commentList = report->comments();
                for(int i = 0;i < commentList.count(); i++)
                {
                    QString comment = commentList.at(i);
                    comment.replace('\n', "<br />");
                    comments += "<br /><strong>----</strong><br />" + comment;
                }
                
                QString text = 
                QString( "<strong>%1:</strong> %2<br />" ).arg( i18n("Bug ID"), report->bugNumber() ) +
                QString( "<strong>%1:</strong> %2/%3<br />" ).arg( i18n("Product"), report->product(), report->component() ) +
                QString( "<strong>%1:</strong> %2<br />" ).arg( i18n("Short Description"), report->shortDescription() ) +
                QString( "<strong>%1:</strong> %2<br />" ).arg( i18n("Status"), report->bugStatus() ) +
                QString( "<strong>%1:</strong> %2<br />" ).arg( i18n("Resolution"), report->resolution() ) +
                QString( "<strong>%1:</strong><br />%2" ).arg( i18n("Full Description:"), report->description().replace('\n',"<br />") ) +
                QString( "<br /><br /><strong>%1:</strong> %2" ).arg( i18n("Comments"), comments );
                
                m_infoDialogBrowser->setText( text );
                m_mineMayBeDuplicateButton->setEnabled( true );
            }
        }
    }
    else
    {
        bugFetchError( i18n( "Invalid report" ) );
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
    
    m_searchingLabel->setText( i18n( "Searching for duplicates (from %1 to %2) ...", startDateStr, endDateStr ) );
    
    m_crashInfo->getBZ()->searchBugs( m_crashInfo->getReport()->shortDescription(), m_crashInfo->getProductName(), "crash", startDateStr, endDateStr , m_crashInfo->getBacktraceParser()->firstValidFunctions() );
   
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
    
    int results = list.count();
    if( results > 0 )
    {
        m_searchingLabel->setText( i18n( "Search Finished (results from %1 to %2)", m_startDate.toString("yyyy-MM-dd"), QDate::currentDate().toString("yyyy-MM-dd") ) );
        
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
            m_searchingLabel->setText( i18n( "Search Finished. No more possible date ranges to search" ) );
            
            enableControls( true );
            m_searchMoreButton->setEnabled( false );
        }
    }
}

//END BugzillaDuplicatesPage


//BEGIN BugzillaInformationPage

BugzillaInformationPage::BugzillaInformationPage( CrashInfo * info )
    : DrKonqiAssistantPage(),
    m_crashInfo( info )
{
    m_titleLabel = new QLabel( i18n( "Title of the bug report" ) );
    m_titleEdit = new KLineEdit();
    connect( m_titleEdit, SIGNAL(textChanged(QString)), this, SLOT(checkTexts()) );
    
    m_detailsLabel = new QLabel( i18n( "Details of the crash situation" ) );
    m_detailsEdit = new KTextEdit();
    connect( m_detailsEdit, SIGNAL(textChanged()), this, SLOT(checkTexts()) );
    
    m_reproduceLabel = new QLabel( i18n( "Steps to reproduce" ) );
    m_reproduceEdit = new KTextEdit();
    connect( m_reproduceEdit, SIGNAL(textChanged()), this, SLOT(checkTexts()) );

    QVBoxLayout * layout = new QVBoxLayout();
    layout->addWidget( new QLabel( "Some explanation about this step ??" ) ); //TODO rewrite // text lenght?
    layout->addWidget( m_titleLabel );
    layout->addWidget( m_titleEdit );
    layout->addWidget( m_detailsLabel );
    layout->addWidget( m_detailsEdit );
    layout->addWidget( m_reproduceLabel );
    layout->addWidget( m_reproduceEdit );
    layout->addStretch();
    layout->addWidget( new QLabel( i18n( "<strong>Notice:</strong> The crash information will be automatically integrated into the bug report" ) ) );
    
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
    bool ok = !(m_titleEdit->text().isEmpty() || detailsEmpty || reproduceEmpty );
    
    //Check title, details and steps text lenghts.... TODO discuss the exact lenght
    if ( ok )
        if ( m_titleEdit->text().size() < 20 && m_reproduceEdit->toPlainText().size() < 30 && m_detailsEdit->toPlainText().size() < 30)
            ok = false;
    
    setNextButton( ok );
}

void BugzillaInformationPage::aboutToHide()
{
    //Save fields data
    m_crashInfo->getReport()->setShortDescription( m_titleEdit->text() );
    m_crashInfo->setDetailText( m_detailsEdit->toPlainText() );
    m_crashInfo->setReproduceText( m_reproduceEdit->toPlainText() );
}

//END BugzillaInformationPage

//BEGIN BugzillaCommitPage

BugzillaCommitPage::BugzillaCommitPage( CrashInfo * info )
    : DrKonqiAssistantPage(),
    m_crashInfo( info )
{
    connect( m_crashInfo->getBZ(), SIGNAL(reportCommited(int)), this, SLOT(commited(int)) );
    connect( m_crashInfo->getBZ(), SIGNAL(commitReportError(QString)), this, SLOT(commitError(QString)) );
    
    m_statusLabel = new QLabel();
    m_statusLabel->setWordWrap( true );
    m_statusLabel->setOpenExternalLinks( true );
    
    //TODO button to retry on error ?
    
    QVBoxLayout * layout = new QVBoxLayout();
    layout->addWidget( m_statusLabel );
    setLayout( layout );
}

void BugzillaCommitPage::aboutToShow()
{
    m_statusLabel->setText( i18n( "Generating report ..." ) );
    m_crashInfo->fillReportFields();
    
    m_statusLabel->setText( i18n( "Posting report ..." ) );
    m_crashInfo->commitBugReport();
    
    setBusy();
}

void BugzillaCommitPage::commited( int bug_id )
{
    m_statusLabel->setText( i18n("Report commited!<br />Bug Number :: %1<br />Link :: <link url=\"%2\">%2</link>", bug_id, m_crashInfo->getBZ()->urlForBug( bug_id ) ));
    
    setIdle( false );
    //TODO enable finish button ??
}

void BugzillaCommitPage::commitError( QString errorString )
{
    m_statusLabel->setText( i18n( "Error on commiting bug report %1", errorString ) );
    
    setIdle( false );
}

//END BugzillaCommitPage
