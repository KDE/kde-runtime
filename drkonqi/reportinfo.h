/*******************************************************************
* reportinfo.h
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

#ifndef REPORTINFO__H
#define REPORTINFO__H

#include <QtCore/QString>

class BugzillaManager;
class BugReport;

class ReportInfo
{
  public:
    ReportInfo();
    ~ReportInfo();

    bool getUserCanDetail() const { return m_userCanDetail; }
    void setUserCanDetail( bool canDetail ) { m_userCanDetail = canDetail; }
    
    bool getUserIsWillingToHelp() const { return m_userIsWillingToHelp; }
    void setUserIsWillingToHelp ( bool isWillingToHelp ) { m_userIsWillingToHelp = isWillingToHelp; }

    BugzillaManager * getBZ() const { return m_bugzilla; }
    BugReport * getReport() const { return m_report; }

    void setDetailText( const QString & text ) { m_userDetailText = text; }
    void setPossibleDuplicate( const QString & bug ) { m_possibleDuplicate = bug; }

    QString generateReportTemplate( bool bugzilla = false ) const;
    void fillReportFields();
    void setDefaultProductComponent();
    
    void sendBugReport();

  private:
    //Information about libraries and OS
    QString getKDEVersion() const;
    QString getQtVersion() const;
    QString getOS() const;
    QString getLSBRelease() const;

    mutable QString     m_OS;
    mutable QString     m_LSBRelease;

    bool                m_userCanDetail;
    bool                m_userIsWillingToHelp;
    
    BugzillaManager *   m_bugzilla;
    BugReport *         m_report;

    QString             m_userDetailText;
    QString             m_possibleDuplicate;
};

#endif
