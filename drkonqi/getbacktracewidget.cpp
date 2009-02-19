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
#include <ktextedit.h>
#include <kicon.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <ktemporaryfile.h>
#include <klocale.h>

#include "crashinfo.h"
#include "usefulnessmeter.h"

GetBacktraceWidget::GetBacktraceWidget( CrashInfo * info ) : QWidget()
{
    crashInfo = info;
    connect( crashInfo, SIGNAL(backtraceGenerated()) , this, SLOT(backtraceGenerated()) );
    
    backtraceEdit = new KTextEdit();
    backtraceEdit->setText( i18nc("loading information, waiting", "Loading ..." ) );
    backtraceEdit->setReadOnly( true );
    
    connect( crashInfo, SIGNAL(backtraceNewData(QString)) , backtraceEdit, SLOT(append(QString)) );
    
    statusLabel = new QLabel( i18nc("loading information, waiting", "Loading ..." ) );
    usefulnessMeter = new UsefulnessMeter( this );

    extraDetailsLabel = new QLabel();
    extraDetailsLabel->setOpenExternalLinks( true );
    extraDetailsLabel->setWordWrap( true );
    
    extraDetailsLabel->setVisible( false );
    
    installDebugButton =  new KPushButton( KIcon("list-add"), i18nc("button action", "&Install Missing Packages" ) );
    installDebugButton->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
    installDebugButton->setVisible( false );
    
    reloadBacktraceButton = new KPushButton( KIcon("view-refresh"), i18nc("button action", "&Reload Crash Information" ) );
    reloadBacktraceButton->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
    reloadBacktraceButton->setVisible( false );
    connect( reloadBacktraceButton, SIGNAL(clicked()), this, SLOT(regenerateBacktrace()) );

    copyButton = new KPushButton( KIcon("edit-copy") ,i18nc("button action", "&Copy" ));
    connect( copyButton, SIGNAL(clicked()) , this, SLOT(copyClicked()) );
    copyButton->setEnabled( false );

    saveButton = new KPushButton( KIcon("document-save-as"), i18nc("button action", "&Save" ));
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
    layout->setSpacing( 2 );
    layout->addLayout( headerLayout );
    layout->addWidget( backtraceEdit );
    layout->addLayout( backtraceButtons );
    layout->addLayout( detailsLayout );

    setLayout(layout);
     
}

void GetBacktraceWidget::regenerateBacktrace()
{
    backtraceEdit->setText( i18nc("loading information, wait", "Loading ... " ));
    backtraceEdit->setEnabled( false );
    
    statusLabel->setText( i18nc("loading information, wait", "Loading crash information ... (this will take some time)" ));
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
            extraDetailsLabel->setText( i18n("The crash information lacks of some important details.<br />You may need to read <a href=\"http://techbase.kde.org/Development/Tutorials/Debugging/How_to_create_useful_crash_reports\">How to create useful crash reports</a> in order to know how to get an useful backtrace.<br />After you install the needed packages you can click the \"Reload Crash Information\" button "));
            reloadBacktraceButton->setVisible( true );
            //installDebugButton->setVisible( true );
            
            if (!missingSymbols.isEmpty() ) //Detected missing symbols
            {
                QString details = i18nc("order", "You need to install the debug symbols for the following package(s):");
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
        
        statusLabel->setText( i18n("The crash information could not be generated" ) );
        
        backtraceEdit->setText( i18n("The crash information could not be generated" ) );
        
        extraDetailsLabel->setVisible( true );
        extraDetailsLabel->setText( i18n("You need to install the debug symbols package for this application<br />Please read <a href=\"http://techbase.kde.org/Development/Tutorials/Debugging/How_to_create_useful_crash_reports\">How to create useful crash reports</a> in order to know how to get an useful backtrace.<br />After you install the needed packages you can click the \"Reload Crash Information\" button ") );
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
        
        statusLabel->setText( i18n("The debugger application is missing or could not be launched" ));
        backtraceEdit->setText( i18n("The crash information could not be generated" ));
        extraDetailsLabel->setVisible( true );
        extraDetailsLabel->setText( i18n("You need to install the debugger package (<i>%1</i>) and click the \"Reload Crash Information\" button", crashInfo->getDebugger() ) );
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
