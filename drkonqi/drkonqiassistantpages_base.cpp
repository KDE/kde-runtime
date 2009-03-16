/*******************************************************************
* drkonqiassistantpages_base.cpp
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

#include "drkonqiassistantpages_base.h"

#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>

#include <kpushbutton.h>
#include <ktextbrowser.h>
#include <ktoolinvocation.h>
#include <kicon.h>
#include <kfiledialog.h>
#include <ksavefile.h>
#include <ktemporaryfile.h>
#include <kmessagebox.h>
#include <klocale.h>

//BEGIN IntroductionPage

IntroductionPage::IntroductionPage() : 
    DrKonqiAssistantPage()
{
    QLabel * mainLabel = new QLabel(
        i18n("<para>This assistant will analyse the crash information and guide you through the bug reporting process</para><para>You can get help about this bug reporting assistant clicking the \"Help\" button</para><para>To start gathering the crash information press the \"Next\" button</para>")
    );
    mainLabel->setWordWrap( true );

    QVBoxLayout * layout = new QVBoxLayout( this );
    layout->addWidget( mainLabel );
    layout->addStretch();
    setLayout( layout );
}

//END IntroductionPage

//BEGIN CrashInformationPage

CrashInformationPage::CrashInformationPage( CrashInfo * crashInfo) 
    : DrKonqiAssistantPage(),
    m_crashInfo( crashInfo )
{
    m_backtraceWidget = new GetBacktraceWidget( m_crashInfo );
    
    //connect backtraceWidget signals
    connect( m_backtraceWidget, SIGNAL(stateChanged()) , this, SLOT(emitCompleteChanged()) );
    
    QLabel * subTitle = new QLabel(
        i18n( "This page will fetch some useful information about the crash to generate a better bug report" )
    );
    subTitle->setWordWrap( true ); 
    subTitle->setAlignment( Qt::AlignHCenter );

    QVBoxLayout * layout = new QVBoxLayout( this );
    layout->addWidget( subTitle );
    layout->addWidget( m_backtraceWidget );
    setLayout( layout );    
}

void CrashInformationPage::aboutToShow()
{ 
    m_backtraceWidget->generateBacktrace(); 
    emitCompleteChanged();
}

bool CrashInformationPage::isComplete()
{
    return ( m_crashInfo->getBacktraceState() != CrashInfo::NonLoaded &&  
        m_crashInfo->getBacktraceState() != CrashInfo::Loading );
}

//END CrashInformationPage

//BEGIN BugAwarenessPage

BugAwarenessPage::BugAwarenessPage(CrashInfo * info) 
    : DrKonqiAssistantPage(),
    m_crashInfo(info)
{
    //Details
    QLabel * detailLabel = 
        new QLabel(
        i18n(
        "<para>Including in your bug report what were you doing when the application crashed will help the developers "
        "reproduce and fix the bug. <i>( Important details are: what you were doing in the application and documents you were using )</i></para>" ) );
        
        /* mention subapplications? sites you were using?
        "<para>Important details are:<br />"
        "<i>* Actions you were taking on the application and on the whole system</i><br />"
        "<i>* Documents you were using in the application (or a reference to them and their type)</i>"
        " ( you can attach files in the bug report once it is filed )</para>" ) );
        */
        
    detailLabel->setWordWrap( true );

    m_canDetailCheckBox = new QCheckBox( i18n( "I can give detailed information about what I was doing when the application crashed" ) );
  
    QGroupBox * canDetailBox = new QGroupBox( i18n( "Can you give detailed information about what were you doing when the application crashed ?" ) );
    QVBoxLayout * canDetailLayout = new QVBoxLayout();
    canDetailBox->setLayout( canDetailLayout );
    canDetailLayout->addWidget( detailLabel );
    canDetailLayout->addWidget( m_canDetailCheckBox );
    
    //Compromise
    QLabel * compromiseLabel = 
        new QLabel(
        i18n( "Sometimes the developers need more information from the reporter in order to fix the bug." )
        );
    compromiseLabel->setWordWrap( true );
    
    m_compromiseCheckBox = 
        new QCheckBox(
        i18n( "I am willing to help the developers by providing further information" )
        );
    
    QGroupBox * getCompromiseBox = new QGroupBox( i18n( "Are you willing to help the developers?" ) );
    QVBoxLayout * getCompromiseLayout = new QVBoxLayout();
    getCompromiseBox->setLayout( getCompromiseLayout );
    getCompromiseLayout->addWidget( compromiseLabel );
    getCompromiseLayout->addWidget( m_compromiseCheckBox );
    
    //Reproduce
    m_canReproduceCheckBox = new QCheckBox( i18n( "I can reproduce the crash and I can provide steps or a testcase" ) );
    
    QGroupBox * canReproduceBox = new QGroupBox( i18n( "Advanced Usage" ) );
    QVBoxLayout * canReproduceLayout = new QVBoxLayout();
    canReproduceBox->setLayout( canReproduceLayout );
    canReproduceLayout->addWidget( m_canReproduceCheckBox);

    //Main layout
    QVBoxLayout * layout = new QVBoxLayout();
    layout->addWidget( canDetailBox );
    layout->addWidget( getCompromiseBox );
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

