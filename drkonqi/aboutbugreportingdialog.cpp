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
    setWindowIcon( KIcon("help-hint") );
    setCaption( i18n("About Bug Reporting") );
    setModal( true );
    
    setButtons( KDialog::Close );
    setDefaultButton( KDialog::Close );
        
    m_textBrowser = new KTextBrowser( this );
    m_textBrowser->setMinimumSize( QSize(500,300) );

    QString text =
    i18n("You can help us to improve this software filing a bug report.<br /><br />"
    "In order to generate an useful bug report we need to fetch some information about the crash and your system.<br /><br />"
    "We also need you to specify some information about the crash.<br /><br />"
    "<strong>Notice:</strong> You are not forced to file a bug report if you don't want to.<br />If you have never seen this dialog before and you don't know what to do you can close it"
    "<br /><br />ToDo - Bug Reporting Assistant Steps Guide")
    ;
        
    m_textBrowser->setText( text );
    
    setMainWidget( m_textBrowser );
}

AboutBugReportingDialog::~AboutBugReportingDialog()
{
    delete m_textBrowser;
}
