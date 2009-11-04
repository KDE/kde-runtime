/*******************************************************************
* reportinterface.h
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
* Copyright 2009    George Kiagiadakis <gkiagia@users.sourceforge.net>
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

#ifndef REPORTINTERFACE__H
#define REPORTINTERFACE__H

#include <QtCore/QObject>
#include <QtCore/QStringList>

class SystemInformation;

class BugzillaManager;
class BugReport;
class ProductMapping;

class ReportInterface : public QObject
{
    Q_OBJECT
public:
    ReportInterface(QObject *parent = 0);

    bool userCanDetail() const;
    void setUserCanDetail(bool canDetail);

    bool developersCanContactReporter() const;
    void setDevelopersCanContactReporter(bool canContact);

    QStringList firstBacktraceFunctions() const;
    void setFirstBacktraceFunctions(const QStringList & functions);

    QString backtrace() const;
    void setBacktrace(const QString & backtrace);
    
    QString title() const;
    void setTitle(const QString & text);

    void setDetailText(const QString & text);
    void setPossibleDuplicates(const QStringList & duplicatesList);

    QString generateReport(bool drKonqiStamp) const;

    BugReport newBugReportTemplate() const;
    void sendBugReport() const;

    QStringList relatedBugzillaProducts() const;
    
    bool isWorthReporting() const;
    
    //Zero means creating a new bug report
    void setAttachToBugNumber(uint);
    uint attachToBugNumber() const;

    BugzillaManager * bugzillaManager() const;
    
private Q_SLOTS:
    void sendUsingDefaultProduct() const;
    void addedToCC();
    void attachSent(int, int);

Q_SIGNALS:
    void reportSent(int);
    void sendReportError(const QString &);

private:
    bool        m_userCanDetail;
    bool        m_developersCanContactReporter;
    
    QString     m_backtrace;
    QStringList m_firstBacktraceFunctions;
    
    QString     m_reportTitle;
    QString     m_reportDetailText;
    QStringList m_possibleDuplicates;

    uint     m_attachToBugNumber;
    
    ProductMapping *    m_productMapping;
    BugzillaManager *   m_bugzillaManager;
};

#endif
