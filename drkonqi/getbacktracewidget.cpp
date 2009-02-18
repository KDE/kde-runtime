//2000-2003 Hans Petter Bieker <bieker@kde.org>

#include "getbacktracewidget.h"

#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QTextEdit>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>

#include <kpushbutton.h>
#include <kicon.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <ktemporaryfile.h>

#include "crashinfo.h"
#include "usefulnessmeter.h"

GetBacktraceWidget::GetBacktraceWidget( CrashInfo * info ) : QWidget()
{
    crashInfo = info;
    connect( crashInfo, SIGNAL(backtraceGenerated()) , this, SLOT(backtraceGenerated()) );
    
    //QLabel * subTitle = new QLabel( "This page will fetch some useful information about the crash and your system to generate a better bug report" );
    //subTitle->setWordWrap( true ); 
    //subTitle->setAlignment( Qt::AlignHCenter );
  
    backtraceEdit = new QTextEdit();
    backtraceEdit->setText("Loading ...");
    backtraceEdit->setReadOnly( true );
     
    statusLabel = new QLabel("Loading ...");
    usefulnessMeter = new UsefulnessMeter( this );

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
    
    QHBoxLayout * backtraceButtons = new QHBoxLayout;
    backtraceButtons->setSpacing( 2 );
    backtraceButtons->addStretch();
    backtraceButtons->addWidget( saveButton );
    backtraceButtons->addWidget( copyButton );
    
    QHBoxLayout * detailsLayout = new QHBoxLayout();
    detailsLayout->addWidget( extraDetailsLabel );
    detailsLayout->addWidget( installDebugButton );
    detailsLayout->addWidget( reloadBacktraceButton );
    
    QHBoxLayout * headerLayout = new QHBoxLayout();
    headerLayout->addWidget( statusLabel ); 
    headerLayout->addStretch();
    headerLayout->addWidget( usefulnessMeter ); 
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing( 4 );
    layout->addLayout( headerLayout );
    layout->addWidget( backtraceEdit );
    layout->addLayout( backtraceButtons );
    layout->addLayout( detailsLayout );

    setLayout(layout);
     
}

void GetBacktraceWidget::regenerateBacktrace()
{
    backtraceEdit->setText( "Loading ... " );
    backtraceEdit->setEnabled( false );
    
    statusLabel->setText( "Loading crash information ... (this will take some time)" );
    usefulnessMeter->setUsefulness( BacktraceInfo::Unuseful );
    usefulnessMeter->setState( CrashInfo::Loading );
    
    extraDetailsLabel->setVisible( false );
    extraDetailsLabel->setText( "" );
    
    reloadBacktraceButton->setVisible( false );
    
    installDebugButton->setVisible( false );
    
    copyButton->setEnabled( false );
    saveButton->setEnabled( false );
                
    QApplication::setOverrideCursor(Qt::BusyCursor);
    
    crashInfo->generateBacktrace();
    
    emit setNextButton( crashInfo->getBacktraceState() != CrashInfo::NonLoaded &&  crashInfo->getBacktraceState() != CrashInfo::Loading );
}

void GetBacktraceWidget::generateBacktrace()
{
    if ( crashInfo->getBacktraceState() == CrashInfo::NonLoaded )
    {
        regenerateBacktrace();
    }
    else if ( crashInfo->getBacktraceState() != CrashInfo::Loading )
    {
        backtraceGenerated(); //Load already gathered information
    }
}
 
void GetBacktraceWidget::backtraceGenerated()
{
    QApplication::restoreOverrideCursor();
    usefulnessMeter->setState( crashInfo->getBacktraceState() );
    
    if( crashInfo->getBacktraceState() == CrashInfo::Loaded )
    {
        backtraceEdit->setEnabled( true );
        backtraceEdit->setText( crashInfo->getBacktraceOutput() );
        BacktraceInfo * btInfo = crashInfo->getBacktraceInfo();
        
        usefulnessMeter->setUsefulness( btInfo->getUsefulness() );
        statusLabel->setText( btInfo->getUsefulnessString() );
        
        QStringList missingSymbols = btInfo->usefulFilesWithMissingSymbols();
        
        if( btInfo->getUsefulness() != BacktraceInfo::ReallyUseful )
        {
            extraDetailsLabel->setVisible( true );
            extraDetailsLabel->setText("The crash information lacks of some important details.<br />You may need to read <a href=\"http://techbase.kde.org/Development/Tutorials/Debugging/How_to_create_useful_crash_reports\">How to create useful crash reports</a> in order to know how to get an useful backtrace.<br />After you install the needed packages you can click the \"Reload Crash Information\" button ");
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
        //usefulnessMeter->setError( true );
        usefulnessMeter->setUsefulness( BacktraceInfo::Unuseful );
        
        statusLabel->setText( "The crash information could not be generated" );
        
        backtraceEdit->setText( "The crash information could not be generated" );
        
        extraDetailsLabel->setVisible( true );
        extraDetailsLabel->setText("You need to install the debug symbols package for this application<br />Please read <a href=\"http://techbase.kde.org/Development/Tutorials/Debugging/How_to_create_useful_crash_reports\">How to create useful crash reports</a> in order to know how to get an useful backtrace.<br />After you install the needed packages you can click the \"Reload Crash Information\" button ");
        reloadBacktraceButton->setVisible( true );
        //TODO
        //labelUsefulness->setText("The backtrace lacks some important information.<br /><br />You need to install the following packages:<br /><i>libqt4-dbg kdelibs5-dbg kdebase-workspace-dbg</i>");
        //You need to read <a href=\"http://techbase.kde.org/Development/Tutorials/Debugging/How_to_create_useful_crash_reports\">How to create useful crash reports</a> in order to know how to get an useful backtrace
        
        //installDebugButton->setVisible( true );
    }
    else if( crashInfo->getBacktraceState() == CrashInfo::DebuggerFailed )
    {
        //usefulnessMeter->setError( true );
        usefulnessMeter->setUsefulness( BacktraceInfo::Unuseful );
        
        statusLabel->setText( "The debugger application is missing or could not be launched" );
        backtraceEdit->setText( "The crash information could not be generated" );
        extraDetailsLabel->setVisible( true );
        extraDetailsLabel->setText("You need to install the debugger package (<i>" + crashInfo->getDebugger() + "</i>) and click the \"Reload Crash Information\" button ");
        reloadBacktraceButton->setVisible( true );
    }
    
    emit setNextButton( crashInfo->getBacktraceState() != CrashInfo::NonLoaded &&  crashInfo->getBacktraceState() != CrashInfo::Loading );
}

void GetBacktraceWidget::copyClicked()
{
    backtraceEdit->selectAll();
    backtraceEdit->copy();
}

void GetBacktraceWidget::saveClicked()
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