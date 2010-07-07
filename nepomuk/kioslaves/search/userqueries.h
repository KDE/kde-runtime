/*
   Copyright (C) 2010 by Sebastian Trueg <trueg at kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _NEPOMUK_USER_QUERIES_H_
#define _NEPOMUK_USER_QUERIES_H_

#include "nepomuksearchurltools.h"

#include <KConfig>
#include <KConfigGroup>
#include <KUrl>
#include <KDirNotify>
#include <KDebug>

#include <QtCore/QStringList>


namespace Nepomuk {
    /// convert a normal query URL which can be listed to a URL which is used as location for user queries
    inline KUrl queryUrlToUserQueryUrl( const KUrl& url )
    {
        // the query URL is NOT the URL under which the query is listed!
        KUrl queryListUrl;
        queryListUrl.setProtocol( QLatin1String( "nepomuksearch" ) );
        queryListUrl.setFileName( Nepomuk::resourceUriToUdsName( url ) );
        return queryListUrl;
    }

    inline KUrl queryUrlFromUserQueryUrl( const KUrl& url )
    {
        return Nepomuk::udsNameToResourceUri( url.fileName() );
    }

    class UserQueryUrlList : public KUrl::List
    {
    public:
        UserQueryUrlList()
            : KUrl::List() {
            read();
        }

        static KUrl cleanupQueryUrl( const KUrl& url )
        {
            KUrl newUrl( url );
            newUrl.removeQueryItem( QLatin1String( "title" ) );
            newUrl.removeQueryItem( QLatin1String( "userquery" ) );
            // strangly enough removing all query items leaves an empty query
            // instead of no query
            if ( newUrl.queryItems().isEmpty() )
                newUrl.setEncodedQuery( QByteArray() );
            return newUrl;
        }

        QList<KUrl>::iterator findQueryUrl( const KUrl& url ) {
            const KUrl normalizedUrl = cleanupQueryUrl( url );
            QList<KUrl>::iterator it = begin();
            while ( it != end() ) {
                if ( cleanupQueryUrl( *it ) == normalizedUrl )
                    return it;
                ++it;
            }
            return end();
        }

        QList<KUrl>::const_iterator findQueryUrl( const KUrl& url ) const {
            const KUrl normalizedUrl = cleanupQueryUrl( url );
            QList<KUrl>::const_iterator it = constBegin();
            while ( it != end() ) {
                if ( cleanupQueryUrl( *it ) == normalizedUrl )
                    return it;
                ++it;
            }
            return constEnd();
        }

        bool containsQueryUrl( const KUrl& url ) const {
            return findQueryUrl( url ) != constEnd();
        }

        void addUserQuery( const KUrl& url, const QString& title )
        {
            // create new query url
            KUrl queryUrl = cleanupQueryUrl( url );
            queryUrl.addQueryItem( QLatin1String( "title" ), title );

            // remove old duplicates
            removeUserQuery( url );

            // save the new url
            append( queryUrl );
            save();

            // tell KIO about the new user query
            org::kde::KDirNotify::emitFilesAdded( QLatin1String( "nepomuksearch:/" ) );
        }

        void removeUserQuery( const KUrl& url ) {
            // we clean out all duplicates
            QStringList oldUserQueryUrls;
            QList<KUrl>::iterator it = findQueryUrl( url );
            while ( it != end() ) {
                oldUserQueryUrls << Nepomuk::queryUrlToUserQueryUrl( *it ).url();
                erase( it );
                it = findQueryUrl( url );
            }
            save();
            // FIXME: sadly this does not work due to us setting UDS_URL, but emitting that url
            // does not work either... I think we need help here.
            org::kde::KDirNotify::emitFilesRemoved( oldUserQueryUrls );
        }

    private:
        void read() {
            KUrl::List::operator=( KConfig("kio_nepomuksearchrc" ).group( "User Queries" ).readEntry( "User queries", QStringList() ) );
        }

        void save() const {
            KConfig("kio_nepomuksearchrc" ).group( "User Queries" ).writeEntry( "User queries", toStringList() );
        }
    };
}

#endif
