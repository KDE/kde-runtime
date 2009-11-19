/*
   Copyright 2009  Sebastian Trueg <trueg@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "kio_timeline.h"

#include <KUrl>
#include <kio/global.h>
#include <klocale.h>
#include <kio/job.h>
#include <KUser>
#include <KDebug>
#include <KLocale>
#include <kio/netaccess.h>
#include <KCalendarSystem>
#include <KUser>
#include <KComponentData>

#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/XMLSchema>
#include <Soprano/QueryResultIterator>
#include <Soprano/Model>
#include <Soprano/Node>

#include <QtCore/QDate>
#include <QCoreApplication>

#include <sys/types.h>
#include <unistd.h>

using namespace KIO;


namespace {
    const char* DATEFORMATSTART("yyyy-MM-ddT00:00:00.000Z");
    const char* DATEFORMATEND("yyyy-MM-ddT23:59:59.999Z");

    KUrl buildQueryUrl( const QDate& date )
    {
        QString dateFrom = date.toString(DATEFORMATSTART);
        QString dateTo = date.toString(DATEFORMATEND);

        QString query = QString("select distinct ?r where { "
                                "?r a nfo:FileDataObject . "
                                "{"
                                "  ?r nie:lastModified ?date . "
                                "} "
#ifdef NEPOMUK_TIMELINE_WITH_NTAO
                                "UNION "
                                "{"
                                "  ?de a ?det . ?det rdfs:subClassOf ntao:DataObjectEvent . "
                                "  ?de ntao:timestamp ?date . "
                                "  ?de ntao:dataObject ?r . "
                                "} "
#endif
                                "FILTER(?date > '%3'^^%5 && ?date < '%4'^^%5) . "
                                "OPTIONAL { ?r2 a nfo:Folder . FILTER(?r=?r2) . } . FILTER(!BOUND(?r2)) . "
                                "}")
                        .arg(dateFrom)
                        .arg(dateTo)
                        .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::XMLSchema::dateTime()));
        KUrl url("nepomuksearch:/");
        url.addQueryItem( "sparql", query );
        return url;
    }

    KIO::UDSEntry createFolderUDSEntry( const QString& name, const QString& displayName, const QDate& date )
    {
        KIO::UDSEntry uds;
        QDateTime dt( date, QTime(0, 0, 0) );
        kDebug() << dt;
        uds.insert( KIO::UDSEntry::UDS_NAME, name );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, displayName );
        uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1( "inode/directory" ) );
        uds.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, dt.toTime_t() );
        uds.insert( KIO::UDSEntry::UDS_CREATION_TIME, dt.toTime_t() );
        uds.insert( KIO::UDSEntry::UDS_ACCESS, 0700 );
        uds.insert( KIO::UDSEntry::UDS_USER, KUser().loginName() );
        return uds;
    }
}


Nepomuk::TimelineProtocol::TimelineProtocol( const QByteArray& poolSocket, const QByteArray& appSocket )
    : KIO::ForwardingSlaveBase( "timeline", poolSocket, appSocket )
{
    kDebug();
}


Nepomuk::TimelineProtocol::~TimelineProtocol()
{
    kDebug();
}


void Nepomuk::TimelineProtocol::listDir( const KUrl& url )
{
    kDebug() << url;
    if( url.path() == QLatin1String("/") ) {
        // Today
        // ------------------------------------
        KIO::UDSEntry uds = createFolderUDSEntry( QLatin1String("today"), i18n("Today"), QDate::currentDate() );
//        uds.insert( KIO::UDSEntry::UDS_URL, buildQueryUrl( QDate::currentDate() ).url() );
        listEntry( uds, false );

        // Calendar
        // ------------------------------------
        listEntry( createFolderUDSEntry( QLatin1String("calendar"), i18n("Calendar"), QDate::currentDate() ), false );

        listEntry( KIO::UDSEntry(), true );
        finished();
    }
    else if( url.path().startsWith( QLatin1String("/calendar") ) ) {
        QString ru( url.path().mid( 9 ) );
        if( ru.isEmpty() ) {
            listThisYearsMonths();
            // TODO: add entry for previous years
            listEntry( KIO::UDSEntry(), true );
            finished();
        }
        else {
            kDebug() << ru << url.query();
            QStringList terms = ru.split('/', QString::SkipEmptyParts );
            kDebug() << terms;
            if( terms.count() == 1 ) {
                QDate date = QDate::fromString( terms[0], QLatin1String("yyyy-MM") );
                QDate newDate = date;
                QMap<QString,QString> queryItems = url.queryItems();
                QString dayKey = QLatin1String("relDays");
                QString weekKey = QLatin1String("relWeeks");
                QString monthKey = QLatin1String("relMonths");
                QString yearKey = QLatin1String("relYears");
                bool ok = false;

                if (queryItems.contains(yearKey)) {
                    int relYears = queryItems[yearKey].toInt(&ok);
                    if (ok) {
                        newDate = date.addYears(relYears);
                    }
                } else if (queryItems.contains(monthKey)) {
                    int relMonths = queryItems[monthKey].toInt(&ok);
                    if (ok) {
                        newDate = date.addMonths(relMonths);
                    }
                } else if (queryItems.contains(weekKey)) {
                    int relWeeks = queryItems[weekKey].toInt(&ok);
                    if (ok) {
                        const KCalendarSystem * calSystem = KGlobal::locale()->calendar();
                        newDate = date.addDays(relWeeks * calSystem->daysInWeek(date));
                    }
                } else if (queryItems.contains(dayKey)) {
                    int relDays = queryItems[dayKey].toInt(&ok);
                    if (ok) {
                        newDate = date.addDays(relDays);
                    }
                }

                if (ok && newDate <= QDate::currentDate()) {
                    KUrl newUrl;
                    newUrl.setScheme(QLatin1String("timeline"));
                    newUrl.setPath(QString("/calendar/%1").arg(newDate.toString("yyyy-MM")));
                    kDebug() << newUrl;
                    redirection(newUrl);
                    finished();
                }
                else if( date.isValid() ) {
                    kDebug() << date;
                    listDays( date.month(), date.year() );
                    listEntry( KIO::UDSEntry(), true );
                    finished();
                }
                else {
                    error( ERR_MALFORMED_URL, url.url() );
                }
            }
            else {
                ForwardingSlaveBase::listDir( url );
            }
        }
    }
    else {
        ForwardingSlaveBase::listDir( url );
    }
}


void Nepomuk::TimelineProtocol::mkdir( const KUrl &url, int permissions )
{
    Q_UNUSED(permissions);
    error( ERR_UNSUPPORTED_ACTION, url.prettyUrl() );
}


void Nepomuk::TimelineProtocol::get( const KUrl& url )
{
    kDebug() << url;
    ForwardingSlaveBase::get( url );
}


void Nepomuk::TimelineProtocol::put( const KUrl& url, int permissions, KIO::JobFlags flags )
{
    kDebug();
    Q_UNUSED( url );
    Q_UNUSED( permissions );
    Q_UNUSED( flags );

    error( ERR_UNSUPPORTED_ACTION, url.prettyUrl() );
}


void Nepomuk::TimelineProtocol::copy( const KUrl& src, const KUrl& dest, int permissions, KIO::JobFlags flags )
{
    Q_UNUSED(src);
    Q_UNUSED(dest);
    Q_UNUSED(permissions);
    Q_UNUSED(flags);

    error( ERR_UNSUPPORTED_ACTION, src.prettyUrl() );
}


void Nepomuk::TimelineProtocol::rename( const KUrl& src, const KUrl& dest, KIO::JobFlags flags )
{
    Q_UNUSED(src);
    Q_UNUSED(dest);
    Q_UNUSED(flags);

    error( ERR_UNSUPPORTED_ACTION, src.prettyUrl() );
}


void Nepomuk::TimelineProtocol::del( const KUrl& url, bool isfile )
{
    ForwardingSlaveBase::del( url, isfile );
}


void Nepomuk::TimelineProtocol::mimetype( const KUrl& url )
{
    ForwardingSlaveBase::mimetype( url );
}


void Nepomuk::TimelineProtocol::stat( const KUrl& url )
{
    ForwardingSlaveBase::stat( url );
}


// only used for the queries
bool Nepomuk::TimelineProtocol::rewriteUrl( const KUrl& url, KUrl& newURL )
{
    if( url.path() == QLatin1String("/today") ) {
        newURL = buildQueryUrl( QDate::currentDate() );
        kDebug() << url << newURL;
        return true;
    }
    else {
        QString dateString = url.path().section('/', -1, -1, QString::SectionSkipEmpty );
        QDate date = QDate::fromString( dateString, "yyyy-MM-dd" );
        if( date.isValid() ) {
            newURL = buildQueryUrl( date );
            kDebug() << url << newURL;
            return true;
        }
        else {
            kDebug() << url << "failed.";
            return false;
        }
    }
}


void Nepomuk::TimelineProtocol::prepareUDSEntry( KIO::UDSEntry& entry,
                                                 bool listing ) const
{
    kDebug() << entry.stringValue( KIO::UDSEntry::UDS_NEPOMUK_URI) << entry.stringValue( KIO::UDSEntry::UDS_MIME_TYPE) << listing;
    ForwardingSlaveBase::prepareUDSEntry( entry, listing );
}


void Nepomuk::TimelineProtocol::listMonth( int month, int year )
{
    kDebug() << month << year;
    QString dateString
        = KGlobal::locale()->calendar()->formatDate( QDate(year, month, 1),
                                                     i18nc("Month and year used in a tree above the actual days. "
                                                           "Have a look at http://api.kde.org/4.x-api/kdelibs-"
                                                           "apidocs/kdecore/html/classKCalendarSystem.html#a560204439a4b670ad36c16c404f292b4 "
                                                           "to see which variables you can use and ask kde-i18n-doc@kde.org if you have "
                                                           "problems understanding how to translate this",
                                                           "%B %Y") );
    listEntry( createFolderUDSEntry( QDate(year, month, 1).toString(QLatin1String("yyyy-MM")),
                                     dateString,
                                     QDate(year, month, 1) ),
                                     false );
}


void Nepomuk::TimelineProtocol::listDays( int month, int year )
{
    kDebug() << month << year;
    const int days = KGlobal::locale()->calendar()->daysInMonth( QDate( year, month, 1 ) );
    for( int day = 1; day <= days; ++day ) {
        QDate date(year, month, day);
        if( date <= QDate::currentDate() ) {
            KIO::UDSEntry uds = createFolderUDSEntry( date.toString("yyyy-MM-dd"),
                                                      KGlobal::locale()->formatDate( date, KLocale::FancyLongDate ),
                                                      date );
//            uds.insert( KIO::UDSEntry::UDS_URL, buildQueryUrl( date ).url() );
            listEntry( uds, false );
        }
    }
}


void Nepomuk::TimelineProtocol::listThisYearsMonths()
{
    kDebug();
    int currentMonth = QDate::currentDate().month();
    for( int month = 1; month <= currentMonth; ++month ) {
        listMonth( month, QDate::currentDate().year() );
    }
}


void Nepomuk::TimelineProtocol::listPreviousYears()
{
    kDebug();
    // TODO: list years before this year that have files
    // Using a query like: "select ?date where { ?r a nfo:FileDataObject . ?r nie:lastModified ?date . } ORDER BY ?date LIMIT 1" (this would have to be cached)
}


extern "C"
{
    KDE_EXPORT int kdemain( int argc, char **argv )
    {
        // necessary to use other kio slaves
        KComponentData( "kio_timeline" );
        QCoreApplication app( argc, argv );

        kDebug(7102) << "Starting tags slave " << getpid();

        if (argc != 4) {
            kError() << "Usage: kio_timeline protocol domain-socket1 domain-socket2";
            exit(-1);
        }

        Nepomuk::TimelineProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();

        kDebug(7102) << "Timeline slave Done";

        return 0;
    }
}

#include "kio_timeline.moc"