//END BugAwarenessPage

//BEGIN ConclusionPage

ConclusionPage::ConclusionPage(CrashInfo * info) 
    : DrKonqiAssistantPage(),
    m_crashInfo(info),
    isBKO(false),
    needToReport(false)
{
    m_reportEdit = new KTextBrowser();
    m_reportEdit->setReadOnly( true );

    m_saveReportButton = new KPushButton( KGuiItem( i18nc( "Button action", "&Save to File" ), KIcon("document-save"), i18nc("button explanation", "Use this button to save the generated report and conclusions about this crash to a file. You can use this option to report the bug later."), i18nc("button explanation", "Use this button to save the generated report and conclusions about this crash to a file. You can use this option to report the bug later.") ) );
    connect(m_saveReportButton, SIGNAL(clicked()), this, SLOT(saveReport()) );
    
    m_reportButton = new KPushButton( KGuiItem( i18nc( "button action", "&Report bug to the application maintainer" ), KIcon("document-new"), i18nc( "button explanation", "Use this button to report this bug to the application maintainer. This will launch the browser or the mail client using the assigned bug report address" ), i18nc( "button explanation", "Use this button to report this bug to the application maintainer. This will launch the browser or the mail client using the assigned bug report address" ) ) );
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
        QString defname = m_crashInfo->getCrashConfig()->execName() + '-' + QDate::currentDate().toString("yyyyMMdd") + ".report";
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
                    KGuiItem( i18n( "&Overwrite" ), KIcon("document-save-as"), i18nc( "button explanation", "Use this button to overwrite the current file" ), i18nc( "button explanation", "Use this button to overwrite the current file" ) ) ) )
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
        QString subject = QString( "Automatic crash report generated by DrKonqi for " + m_crashInfo->getProductName() );
        QString body = m_crashInfo->generateReportTemplate();
        KToolInvocation::invokeMailer( m_crashInfo->getReportLink(), "","" ,subject, body);
        //setFinishButton( true ); TODO?
    }
    else
    {
        KToolInvocation::invokeBrowser( m_crashInfo->getReportLink() );
        //setFinishButton( true );
    }
}

