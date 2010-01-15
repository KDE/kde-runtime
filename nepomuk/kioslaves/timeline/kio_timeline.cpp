/*
   Copyright 2009-2010 Sebastian Trueg <trueg@kde.org>

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

    KIO::UDSEntry createMonthUDSEntry( int month, int year )
    {
        QString dateString
            = KGlobal::locale()->calendar()->formatDate( QDate(year, month, 1),
                                                         i18nc("Month and year used in a tree above the actual days. "
                                                               "Have a look at http://api.kde.org/4.x-api/kdelibs-"
                                                               "apidocs/kdecore/html/classKCalendarSystem.html#a560204439a4b670ad36c16c404f292b4 "
                                                               "to see which variables you can use and ask kde-i18n-doc@kde.org if you have "
                                                               "problems understanding how to translate this",
                                                               "%B %Y") );
        return createFolderUDSEntry( QDate(year, month, 1).toString(QLatin1String("yyyy-MM")),
                                     dateString,
                                     QDate(year, month, 1) );
    }

    KIO::UDSEntry createDayUDSEntry( const QDate& date )
    {
        return createFolderUDSEntry( date.toString("yyyy-MM-dd"),
                                     KGlobal::locale()->formatDate( date, KLocale::FancyLongDate ),
                                     date );
    }

    QDate applyRelativeDateModificators( const QDate& date, const QMap<QString, QString>& modificators )
    {
        QDate newDate( date );
        const QString dayKey = QLatin1String("relDays");
        const QString weekKey = QLatin1String("relWeeks");
        const QString monthKey = QLatin1String("relMonths");
        const QString yearKey = QLatin1String("relYears");
        bool ok = false;

        if (modificators.contains(yearKey)) {
            int relYears = modificators[yearKey].toInt(&ok);
            if (ok) {
                newDate = newDate.addYears(relYears);
            }
        }
        if (modificators.contains(monthKey)) {
            int relMonths = modificators[monthKey].toInt(&ok);
            if (ok) {
                newDate = newDate.addMonths(relMonths);
            }
        }
        if (modificators.contains(weekKey)) {
            int relWeeks = modificators[weekKey].toInt(&ok);
            if (ok) {
                const KCalendarSystem * calSystem = KGlobal::locale()->calendar();
                newDate = newDate.addDays(relWeeks * calSystem->daysInWeek(date));
            }
        }
        if (modificators.contains(dayKey)) {
            int relDays = modificators[dayKey].toInt(&ok);
            if (ok) {
                newDate = newDate.addDays(relDays);
            }
        }
        return newDate;
    }
}


Nepomuk::TimelineProtocol::TimelineProtocol( const QByteArray& poolSocket, const QByteArray& appSocket )
    : KIO::ForwardingSlaveBase( "timeline", poolSocket, appSocket ),
      m_folderType( NoFolder ),
      m_dateRegexp( QLatin1String("\\d{4}-\\d{2}(?:-(\\d{2}))?") )
{
    kDebug();
}


Nepomuk::TimelineProtocol::~TimelineProtocol()
{
    kDebug();
}


void Nepomuk::TimelineProtocol::listDir( const KUrl& url )
{
    if ( parseUrl( url ) ) {
        switch( m_folderType ) {
        case RootFolder:
            listEntry( createFolderUDSEntry( QLatin1String("today"), i18n("Today"), QDate::currentDate() ), false );
            listEntry( createFolderUDSEntry( QLatin1String("calendar"), i18n("Calendar"), QDate::currentDate() ), false );
            listEntry( KIO::UDSEntry(), true );
            finished();
            break;

        case CalendarFolder:
            listThisYearsMonths();
            // TODO: add entry for previous years
            listEntry( KIO::UDSEntry(), true );
            finished();
            break;

        case MonthFolder:
            listDays( m_date.month(), m_date.year() );
            listEntry( KIO::UDSEntry(), true );
            finished();
            break;

        case DayFolder:
            ForwardingSlaveBase::listDir( url );
            break;

        default:
            error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
            break;
        }
    }
    else {
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
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

    if ( parseUrl( url ) && !m_filename.isEmpty() ) {
        ForwardingSlaveBase::get( url );
    }
    else {
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
    }
}


void Nepomuk::TimelineProtocol::put( const KUrl& url, int permissions, KIO::JobFlags flags )
{
    kDebug() << url;

    if ( parseUrl( url ) && !m_filename.isEmpty() ) {
        ForwardingSlaveBase::put( url, permissions, flags );
    }
    else {
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
    }
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
    kDebug() << url;
    ForwardingSlaveBase::del( url, isfile );
}


void Nepomuk::TimelineProtocol::mimetype( const KUrl& url )
{
    kDebug() << url;
    ForwardingSlaveBase::mimetype( url );
}


void Nepomuk::TimelineProtocol::stat( const KUrl& url )
{
    if ( parseUrl( url ) ) {
        switch( m_folderType ) {
        case RootFolder: {
            KIO::UDSEntry uds;
            uds.insert( KIO::UDSEntry::UDS_NAME, QString::fromLatin1( "/" ) );
            uds.insert( KIO::UDSEntry::UDS_ICON_NAME, QString::fromLatin1( "nepomuk" ) );
            uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
            uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1( "inode/directory" ) );
            statEntry( uds );
            finished();
            break;
        }

        case CalendarFolder:
            statEntry( createFolderUDSEntry( QLatin1String("calendar"), i18n("Calendar"), QDate::currentDate() ) );
            finished();
            break;

        case MonthFolder:
            statEntry( createMonthUDSEntry( m_date.month(), m_date.year() ) );
            finished();
            break;

        case DayFolder:
            if ( m_filename.isEmpty() ) {
                statEntry( createDayUDSEntry( m_date ) );
                finished();
            }
            else {
                ForwardingSlaveBase::stat( url );
            }
            break;

        default:
            error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
            break;
        }
    }
    else {
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
    }
}


// only used for the queries
bool Nepomuk::TimelineProtocol::rewriteUrl( const KUrl& url, KUrl& newURL )
{
    if ( parseUrl( url ) && m_folderType == DayFolder ) {
        newURL = buildQueryUrl( m_date );
        newURL.setPath( QLatin1String( "/" ) + m_filename );
        return true;
    }
    else {
        return false;
    }
}


void Nepomuk::TimelineProtocol::prepareUDSEntry( KIO::UDSEntry& entry,
                                                 bool listing ) const
{
    kDebug() << entry.stringValue( KIO::UDSEntry::UDS_NEPOMUK_URI) << entry.stringValue( KIO::UDSEntry::UDS_MIME_TYPE) << listing;
    ForwardingSlaveBase::prepareUDSEntry( entry, listing );
}


void Nepomuk::TimelineProtocol::listDays( int month, int year )
{
    kDebug() << month << year;
    const int days = KGlobal::locale()->calendar()->daysInMonth( QDate( year, month, 1 ) );
    for( int day = 1; day <= days; ++day ) {
        QDate date(year, month, day);
        if( date <= QDate::currentDate() ) {
            listEntry( createDayUDSEntry( date ), false );
        }
    }
}


void Nepomuk::TimelineProtocol::listThisYearsMonths()
{
    kDebug();
    int currentMonth = QDate::currentDate().month();
    for( int month = 1; month <= currentMonth; ++month ) {
        listEntry( createMonthUDSEntry( month, QDate::currentDate().year() ), false );
    }
}


void Nepomuk::TimelineProtocol::listPreviousYears()
{
    kDebug();
    // TODO: list years before this year that have files
    // Using a query like: "select ?date where { ?r a nfo:FileDataObject . ?r nie:lastModified ?date . } ORDER BY ?date LIMIT 1" (this would have to be cached)
}


bool Nepomuk::TimelineProtocol::parseUrl( const KUrl& url )
{
    kDebug() << url;

    // rest
    m_date = QDate();
    m_filename.truncate(0);

    const QString path = url.path(KUrl::RemoveTrailingSlash);

    if( path.isEmpty() || path == QLatin1String("/") ) {
        m_folderType = RootFolder;
        kDebug() << url << "is root folder";
        return true;
    }
    else if( path.startsWith( QLatin1String( "/today" ) ) ) {
        m_folderType = DayFolder;
        m_date = QDate::currentDate();
        m_filename = path.mid( 7 );
        kDebug() << url << "is today folder:" << m_date << m_filename;
        return true;
    }
    else if( path == QLatin1String( "/calendar" ) ) {
        m_folderType = CalendarFolder;
        kDebug() << url << "is calendar folder";
        return true;
    }
    else {
        QStringList sections = path.split( QLatin1String( "/" ), QString::SkipEmptyParts );
        QString dateString;
        if ( m_dateRegexp.exactMatch( sections.last() ) ) {
            dateString = sections.last();
        }
        else if ( sections.count() > 1 && m_dateRegexp.exactMatch( sections[sections.count()-2] ) ) {
            dateString = sections[sections.count()-2];
            m_filename = sections.last();
        }
        else {
            kDebug() << url << "COULD NOT PARSE";
            return false;
        }

        if ( m_dateRegexp.cap( 1 ).isEmpty() ) {
            // no day -> month listing
            m_folderType = MonthFolder;
            kDebug() << "parsing " << dateString;
            m_date = QDate::fromString( dateString, QLatin1String("yyyy-MM") );
            kDebug() << url << "is month folder:" << m_date.month() << m_date.year();
            return m_date.month() > 0 && m_date.year() > 0;
        }
        else {
            m_folderType = DayFolder;
            kDebug() << "parsing " << dateString;
            m_date = applyRelativeDateModificators( QDate::fromString( dateString, "yyyy-MM-dd" ), url.queryItems() );
            // only in day folders we can have filenames
            kDebug() << url << "is day folder:" << m_date << m_filename;
            return m_date.isValid();
        }
    }
}

extern "C"
{
    KDE_EXPORT int kdemain( int argc, char **argv )
    {
        // necessary to use other kio slaves
        KComponentData( "kio_timeline" );
        QCoreApplication app( argc, argv );

        kDebug(7102) << "Starting timeline slave " << getpid();

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
