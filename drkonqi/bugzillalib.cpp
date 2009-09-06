/*******************************************************************
* bugzillalib.cpp
* Copyright  2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
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

#include "bugzillalib.h"

#include <QtCore/QTextStream>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QRegExp>

#include <QtXml/QDomNode>
#include <QtXml/QDomNodeList>
#include <QtXml/QDomElement>
#include <QtXml/QDomNamedNodeMap>

#include <QDBusInterface>
#include <QDBusReply>

#include <KIO/Job>
#include <KUrl>
#include <KConfig>
#include <KConfigGroup>
#include <KLocale>
#include <KDebug>

static const char bugtrackerBKOBaseUrl[] = "https://bugs.kde.org/";

static const char columns[] = "bug_severity,priority,bug_status,product,short_desc"; //resolution,

//BKO URLs
static const char loginUrl[] = "index.cgi";

static const char searchUrl[] = "buglist.cgi?query_format=advanced&short_desc_type=anywordssubstr"
                            "&short_desc=%1&product=kde&product=%2&long_desc_type=anywordssubstr&"
                            "long_desc=%3&bug_file_loc_type=allwordssubstr&bug_file_loc=&keywords_"
                            "type=allwords&keywords=&emailtype1=substring&email1=&emailtype2=substr"
                            "ing&email2=&bugidtype=include&bug_id=&votes=&chfieldfrom=%4&chfieldto="
                            "%5&chfield=[Bug+creation]&chfieldvalue=&cmdtype=doit&field0-0-0=noop"
                            "&type0-0-0=noop&value0-0-0=&bug_severity=%6&order=Importance"
                            "&columnlist=%7&ctype=csv";
// short_desc, product, long_desc(possible backtraces lines), searchFrom, searchTo, severity, columnList

static const char showBugUrl[] = "show_bug.cgi?id=%1";
static const char fetchBugUrl[] = "show_bug.cgi?id=%1&ctype=xml";

static const char sendReportUrl[] = "post_bug.cgi";

BugzillaManager::BugzillaManager(QObject *parent):
        QObject(parent),
        m_logged(false),
        m_searchJob(0),
        m_fallbackManualCookie(false)
{
    m_bugTrackerUrl = bugtrackerBKOBaseUrl;
}

void BugzillaManager::setCustomBugtrackerUrl(const QString & customUrl)
{
    m_bugTrackerUrl = customUrl;
}

void BugzillaManager::setLoginData(const QString & _username, const QString & _password)
{
    m_username = _username;
    m_password = _password;
    m_logged = false;
}

void BugzillaManager::tryLogin(bool forceManualCookie)
{
    if (!m_logged) {
        QByteArray postData =
            QByteArray("GoAheadAndLogIn=1&Bugzilla_login=") +
            QUrl::toPercentEncoding(m_username) +
            QByteArray("&Bugzilla_password=") +
            QUrl::toPercentEncoding(m_password) +
            QByteArray("&log_in=Log+in");

        KIO::Job * loginJob =
            KIO::storedHttpPost(postData, KUrl(QString(m_bugTrackerUrl) + QString(loginUrl)),
                                KIO::HideProgressInfo);
        connect(loginJob, SIGNAL(finished(KJob*)) , this, SLOT(loginDone(KJob*)));

        if (!isCookieDaemonActive() || forceManualCookie) {
            kDebug() << "Cookie daemon problem, fallback to manual";
            m_fallbackManualCookie = true;
            loginJob->addMetaData("cookies", "manual"); //Use manual-set cookies
        }
        loginJob->addMetaData("content-type", "Content-Type: application/x-www-form-urlencoded");
    }
}

void BugzillaManager::loginDone(KJob* job)
{
    if (!job->error()) {
        KIO::StoredTransferJob * loginJob = (KIO::StoredTransferJob *)job;
        QByteArray response = loginJob->data();

        bool error = false;
        if (response.contains(QByteArray("The username or password you entered is not valid"))) {
            m_logged = false;
        } else {
            if (response.contains(QByteArray("Managing Your Account"))
                && response.contains(QByteArray("Log out"))) {
                
               if (m_fallbackManualCookie) {
                    processLoginCookie(loginJob);
                } else {
                    //If the cookie daemon is active + cookie not set; 
                    //or if the cookie daemon died already, force manual cookies
                    if ((isCookieDaemonActive() && !isLoginCookieSet()) || 
                            !isCookieDaemonActive()) {
                        tryLogin(true);
                        return;
                    }
                }
                
                m_logged = true;
            } else {
                m_logged = false;
                error = true;
            }
        }

        if (error) {
            emit loginError(i18nc("@info","Unknown response from the server"));
        } else {
            emit loginFinished(m_logged);
        }
    } else {
        emit loginError(job->errorString());
    }
}

void BugzillaManager::processLoginCookie(KIO::Job * job)
{
    const QStringList cookieList = (job->queryMetaData("setcookies")).split('\n');
    QString cookieString;
    if(!cookieList.isEmpty()) {
        cookieString = "Cookie: ";
        foreach(const QString &str, cookieList) {
            if(str.contains("Set-Cookie: ")) {
                const QStringList cl = str.split(' ');
                if (cl.size() >= 2) {
                    cookieString += cl.at(1);
                }
            }
        }
    }
    m_cookieString = cookieString;
}

void BugzillaManager::fetchBugReport(int bugnumber, QObject * jobOwner)
{
    KUrl url = KUrl(QString(m_bugTrackerUrl) + QString(fetchBugUrl).arg(bugnumber));

    if (!jobOwner) jobOwner = this;
    
    KIO::Job * fetchBugJob = KIO::storedGet(url, KIO::Reload, KIO::HideProgressInfo);
    fetchBugJob->setParent(jobOwner);
    connect(fetchBugJob, SIGNAL(finished(KJob*)) , this, SLOT(fetchBugReportDone(KJob*)));
    
    if (m_fallbackManualCookie && !m_cookieString.isEmpty()) {
        fetchBugJob->addMetaData("cookies", "manual");
        fetchBugJob->addMetaData("setcookies", m_cookieString);
    }
}

void BugzillaManager::fetchBugReportDone(KJob* job)
{
    if (!job->error()) {
        KIO::StoredTransferJob * fetchBugJob = (KIO::StoredTransferJob *)job;

        BugReportXMLParser * parser = new BugReportXMLParser(fetchBugJob->data());
        BugReport report = parser->parse();

        if (parser->isValid()) {
            emit bugReportFetched(report, job->parent());
        } else {
            emit bugReportError(i18nc("@info","Invalid bug report: corrupted data"), job->parent());
        }

        delete parser;
    } else {
        emit bugReportError(job->errorString(), job->parent());
    }
}

void BugzillaManager::searchBugs(QString words, const QStringList & products,
                                const QString & severity, const QString & date_start, 
                                const QString & date_end, QString comment)
{
    QString product;
    if (products.size() > 0) {
        if (products.size() == 1) {
            product = products.at(0);
        } else  {
            Q_FOREACH(const QString & p, products) {
                product += p + "&product=";
            }
            product = product.mid(0,product.size()-9);
        }
    }

    QString url = QString(m_bugTrackerUrl) +
                  QString(searchUrl).arg(words.replace(' ' , '+'), product, 
                                         comment.replace(' ' , '+'), date_start,
                                         date_end, severity, QString(columns));
    
    stopCurrentSearch();
    
    m_searchJob = KIO::storedGet(KUrl(url) , KIO::Reload, KIO::HideProgressInfo);
    connect(m_searchJob, SIGNAL(finished(KJob*)) , this, SLOT(searchBugsDone(KJob*)));
    
    if (m_fallbackManualCookie && !m_cookieString.isEmpty()) {
        m_searchJob->addMetaData("cookies", "manual");
        m_searchJob->addMetaData("setcookies", m_cookieString);
    }
}

void BugzillaManager::stopCurrentSearch()
{
    if (m_searchJob) { //Stop previous searchJob
        m_searchJob->disconnect();
        m_searchJob->kill();
        m_searchJob = 0;
    }
}

void BugzillaManager::searchBugsDone(KJob * job)
{
    if (!job->error()) {
        KIO::StoredTransferJob * searchBugsJob = (KIO::StoredTransferJob *)job;

        BugListCSVParser * parser = new BugListCSVParser(searchBugsJob->data());
        BugMapList list = parser->parse();

        if (parser->isValid()) {
            emit searchFinished(list);
        } else {
            emit searchError(i18nc("@info","Invalid bug list: corrupted data"));
        }

        delete parser;
    } else {
        emit searchError(job->errorString());
    }
    
    m_searchJob = 0;
}

void BugzillaManager::sendReport(BugReport report)
{
    QByteArray postData = generatePostDataForReport(report);

    QString url = QString(m_bugTrackerUrl) + QString(sendReportUrl);

    KIO::Job * sendJob =
        KIO::storedHttpPost(postData, KUrl(url), KIO::HideProgressInfo);

    connect(sendJob, SIGNAL(finished(KJob*)) , this, SLOT(sendReportDone(KJob*)));

    if (m_fallbackManualCookie && !m_cookieString.isEmpty()) {
        sendJob->addMetaData("cookies", "manual");
        sendJob->addMetaData("setcookies", m_cookieString);
    }
    sendJob->addMetaData("content-type", "Content-Type: application/x-www-form-urlencoded");    
}

QByteArray BugzillaManager::generatePostDataForReport(BugReport report) const
{
    QByteArray postData =
        QByteArray("product=") +
        QUrl::toPercentEncoding(report.product()) +
        QByteArray("&version=unspecified&component=") +
        QUrl::toPercentEncoding(report.component()) +
        QByteArray("&bug_severity=") +
        QUrl::toPercentEncoding(report.bugSeverity()) +
        QByteArray("&rep_platform=") +
        QUrl::toPercentEncoding(report.platform()) +
        QByteArray("&op_sys=") +
        QUrl::toPercentEncoding(report.operatingSystem()) +
        QByteArray("&priority=") +
        QUrl::toPercentEncoding(report.priority()) +
        QByteArray("&bug_status=") +
        QUrl::toPercentEncoding(report.bugStatus()) +
        QByteArray("&short_desc=") +
        QUrl::toPercentEncoding(report.shortDescription()) +
        QByteArray("&comment=") +
        QUrl::toPercentEncoding(report.description());

    return postData;
}

void BugzillaManager::sendReportDone(KJob * job)
{
    if (!job->error()) {
        KIO::StoredTransferJob * sendJob = (KIO::StoredTransferJob *)job;
        QString response = sendJob->data();

        QRegExp reg("<title>Bug (\\d+) Submitted</title>");
        int pos = reg.indexIn(response);
        if (pos != -1) {
            int bug_id = reg.cap(1).toInt();
            emit reportSent(bug_id);
        } else {
            QString error;

            QRegExp reg("<td id=\"error_msg\" class=\"throw_error\">(.+)</td>");
            response.remove('\r'); response.remove('\n');
            pos = reg.indexIn(response);
            if (pos != -1) {
                error = reg.cap(1).trimmed();
            } else {
                QString illegalPlatformMessage = "Bugzilla has suffered an internal error. "
                "Please save this page and send";
                QString illegalPlatformMessage2 = "A legal Platform was not set.";
                if (response.contains(illegalPlatformMessage) && 
                            response.contains(illegalPlatformMessage2)) {
                    emit sendReportErrorInvalidValues();
                    return;
                } else {
                    error = i18nc("@info","Unknown error");
                }
            }

            if (error.contains(QLatin1String("does not exist or you aren't authorized to"))
                || error.contains(QLatin1String("There is no component named 'general'"))) {
                emit sendReportErrorInvalidValues();
            } else {
                emit sendReportError(error);
            }
        }
    } else {
        emit sendReportError(job->errorString());
    }

}

QString BugzillaManager::urlForBug(int bug_number)
{
    return QString(m_bugTrackerUrl) + QString(showBugUrl).arg(bug_number);
}

bool BugzillaManager::isLoginCookieSet()
{
    //Check if the bugzilla cookie is set properly
    QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");
    QDBusReply<QString> reply = kded.call("listCookies", 
                                    KUrl(QString(m_bugTrackerUrl) + QString(loginUrl)).url());
    if (reply.isValid()) {
        return reply.value().contains(QLatin1String("Bugzilla_logincookie"));
    }
    return false;
}

bool BugzillaManager::isCookieDaemonActive()
{
    //Check if global cookies are activated
    KConfig cfg ("kcookiejarrc");
    KConfigGroup group = cfg.group ("Cookie Policy");
    if (group.readEntry("Cookies", false)) {
        //Check if cookies server is running
        QDBusInterface kdedInterface("org.kde.kded", "/kded", "org.kde.kded");
        QDBusReply<QStringList> reply = kdedInterface.call("loadedModules");
        if (reply.isValid()) {
            return reply.value().contains(QLatin1String("kcookiejar"));
        }
    }
    return false;
}

BugListCSVParser::BugListCSVParser(QByteArray data)
{
    m_data = data;
    m_isValid = false;
}

BugMapList BugListCSVParser::parse()
{
    BugMapList list;

    if (!m_data.isEmpty()) {
        //Parse buglist CSV
        QTextStream ts(&m_data);
        QString headersLine = ts.readLine().remove(QLatin1Char('\"')) ;   //Discard headers
        QString expectedHeadersLine = QString(columns);

        if (headersLine == (QString("bug_id,") + expectedHeadersLine)) {
            QStringList headers = expectedHeadersLine.split(',');
            int headersCount = headers.count();

            while (!ts.atEnd()) {
                BugMap bug; //bug report data map

                QString line = ts.readLine();

                //Get bug_id (always at first column)
                int bug_id_index = line.indexOf(',');
                QString bug_id = line.left(bug_id_index);
                bug.insert("bug_id", bug_id);

                line = line.mid(bug_id_index + 2);

                QStringList fields = line.split(",\"");

                for (int i = 0; i < headersCount ; i++) {
                    QString field = fields.at(i);
                    field = field.left(field.size() - 1) ;   //Remove trailing "
                    bug.insert(headers.at(i), field);
                }

                list.append(bug);
            }

            m_isValid = true;
        }
    }

    return list;
}

BugReportXMLParser::BugReportXMLParser(const QByteArray & data)
{
    m_valid = m_xml.setContent(data, true);
}

BugReport BugReportXMLParser::parse()
{
    BugReport report; //creates an invalid and empty report object

    if (m_valid) {
        //Check bug notfound
        QDomNodeList bug_number = m_xml.elementsByTagName("bug");
        QDomNode d = bug_number.at(0);
        QDomNamedNodeMap a = d.attributes();
        QDomNode d2 = a.namedItem("error");
        m_valid = d2.isNull();

        if (m_valid) {
            m_valid = true;
            report.setValid(true);

            //Get basic fields
            report.setBugNumber(getSimpleValue("bug_id"));
            report.setShortDescription(getSimpleValue("short_desc"));
            report.setProduct(getSimpleValue("product"));
            report.setComponent(getSimpleValue("component"));
            report.setVersion(getSimpleValue("version"));
            report.setOperatingSystem(getSimpleValue("op_sys"));
            report.setBugStatus(getSimpleValue("bug_status"));
            report.setResolution(getSimpleValue("resolution"));
            report.setPriority(getSimpleValue("priority"));
            report.setBugSeverity(getSimpleValue("bug_severity"));

            //Parse full content + comments
            QStringList m_commentList;
            QDomNodeList comments = m_xml.elementsByTagName("long_desc");
            for (int i = 0; i < comments.count(); i++) {
                QDomElement element = comments.at(i).firstChildElement("thetext");
                m_commentList << element.text();
            }

            report.setComments(m_commentList);

        } //isValid
    } //isValid

    return report;
}

QString BugReportXMLParser::getSimpleValue(const QString & name)   //Extract an unique tag from XML
{
    QString ret;

    QDomNodeList bug_number = m_xml.elementsByTagName(name);
    if (bug_number.count() == 1) {
        QDomNode node = bug_number.at(0);
        ret = node.toElement().text();
    }
    return ret;
}

BugReport::BugReport()
{
    m_isValid = false;
}

