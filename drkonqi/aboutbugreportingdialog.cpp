/*******************************************************************
* aboutbugreportingdialog.cpp
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

#include "aboutbugreportingdialog.h"

#include <klocale.h>
#include <ktextbrowser.h>

AboutBugReportingDialog::AboutBugReportingDialog(QWidget * parent):
    KDialog(parent)
{
    setWindowIcon( KIcon( "help-hint" ) );
    setCaption( i18nc( "title", "About Bug Reporting" ) );
    setModal( true );
    
    setButtons( KDialog::Close );
    setDefaultButton( KDialog::Close );
        
    m_textBrowser = new KTextBrowser( this );
    m_textBrowser->setMinimumSize( QSize(500,300) );

    QString text =
    
    //TODO write this and divide it to small i18n calls
    i18n(
    "<title>Information about bug reporting</title>"
    "<para>You can help us improve this software by filing a bug report.</para>"
    "<para>Note: It is safe to close this dialog. If you do not want to, you do not have to file a bug report.</para>"
    "<para>In order to generate a useful bug report we need some information about both the crash and your system.</para>"
    "<title>Bug Reporting Assistant Steps Guide</title>"
    "<subtitle>Crash Information</subtitle>"
    "<subtitle>What do you know about the crash?</subtitle>"
    "<subtitle>Conclusions</subtitle>"
    "<title>Reporting to bugs.kde.org</title>"
    );
        
    m_textBrowser->setText( text );
    
    setMainWidget( m_textBrowser );
}

AboutBugReportingDialog::~AboutBugReportingDialog()
{
    delete m_textBrowser;
}
