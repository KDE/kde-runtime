/*******************************************************************
* getbacktracewidget.cpp
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
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
#include "usefulnessmeter.h"
#include "drkonqi.h"
#include "backtracegenerator.h"
#include "backtraceparser.h"
#include "krashconf.h"

#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>

#include <kpushbutton.h>
#include <ktextbrowser.h>
#include <kicon.h>
#include <klocale.h>

GetBacktraceWidget::GetBacktraceWidget( BacktraceGenerator * generator ) :
    QWidget(),
    m_btGenerator(generator)
{
    ui.setupUi(this);

    connect( m_btGenerator, SIGNAL(done()) , this, SLOT(loadData()) );
    connect( m_btGenerator, SIGNAL(someError()) , this, SLOT(loadData()) );
    connect( m_btGenerator, SIGNAL(failedToStart()) , this, SLOT(loadData()) );
    connect( m_btGenerator, SIGNAL(newLine(QString)) , this, SLOT(backtraceNewLine(QString)) );
    
    ui.m_extraDetailsLabel->setVisible( false );
    ui.m_reloadBacktraceButton->setGuiItem( KGuiItem( i18nc("button action", "&Reload Crash Information" ),KIcon("view-refresh"), i18nc( "button explanation", "Use this button to reload the crash information (backtrace). This is useful when you have installed the proper debug symbols packages and you want to obtain a better backtrace" ), i18nc( "button explanation", "Use this button to reload the crash information (backtrace). This is useful when you have installed the proper debug symbols packages and you want to obtain a better backtrace" ) )  );
    connect( ui.m_reloadBacktraceButton, SIGNAL(clicked()), this, SLOT(regenerateBacktrace()) );

    ui.m_copyButton->setGuiItem( KGuiItem( i18nc("button action", "&Copy" ), KIcon("edit-copy"), i18nc( "button explanation", "Use this button to copy the crash information (backtrace) to the clipboard" ), i18nc( "button explanation", "Use this button to copy the crash information (backtrace) to the clipboard" ) ) );
    connect( ui.m_copyButton, SIGNAL(clicked()) , this, SLOT(copyClicked()) );
    ui.m_copyButton->setEnabled( false );

    ui.m_saveButton->setGuiItem( KGuiItem( i18nc("button action", "&Save to File" ), KIcon("document-save"), i18nc("button explanation", "Use this button to save the crash information (backtrace) to a file. This is useful if you want to take a look at it or to report the bug later" ), i18nc("button explanation", "Use this button to save the crash information (backtrace) to a file. This is useful if you want to take a look at it or to report the bug later" ) ) );
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
    m_usefulnessMeter->setState( BacktraceGenerator::Loading );
    
    ui.m_extraDetailsLabel->setVisible( false );
    ui.m_extraDetailsLabel->clear();
    
    ui.m_reloadBacktraceButton->setEnabled( false );

    ui.m_copyButton->setEnabled( false );
    ui.m_saveButton->setEnabled( false );
}

//Force backtrace generation
void GetBacktraceWidget::regenerateBacktrace()
{
    setAsLoading();
    
    if( DrKonqi::instance()->currentState() != DrKonqi::DebuggerRunning )
        m_btGenerator->start();
    else
        anotherDebuggerRunning();
        
    emit stateChanged();
}

void GetBacktraceWidget::generateBacktrace()
{
    if ( m_btGenerator->state() == BacktraceGenerator::NotLoaded )
    {
        regenerateBacktrace();    //First backtrace generation
    }
    else if ( m_btGenerator->state() == BacktraceGenerator::Loading )
    {
        setAsLoading(); //Set in loading state, the widget will catch the backtrace events anyway
        emit stateChanged();
    }
    else //*Finished* states
    {
        setAsLoading();
        emit stateChanged();
        loadData(); //Load already gathered information
    }
}
 
void GetBacktraceWidget::anotherDebuggerRunning()
{
    ui.m_backtraceEdit->setEnabled( false );
    ui.m_backtraceEdit->setPlainText( i18n("Another debugger is currently running. The crash information could not be fetched") );
    m_usefulnessMeter->setState( BacktraceGenerator::Failed );
    m_usefulnessMeter->setUsefulness( BacktraceParser::Useless );
    ui.m_statusWidget->setIdle( i18n("The crash information could not be fetched") );
    ui.m_extraDetailsLabel->setVisible( true );
    ui.m_extraDetailsLabel->setText( i18n("Another debugging process is attached to the crashed application. The DrKonqi debugger can not fetch the backtrace. Close the process and click \"Reload Crash Information\" to get it.") );
    ui.m_reloadBacktraceButton->setEnabled( true );
}

void GetBacktraceWidget::loadData()
{
    m_usefulnessMeter->setState( m_btGenerator->state() );
    
    if( m_btGenerator->state() == BacktraceGenerator::Loaded )
    {
        ui.m_backtraceEdit->setEnabled( true );
        ui.m_backtraceEdit->setPlainText( m_btGenerator->backtrace() );

        BacktraceParser * btParser = m_btGenerator->parser();
        m_usefulnessMeter->setUsefulness( btParser->backtraceUsefulness() );

        QString usefulnessText;
        switch( btParser->backtraceUsefulness() ) {
            case BacktraceParser::ReallyUseful:
                usefulnessText = i18nc("backtrace rating description", "This crash information is useful");break;
            case BacktraceParser::MayBeUseful:
                usefulnessText = i18nc("backtrace rating description", "This crash information may be useful");break;
            case BacktraceParser::ProbablyUseless:
                usefulnessText = i18nc("backtrace rating description", "This crash information is probably not really useful");break;
            case BacktraceParser::Useless:
                usefulnessText = i18nc("backtrace rating description", "This crash information is not really useful");break;
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
            ui.m_extraDetailsLabel->setText( i18n( "Please read <link url='%1'>How to create useful crash reports</link> to learn how to get a useful backtrace.<br />After you install the needed packages you can click the \"Reload Crash Information\" button ", QLatin1String("http://techbase.kde.org/Development/Tutorials/Debugging/How_to_create_useful_crash_reports") ) );

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
    else if( m_btGenerator->state() == BacktraceGenerator::Failed )
    {
        m_usefulnessMeter->setUsefulness( BacktraceParser::Useless );
        
        ui.m_statusWidget->setIdle( i18n("The crash information could not be generated" ) );
        
        ui.m_backtraceEdit->setPlainText( i18n("The crash information could not be generated" ) );
        
        ui.m_extraDetailsLabel->setVisible( true );
        ui.m_extraDetailsLabel->setText( i18n( "You need to install the debug symbols package for this application<br />Please read <link url='%1'>How to create useful crash reports</link> to learn how to get a useful backtrace.<br />After you install the needed packages you can click the \"Reload Crash Information\" button ", QLatin1String("http://techbase.kde.org/Development/Tutorials/Debugging/How_to_create_useful_crash_reports") ) );
    }
    else if( m_btGenerator->state() == BacktraceGenerator::FailedToStart )
    {
        m_usefulnessMeter->setUsefulness( BacktraceParser::Useless );
        
        ui.m_statusWidget->setIdle( i18n("The debugger application is missing or could not be launched" ));
        
        ui.m_backtraceEdit->setPlainText( i18n("The crash information could not be generated" ));
        ui.m_extraDetailsLabel->setVisible( true );
        ui.m_extraDetailsLabel->setText( i18n("You need to install the debugger package (<i>%1</i>) and click the \"Reload Crash Information\" button", DrKonqi::instance()->krashConfig()->debuggerName() ) );
    }
    
    ui.m_reloadBacktraceButton->setEnabled( true );
    emit stateChanged();
}

void GetBacktraceWidget::backtraceNewLine(const QString & line)
{
    ui.m_backtraceEdit->append(line.trimmed());
}

void GetBacktraceWidget::copyClicked()
{
    ui.m_backtraceEdit->selectAll();
    ui.m_backtraceEdit->copy();
}

void GetBacktraceWidget::saveClicked()
{
    DrKonqi::saveReport(m_btGenerator->backtrace(), this);
}
