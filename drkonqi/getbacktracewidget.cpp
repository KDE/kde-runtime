/*******************************************************************
* getbacktracewidget.cpp
* Copyright 2000-2003 Hans Petter Bieker <bieker@kde.org>     
*           2009      Dario Andres Rodriguez <andresbajotierra@gmail.com>
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

#include "getbacktracewidget.h"

#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>

#include <kpushbutton.h>
#include <ktextbrowser.h>
#include <kicon.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <ktemporaryfile.h>
#include <klocale.h>

#include "crashinfo.h"
#include "usefulnessmeter.h"

GetBacktraceWidget::GetBacktraceWidget( CrashInfo * info ) : 
    QWidget(),
    crashInfo(info)
{
    ui.setupUi(this);
    
    connect( crashInfo, SIGNAL(backtraceGenerated()) , this, SLOT(backtraceGenerated()) );
    connect( crashInfo, SIGNAL(backtraceNewData(QString)) , ui.m_backtraceEdit, SLOT(append(QString)) );
    
    ui.m_extraDetailsLabel->setVisible( false );
    ui.m_reloadBacktraceButton->setGuiItem( KGuiItem( i18nc("button action", "&Reload Crash Information" ),KIcon("view-refresh"), i18nc( "button explanation", "Use this button to reload the crash information (backtrace). This is useful when you have installed the proper debug symbols packages and you want to obtain a better backtrace" ), i18nc( "button explanation", "Use this button to reload the crash information (backtrace). This is useful when you have installed the proper debug symbols packages and you want to obtain a better backtrace" ) )  );
    connect( ui.m_reloadBacktraceButton, SIGNAL(clicked()), this, SLOT(regenerateBacktrace()) );

    ui.m_copyButton->setGuiItem( KGuiItem( i18nc("button action", "&Copy" ), KIcon("edit-copy"), i18nc( "button explanation", "Use this button to copy the crash information (backtrace) to the clipboard" ), i18nc( "button explanation", "Use this button to copy the crash information (backtrace) to the clipboard" ) ) );
    connect( ui.m_copyButton, SIGNAL(clicked()) , this, SLOT(copyClicked()) );
    ui.m_copyButton->setEnabled( false );

    ui.m_saveButton->setGuiItem( KGuiItem( i18nc("button action", "&Save to File" ), KIcon("document-save"), i18nc("button explanation", "Use this button to save the crash information (backtrace) to a file. This is useful if you want to take a look on it or to report the bug later" ), i18nc("button explanation", "Use this button to save the crash information (backtrace) to a file. This is useful if you want to take a look on it or to report the bug later" ) ) );
    connect( ui.m_saveButton, SIGNAL(clicked()) , this, SLOT(saveClicked()) );
    ui.m_saveButton->setEnabled( false );

    //StatusWidget
    
    m_usefulnessMeter = new UsefulnessMeter( ui.m_statusWidget );
    ui.m_statusWidget->addCustomStatusWidget( m_usefulnessMeter );
    
    ui.m_statusWidget->setIdle( QString() );
}

void GetBacktraceWidget::setAsLoading()
{
    ui.m_backtraceEdit->setText( i18nc("loading information, wait", "Loading ... " ));
    ui.m_backtraceEdit->setEnabled( false );

    ui.m_statusWidget->setBusy( i18nc("loading information, wait", "Loading crash information ... (this will take some time)" ) );
    m_usefulnessMeter->setUsefulness( BacktraceParser::Useless );
    m_usefulnessMeter->setState( CrashInfo::Loading );
    
    ui.m_extraDetailsLabel->setVisible( false );
    ui.m_extraDetailsLabel->setText( "" );
    
    ui.m_reloadBacktraceButton->setEnabled( false );

    ui.m_copyButton->setEnabled( false );
    ui.m_saveButton->setEnabled( false );
                
    emit stateChanged();
}

void GetBacktraceWidget::regenerateBacktrace()
{
    setAsLoading();
    crashInfo->generateBacktrace();
}

void GetBacktraceWidget::generateBacktrace()
{
    if ( crashInfo->getBacktraceState() == CrashInfo::NonLoaded )
    {
        regenerateBacktrace();
    }
    else if ( crashInfo->getBacktraceState() == CrashInfo::Loading )
    {
        setAsLoading(); //Set in loading state, the widget will catch the backtrace events anyway
    }
    else //*Finished* states
    {
        setAsLoading();
        backtraceGenerated(); //Load already gathered information
    }
}
 
void GetBacktraceWidget::backtraceGenerated()
{
    m_usefulnessMeter->setState( crashInfo->getBacktraceState() );
    
    if( crashInfo->getBacktraceState() == CrashInfo::Loaded )
    {
        ui.m_backtraceEdit->setEnabled( true );
        ui.m_backtraceEdit->setPlainText( crashInfo->getBacktraceOutput() );
        BacktraceParser * btParser = crashInfo->getBacktraceParser();
        
        m_usefulnessMeter->setUsefulness( btParser->backtraceUsefulness() );

        QString usefulnessText;
        switch( btParser->backtraceUsefulness() ) {
            case BacktraceParser::ReallyUseful:
                usefulnessText = i18nc("backtrace rating description", "This crash information is useful");break;
            case BacktraceParser::MayBeUseful:
                usefulnessText = i18nc("backtrace rating description", "This crash information may be useful");break;
            case BacktraceParser::ProbablyUseless:
                usefulnessText = i18nc("backtrace rating description", "This crash information is probably useless");break;
            case BacktraceParser::Useless:
                usefulnessText = i18nc("backtrace rating description", "The crash information is useless");break;
            default:
                //let's hope nobody will ever see this... ;)
                usefulnessText = i18nc("backtrace rating description that should never appear",
                        "The rating of this crash information is invalid. This is a bug in drkonqi itself.");
                break;
        }
        ui.m_statusWidget->setIdle( usefulnessText );

        //QStringList missingSymbols = QStringList() << btInfo->usefulFilesWithMissingSymbols();
        if( btParser->backtraceUsefulness() != BacktraceParser::ReallyUseful )
        {
            ui.m_extraDetailsLabel->setVisible( true );
            ui.m_extraDetailsLabel->setText( i18n( "The crash information lacks of some important details.<br />Please read <link url='%1'>How to create useful crash reports</link> in order to know how to get an useful backtrace.<br />After you install the needed packages you can click the \"Reload Crash Information\" button ", QLatin1String("http://techbase.kde.org/Development/Tutorials/Debugging/How_to_create_useful_crash_reports") ) );
            ui.m_reloadBacktraceButton->setEnabled( true );

            #if 0
            if (!missingSymbols.isEmpty() ) //Detected missing symbols
            {
                QString details = i18nc("order", "You need to install the debug symbols for the following package(s):");
                Q_FOREACH( const QString & pkg, missingSymbols)
                {
                    details += "<i>"+ pkg + "</i> ";
                }
                ui.m_extraDetailsLabel->setText( ui.m_extraDetailsLabel->text() + "<br /><br />" + details );
            }
            #endif
        }
        
        ui.m_copyButton->setEnabled( true );
        ui.m_saveButton->setEnabled( true );
    }
    else if( crashInfo->getBacktraceState() == CrashInfo::Failed )
    {
        m_usefulnessMeter->setUsefulness( BacktraceParser::Useless );
        
        ui.m_statusWidget->setIdle( i18n("The crash information could not be generated" ) );
        
        ui.m_backtraceEdit->setPlainText( i18n("The crash information could not be generated" ) );
        
        ui.m_extraDetailsLabel->setVisible( true );
        ui.m_extraDetailsLabel->setText( i18n( "You need to install the debug symbols package for this application<br />Please read <link url='%1'>How to create useful crash reports</link> in order to know how to get an useful backtrace.<br />After you install the needed packages you can click the \"Reload Crash Information\" button ", QLatin1String("http://techbase.kde.org/Development/Tutorials/Debugging/How_to_create_useful_crash_reports") ) );
        ui.m_reloadBacktraceButton->setEnabled( true );
    }
    else if( crashInfo->getBacktraceState() == CrashInfo::DebuggerFailed )
    {
        m_usefulnessMeter->setUsefulness( BacktraceParser::Useless );
        
        ui.m_statusWidget->setIdle( i18n("The debugger application is missing or could not be launched" ));
        
        ui.m_backtraceEdit->setPlainText( i18n("The crash information could not be generated" ));
        ui.m_extraDetailsLabel->setVisible( true );
        ui.m_extraDetailsLabel->setText( i18n("You need to install the debugger package (<i>%1</i>) and click the \"Reload Crash Information\" button", crashInfo->getDebugger() ) );
        ui.m_reloadBacktraceButton->setEnabled( true );
    }
    
    emit stateChanged();
}

void GetBacktraceWidget::copyClicked()
{
    ui.m_backtraceEdit->selectAll();
    ui.m_backtraceEdit->copy();
}

void GetBacktraceWidget::saveClicked()
{
    if ( crashInfo->getCrashConfig()->safeMode() )
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
        QString defname = crashInfo->getCrashConfig()->execName() + '-' + QDate::currentDate().toString("yyyyMMdd") + ".kcrash";
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
