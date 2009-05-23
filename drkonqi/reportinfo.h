/*******************************************************************
* reportinfo.h
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

#ifndef REPORTINFO__H
#define REPORTINFO__H

#include <QtCore/QObject>
#include <QtCore/QStringList>
class BugzillaManager;
class BugReport;
class ProductMapping;

class ReportInfo : public QObject
{
    Q_OBJECT
public:
    ReportInfo(QObject *parent = 0);

    bool userCanDetail() const;
    void setUserCanDetail(bool canDetail);

    bool developersCanContactReporter() const;
    void setDevelopersCanContactReporter(bool canContact);

    QString reportKeywords() const;
    void setReportKeywords(const QString & keywords);

    QStringList firstBacktraceFunctions() const;
    void setFirstBacktraceFunctions(const QStringList & functions);

    QString backtrace() const;
    void setBacktrace(const QString & backtrace);
    
    void setDetailText(const QString & text);
    void setPossibleDuplicate(const QString & bug);

    QString generateReport(bool drKonqiStamp) const;

    BugReport newBugReportTemplate() const;
    void sendBugReport(BugzillaManager *bzManager) const;

private slots:
    void sendUsingDefaultProduct() const;
    void lsbReleaseFinished();

private:
    QString osString() const;

    bool        m_userCanDetail;
    bool        m_developersCanContactReporter;
    QString     m_reportKeywords;
    QString     m_backtrace;
    QStringList m_firstBacktraceFunctions;
    QString     m_userDetailText;
    QString     m_possibleDuplicate;

    QString     m_lsbRelease;
    
    ProductMapping *    m_productMapping;
};

#endif
