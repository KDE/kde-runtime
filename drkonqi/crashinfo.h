/*******************************************************************
* crashinfo.h
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

#ifndef CRASHINFO__H
#define CRASHINFO__H

#include "krashconf.h"
#include "bugzillalib.h"
#include <kdeversion.h>

class CrashInfo : public QObject
{
    Q_OBJECT
    
  public:
    
    CrashInfo();
    ~CrashInfo();

    const KrashConfig * getCrashConfig(); //TODO remove me

    const QString getCrashTitle();
    
    //Information about the user condition
    void setUserCanDetail( bool canDetail ) { m_userCanDetail = canDetail; }
    void setUserCanReproduce ( bool canReproduce ) { m_userCanReproduce = canReproduce; }
    void setUserIsWillingToHelp ( bool isWillingToHelp ) { m_userIsWillingToHelp = isWillingToHelp; }
    
    bool getUserCanDetail() { return m_userCanDetail; }
    bool getUserCanReproduce() { return m_userCanReproduce; }
    bool getUserIsWillingToHelp() { return m_userIsWillingToHelp; }
    
    //Information about the crashed app and OS
    QString getKDEVersion() { return KDE::versionString(); } 
    QString getQtVersion() { return qVersion(); }
    QString getOS();
    QString getLSBRelease();
    
    QString getProductName();
    QString getProductVersion();
    
    //Information and methods about a possible bug reporting
    bool isKDEBugzilla();
    bool isReportMail();
    QString getReportLink();
    
    QString generateReportTemplate( bool bugzilla = false );
    
    BugzillaManager * getBZ() { return m_bugzilla; };
    
    //Future bug report data & functions
    BugReport * getReport() { return m_report; }
    
    void setDetailText( QString text ) { m_userDetailText = text; }
    void setReproduceText( QString text ) { m_userReproduceText = text; }
    void setPossibleDuplicate( QString bug ) { m_possibleDuplicate = bug; }
    
    void fillReportFields();
    
    void commitBugReport();
    
  private:

    bool                m_userCanDetail;
    bool                m_userCanReproduce;
    bool                m_userIsWillingToHelp;
  
    QString             m_OS;
    QString             m_LSBRelease;
    
    BugzillaManager *   m_bugzilla;
    BugReport *         m_report;
    
    QString             m_userDetailText;
    QString             m_userReproduceText;
    QString             m_possibleDuplicate;
};

#endif