void ConclusionPage::aboutToShow()
{
    isBKO = false;
    needToReport = false;
    emitCompleteChanged();
    
    QString report;
    
    BacktraceParser::Usefulness use = m_crashInfo->getBacktraceParser()->backtraceUsefulness();
    bool canDetails = m_crashInfo->getUserCanDetail();
    bool canReproduce = m_crashInfo->getUserCanReproduce();
    bool getCompromise = m_crashInfo->getUserGetCompromise();
    
    switch( use )
    {
        case BacktraceParser::ReallyUseful:
        {
            needToReport = ( canDetails || canReproduce ); //TODO discuss: always true ?
            report = i18n( "* The crash information is really useful and worth reporting" );
            break;
        }
        case BacktraceParser::MayBeUseful:
        {
            needToReport = ( canReproduce || canDetails || getCompromise );
            report = i18n( "* The crash information lacks some details but may be useful" ) ;
            break;
        }
        case BacktraceParser::ProbablyUseless:
        {
            needToReport = ( canReproduce || ( canDetails && getCompromise ) );
            report = i18n( "* The crash information lacks a lot of important details and it is probably useless" );
            break;
        }           
        case BacktraceParser::Useless:
        case BacktraceParser::InvalidUsefulness:
        {
            needToReport = ( canReproduce || ( canDetails && getCompromise ) );
            report = i18n( "* The crash information is completely useless" ) ;
     break;
        }
    }
    
    report.append( QLatin1String( "<br />" ) );
    if( canDetails )
        report += i18n( "* You can explain in detail what were you doing when the application crashed" );
    else
            report += i18n( "* You aren't sure what were you doing when the application crashed" ) ;
       /*        report += i18n( "* You can't say what were you doing when the application crashed" ) ;
        There's something between "very detailed" and "no clue"... */
    
    report.append( QLatin1String( "<br />" ) );
    if( canReproduce )
        report += i18n( "* You can reproduce the crash at will and you can provide steps or a testcase" );
    else
        report += i18n( "* You can't reproduce the crash at will, but you can provide steps or a testcase" ); // is that possible for newbies?
        
    if ( needToReport )
    {
        QString reportMethod;
        QString reportLink;
        
        isBKO = m_crashInfo->isKDEBugzilla();
        m_reportButton->setVisible( !isBKO );
        if ( isBKO )
        {
            emitCompleteChanged();
            m_explanationLabel->setText( i18n( "This application is supported in the KDE Bugtracker, press Next to start the report assistant" ) );
            
            reportMethod = i18n( "You need to file a new bug report, press Next to start the report assistant" );
            reportLink = i18nc( "address to report the bug", "Report at http://bugs.kde.org" );
        }
        else
        {
            m_explanationLabel->setText( i18n( "<strong>Notice:</strong> This application isn't supported in the KDE Bugtracker, you need to report the bug to the maintainer" ) );
            
            reportMethod = i18n( "You need to file a new bug report with the following information:" );
            reportLink =  i18nc( "address(mail or web) to report the bug", "Report to %1", m_crashInfo->getReportLink() );
            
            if( m_crashInfo->isReportMail() )
            {
                m_reportButton->setGuiItem( KGuiItem( i18n("Send a mail to the application maintainer" ), KIcon("mail-message-new"), i18n( "Launches the mail client to send an email to the application maintainer with the crash information" ) , i18n( "Launch the mail client to send an email to the application maintainer with the crash information" ) ) ) ;
            }
            else
            {
                m_reportButton->setGuiItem( KGuiItem( i18n("Open the application maintainer web site" ), KIcon("internet-web-browser"), i18n( "Launches the web browser showing the application web page in order to contact the maintainer" ) , i18n( "Launches the web browser showing the application web page in order to contact the maintainer" ) ) ) ;
            }
        }
        
        report += QString("<br /><strong>%1</strong><br />%2<br /><br />--------<br /><br />%3").arg( i18n( "The crash is worth reporting" ), reportMethod, m_crashInfo->generateReportTemplate() );
        
        report += QString("<br /><br />") + reportLink;
    }
    else
    {
        m_reportButton->setVisible( false );
        
        report += QString("<br /><strong>%1</strong><br />%2<br />--------<br /><br />%3").arg( i18n( "The crash is not worth reporting" ), i18n( "However you can report it on your own if you want, using the following information:" ), m_crashInfo->generateReportTemplate() );
        
        report.append( QString( "<br /><br />" ) );
        if ( m_crashInfo->isKDEBugzilla() )
        {
            report += i18nc( "address to report the bug", "Report at http://bugs.kde.org" );
            m_explanationLabel->setText( i18n( "This application is supported in the KDE Bugtracker, you can report this bug at https://bugs.kde.org" ) );
        }
        else
        {
            report += i18nc( "address(mail or web) to report the bug", "Report to %1", m_crashInfo->getReportLink() );
            m_explanationLabel->setText( i18n( "This application isn't supported in the KDE Bugtracker, you need to report the bug to the maintainer : <i>%1</i>", m_crashInfo->getReportLink() ) );
        }
    }
    
    m_reportEdit->setHtml( report );
}

bool ConclusionPage::isComplete()
{
    return ( isBKO && needToReport );
}

//END ConclusionPage
