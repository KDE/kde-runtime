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

#include <QtGui/QApplication>
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
    
    ui.m_reloadBacktraceButton->setGuiItem( KGuiItem( i18nc("button action", "&Reload Crash Information" ),KIcon("view-refresh") )  );
    connect( ui.m_reloadBacktraceButton, SIGNAL(clicked()), this, SLOT(regenerateBacktrace()) );

    ui.m_copyButton->setGuiItem( KGuiItem( i18nc("button action", "&Copy" ), KIcon("edit-copy")  ) );
    connect( ui.m_copyButton, SIGNAL(clicked()) , this, SLOT(copyClicked()) );
    ui.m_copyButton->setEnabled( false );

    ui.m_saveButton->setGuiItem( KGuiItem( i18nc("button action", "&Save" ), KIcon("document-save-as") ) );
    connect( ui.m_saveButton, SIGNAL(clicked()) , this, SLOT(saveClicked()) );
    ui.m_saveButton->setEnabled( false );
    
}

void GetBacktraceWidget::regenerateBacktrace()
{
    ui.m_backtraceEdit->setText( i18nc("loading information, wait", "Loading ... " ));
    //ui.m_backtraceEdit->setEnabled( false );
    
    ui.m_statusLabel->setText( i18nc("loading information, wait", "Loading crash information ... (this will take some time)" ));
    ui.m_usefulnessMeter->setUsefulness( BacktraceInfo::Unuseful );
    ui.m_usefulnessMeter->setState( CrashInfo::Loading );
    
    ui.m_extraDetailsLabel->setVisible( false );
    ui.m_extraDetailsLabel->setText( "" );
    
    ui.m_reloadBacktraceButton->setEnabled( false );

    ui.m_copyButton->setEnabled( false );
    ui.m_saveButton->setEnabled( false );
                
    QApplication::setOverrideCursor(Qt::BusyCursor);
    
    crashInfo->generateBacktrace();
    
    // bool canChangePage = crashInfo->getBacktraceState() != CrashInfo::NonLoaded &&  crashInfo->getBacktraceState() != CrashInfo::Loading;
    emit setBusy();
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
    ui.m_usefulnessMeter->setState( crashInfo->getBacktraceState() );
    
    if( crashInfo->getBacktraceState() == CrashInfo::Loaded )
    {
        ui.m_backtraceEdit->setEnabled( true );
        ui.m_backtraceEdit->setText( crashInfo->getBacktraceOutput() );
        BacktraceInfo * btInfo = crashInfo->getBacktraceInfo();
        
        ui.m_usefulnessMeter->setUsefulness( btInfo->getUsefulness() );
        ui.m_statusLabel->setText( btInfo->getUsefulnessString() );
        
        QStringList missingSymbols = btInfo->usefulFilesWithMissingSymbols();
        
        if( btInfo->getUsefulness() != BacktraceInfo::ReallyUseful )
        {
            ui.m_extraDetailsLabel->setVisible( true );
            ui.m_extraDetailsLabel->setText( QString("The crash information lacks of some important details.<br />Please read <a href=\"http://techbase.kde.org/Development/Tutorials/Debugging/How_to_create_useful_crash_reports\">How to create useful crash reports</a> in order to know how to get an useful backtrace.<br />After you install the needed packages you can click the \"Reload Crash Information\" button "));
            ui.m_reloadBacktraceButton->setEnabled( true );
            
            if (!missingSymbols.isEmpty() ) //Detected missing symbols
            {
                QString details = i18nc("order", "You need to install the debug symbols for the following package(s):");
                Q_FOREACH( const QString & pkg, missingSymbols)
                {
                    details += "<i>"+ pkg + "</i> ";
                }
                ui.m_extraDetailsLabel->setText( ui.m_extraDetailsLabel->text() + "<br /><br />" + details );
            }
        }
        
        ui.m_copyButton->setEnabled( true );
        ui.m_saveButton->setEnabled( true );
    }
    else if( crashInfo->getBacktraceState() == CrashInfo::Failed )
    {
        ui.m_usefulnessMeter->setUsefulness( BacktraceInfo::Unuseful );
        
        ui.m_statusLabel->setText( i18n("The crash information could not be generated" ) );
        
        ui.m_backtraceEdit->setText( i18n("The crash information could not be generated" ) );
        
        ui.m_extraDetailsLabel->setVisible( true );
        ui.m_extraDetailsLabel->setText( QString("You need to install the debug symbols package for this application<br />Please read <a href=\"http://techbase.kde.org/Development/Tutorials/Debugging/How_to_create_useful_crash_reports\">How to create useful crash reports</a> in order to know how to get an useful backtrace.<br />After you install the needed packages you can click the \"Reload Crash Information\" button ") );
        ui.m_reloadBacktraceButton->setEnabled( true );
        //TODO
        //labelUsefulness->setText("The backtrace lacks some important information.<br /><br />You need to install the following packages:<br /><i>libqt4-dbg kdelibs5-dbg kdebase-workspace-dbg</i>");
        //You need to read <a href=\"http://techbase.kde.org/Development/Tutorials/Debugging/How_to_create_useful_crash_reports\">How to create useful crash reports</a> in order to know how to get an useful backtrace
        
    }
    else if( crashInfo->getBacktraceState() == CrashInfo::DebuggerFailed )
    {
        ui.m_usefulnessMeter->setUsefulness( BacktraceInfo::Unuseful );
        
        ui.m_statusLabel->setText( i18n("The debugger application is missing or could not be launched" ));
        ui.m_backtraceEdit->setText( i18n("The crash information could not be generated" ));
        ui.m_extraDetailsLabel->setVisible( true );
        ui.m_extraDetailsLabel->setText( i18n("You need to install the debugger package (<i>%1</i>) and click the \"Reload Crash Information\" button", crashInfo->getDebugger() ) );
        ui.m_reloadBacktraceButton->setEnabled( true );
    }
    
    bool canChange = crashInfo->getBacktraceState() != CrashInfo::NonLoaded &&  
        crashInfo->getBacktraceState() != CrashInfo::Loading;
    
    emit setIdle( canChange );
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
        QString defname = crashInfo->getCrashConfig()->execName() + "-" + QDate::currentDate().toString("yyyyMMdd") + ".kcrash";
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
