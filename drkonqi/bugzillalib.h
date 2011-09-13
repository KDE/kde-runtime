/*******************************************************************
* bugzillalib.h
* Copyright  2009, 2011   Dario Andres Rodriguez <andresbajotierra@gmail.com>
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

#ifndef BUGZILLALIB__H
#define BUGZILLALIB__H

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QStringList>

#include <QtXml/QDomDocument>

namespace KIO { class Job; }
class KJob;
class QString;
class QByteArray;

//Typedefs for Bug Report Listing
typedef QMap<QString, QString>   BugMap; //Report basic fields map
typedef QList<BugMap>           BugMapList; //List of reports

//Main bug report data, full fields + comments
class BugReport
{
public:
    BugReport() {
        m_isValid = false;
    }

    void setBugNumber(const QString & value) {
        setData("bug_id", value);
    }
    QString bugNumber() const {
        return getData("bug_id");
    }
    int bugNumberAsInt() const {
        return getData("bug_id").toInt();
    }

    void setShortDescription(const QString & value) {
        setData("short_desc", value);
    }
    QString shortDescription() const {
        return getData("short_desc");
    }

    void setProduct(const QString & value) {
        setData("product", value);
    }
    QString product() const {
        return getData("product");
    }

    void setComponent(const QString & value) {
        setData("component", value);
    }
    QString component() const {
        return getData("component");
    }

    void setVersion(const QString & value) {
        setData("version", value);
    }
    QString version() const {
        return getData("version");
    }

    void setOperatingSystem(const QString & value) {
        setData("op_sys", value);
    }
    QString operatingSystem() const {
        return getData("op_sys");
    }

    void setPlatform(const QString & value) {
        setData("rep_platform", value);
    }
    QString platform() const {
        return getData("rep_platform");
    }
    
    void setBugStatus(const QString & value) {
        setData("bug_status", value);
    }
    QString bugStatus() const {
        return getData("bug_status");
    }

    void setResolution(const QString & value) {
        setData("resolution", value);
    }
    QString resolution() const {
        return getData("resolution");
    }

    void setPriority(const QString & value) {
        setData("priority", value);
    }
    QString priority() const {
        return getData("priority");
    }

    void setBugSeverity(const QString & value) {
        setData("bug_severity", value);
    }
    QString bugSeverity() const {
        return getData("bug_severity");
    }

    void setDescription(const QString & desc) {
        m_commentList.insert(0, desc);
    }
    QString description() const {
        return m_commentList.at(0);
    }

    void setComments(const QStringList & comm) {
        m_commentList.append(comm);
    }
    QStringList comments() const {
        return m_commentList.mid(1);
    }

    void setMarkedAsDuplicateOf(const QString & dupID) {
        setData("dup_id", dupID);
    }
    QString markedAsDuplicateOf() const {
        return getData("dup_id");
    }

    void setVersionFixedIn(const QString & dupID) {
        setData("cf_versionfixedin", dupID);
    }
    QString versionFixedIn() const {
        return getData("cf_versionfixedin");
    }
    
    void setValid(bool valid) {
        m_isValid = valid;
    }
    bool isValid() const {
        return m_isValid;
    }

private:
    void setData(const QString & key, const QString & val) {
        m_dataMap.insert(key, val);
    }
    QString getData(const QString & key) const {
        return m_dataMap.value(key);
    }

    bool        m_isValid;

    BugMap      m_dataMap;
    QStringList m_commentList;
};

//XML parser that creates a BugReport object
class BugReportXMLParser
{
public:
    BugReportXMLParser(const QByteArray &);

    BugReport parse();

    bool isValid() const {
        return m_valid;
    }

private:
    QString getSimpleValue(const QString &);

    bool            m_valid;
    QDomDocument    m_xml;
};

class BugListCSVParser
{
public:
    BugListCSVParser(const QByteArray&);

    bool isValid() const {
        return m_isValid;
    }

    BugMapList parse();

private:
    bool m_isValid;
    QByteArray  m_data;
};

class BugzillaManager : public QObject
{
    Q_OBJECT

public:
    BugzillaManager(QObject *parent = 0);

    /* Login methods */
    void tryLogin();
    bool getLogged() const;

    void setLoginData(const QString &, const QString &);
    QString getUsername() const;

    /* Bugzilla Action methods */
    void fetchBugReport(int, QObject * jobOwner = 0);

    void searchBugs(const QStringList & products, const QString & severity,
                    const QString & date_start, const QString & date_end , QString comment);
    
    void sendReport(BugReport);
    
    void attachTextToReport(const QString &, const QString &, const QString &, uint, const QString &);

    void addCommentToReport(const QString &, int, bool);

    void addMeToCC(int);

    void checkVersionsForProduct(const QString &);
    
    /* Misc methods */
    QString urlForBug(int bug_number) const;

    void setCustomBugtrackerUrl(const QString &);

    void stopCurrentSearch();
    
private Q_SLOTS:
    /* Slots to handle KJob::finished */
    void loginJobFinished(KJob*);
    void fetchBugJobFinished(KJob*);
    void searchBugsJobFinished(KJob*);
    void sendReportJobFinished(KJob*);
    void attachToReportJobFinished(KJob*);
    void addCommentSubJobFinished(KJob*);
    void addCommentJobFinished(KJob*);
    void addMeToCCSubJobFinished(KJob*);
    void addMeToCCJobFinished(KJob*);
    void checkVersionJobFinished(KJob*);

Q_SIGNALS:
    /* Bugzilla actions finished successfully */
    void loginFinished(bool);
    void bugReportFetched(BugReport, QObject *);
    void searchFinished(const BugMapList &);
    void reportSent(int);
    void attachToReportSent(int, int);
    void addCommentFinished(int);
    void addMeToCCFinished(int);
    void checkVersionsForProductFinished(const QStringList);

    /* Bugzilla actions had errors */
    void loginError(const QString &, const QString &);
    void bugReportError(const QString &, QObject *);
    void searchError(const QString &);
    void sendReportError(const QString &, const QString &);
    void sendReportErrorInvalidValues(); //To use default values
    void attachToReportError(const QString &, const QString &);
    void addCommentError(const QString &, const QString &);
    void addMeToCCError(const QString &, const QString &);
    void checkVersionsForProductError();

private:
    /* Private helper methods */
    QString getToken(const QString &);
    QString getErrorMessage(const QString &, bool fallBackMessage = true);

    void attachToReport(const QByteArray &, const QByteArray &);
    QByteArray generatePostDataForReport(BugReport) const;

    QString     m_username;
    QString     m_password;
    bool        m_logged;

    KIO::Job *  m_searchJob;

    QString     m_bugTrackerUrl;
};

class BugzillaUploadData
{
public:
    BugzillaUploadData(uint bugNumber);
    
    QByteArray postData() const;
    QByteArray boundary() const;
    
    void attachFile(const QString & url, const QString & description);
    void attachRawData(const QByteArray & data, const QString & filename, 
                    const QString & mimeType, const QString & description,
                    const QString & comment = QString());
    
private:
    void addPostField(const QString & name, const QString & value);
    void addPostData(const QString & name, const QByteArray & data, const QString & mimeType,
                     const QString & path);
    void finishPostRequest();
    
    QByteArray      m_boundary;
    uint            m_bugNumber;
    QByteArray      m_postData;
};

#endif
