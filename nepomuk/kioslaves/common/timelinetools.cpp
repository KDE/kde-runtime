/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#include "timelinetools.h"

#include <Soprano/Node>
#include <Soprano/Vocabulary/XMLSchema>

#include "nie.h"
#include "nfo.h"
#include "nie.h"
#include "nuao.h"

#include <Nepomuk/Query/FileQuery>
#include <Nepomuk/Query/AndTerm>
#include <Nepomuk/Query/OrTerm>
#include <Nepomuk/Query/ComparisonTerm>
#include <Nepomuk/Query/LiteralTerm>

#include <KUrl>
#include <KCalendarSystem>
#include <KGlobal>
#include <KLocale>
#include <KDebug>

#include <QtCore/QDate>
#include <QtCore/QRegExp>


namespace {
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
                const KCalendarSystem* calSystem = KGlobal::locale()->calendar();
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


Nepomuk::TimelineFolderType Nepomuk::parseTimelineUrl( const KUrl& url, QDate* date, QString* filename )
{
    kDebug() << url;

    static QRegExp s_dateRegexp( QLatin1String("\\d{4}-\\d{2}(?:-(\\d{2}))?") );

    // reset
    *date = QDate();

    const QString path = url.path(KUrl::RemoveTrailingSlash);

    if( path.isEmpty() || path == QLatin1String("/") ) {
        kDebug() << url << "is root folder";
        return RootFolder;
    }
    else if( path.startsWith( QLatin1String( "/today" ) ) ) {
        *date = QDate::currentDate();
        if ( filename )
            *filename = path.mid( 7 );
        kDebug() << url << "is today folder:" << *date;
        return DayFolder;
    }
    else if( path == QLatin1String( "/calendar" ) ) {
        kDebug() << url << "is calendar folder";
        return CalendarFolder;
    }
    else {
        QStringList sections = path.split( QLatin1String( "/" ), QString::SkipEmptyParts );
        QString dateString;
        if ( s_dateRegexp.exactMatch( sections.last() ) ) {
            dateString = sections.last();
        }
        else if ( sections.count() > 1 && s_dateRegexp.exactMatch( sections[sections.count()-2] ) ) {
            dateString = sections[sections.count()-2];
            if ( filename )
                *filename = sections.last();
        }
        else {
            kDebug() << url << "COULD NOT PARSE";
            return NoFolder;
        }

        if ( s_dateRegexp.cap( 1 ).isEmpty() ) {
            // no day -> month listing
            kDebug() << "parsing " << dateString;
            *date = QDate::fromString( dateString, QLatin1String("yyyy-MM") );
            kDebug() << url << "is month folder:" << date->month() << date->year();
            if ( date->month() > 0 && date->year() > 0 )
                return MonthFolder;
        }
        else {
            kDebug() << "parsing " << dateString;
            *date = applyRelativeDateModificators( QDate::fromString( dateString, "yyyy-MM-dd" ), url.queryItems() );
            // only in day folders we can have filenames
            kDebug() << url << "is day folder:" << *date;
            if ( date->isValid() )
                return DayFolder;
        }
    }

    return NoFolder;
}


KUrl Nepomuk::buildTimelineQueryUrl( const QDate& date )
{
    // create our range
    const Query::LiteralTerm dateFrom( QDateTime( date, QTime( 0,0,0 ) ) );
    const Query::LiteralTerm dateTo( QDateTime( date, QTime( 23, 59, 59, 999 ) ) );

    // include files modified in our date range
    Query::ComparisonTerm lastModifiedStart = Nepomuk::Vocabulary::NIE::lastModified() > dateFrom;
    Query::ComparisonTerm lastModifiedEnd = Nepomuk::Vocabulary::NIE::lastModified() < dateTo;


    // include files created (as in photos taken) in our data range
    Query::ComparisonTerm contentCreatedStart = Nepomuk::Vocabulary::NIE::contentCreated() > dateFrom;
    Query::ComparisonTerm contentCreatedEnd = Nepomuk::Vocabulary::NIE::contentCreated() < dateTo;


    // include files opened (and optionally modified) in our date range
    // TODO: also take the end of the event into account
    // TODO: via the date facet one should be able to exclude usage events since that could clutter the result
    Query::ComparisonTerm accessEventStart = Nepomuk::Vocabulary::NUAO::start() > dateFrom;
    Query::ComparisonTerm accessEventEnd = Nepomuk::Vocabulary::NUAO::start() < dateTo;
    Query::ComparisonTerm accessEventCondition = Nepomuk::Vocabulary::NUAO::involves() == ( accessEventStart && accessEventEnd );

    Query::FileQuery query(
        ( lastModifiedStart && lastModifiedEnd )
        ||
        ( contentCreatedStart && contentCreatedEnd )
        ||
        accessEventCondition.inverted() );

    query.setFileMode( Query::FileQuery::QueryFiles );

    return query.toSearchUrl();
}
