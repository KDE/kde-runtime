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

#ifndef _NEPOMUK_SEARCH_URL_TOOLS_H_
#define _NEPOMUK_SEARCH_URL_TOOLS_H_

#include <QtCore/QString>

#include <nepomuk/query.h>
#include <nepomuk/queryparser.h>

#include <kurl.h>

#include "nie.h"

namespace Nepomuk {
    /**
     * Encode the resource URI into the UDS_NAME to make it unique.
     * It is important that we do not use the % for percent-encoding. Otherwise KUrl::url will
     * re-encode them, thus, destroying our name.
     */
    inline QString resourceUriToUdsName( const KUrl& url )
    {
        return QString::fromAscii( url.toEncoded().toPercentEncoding( QByteArray(), QByteArray(""), '_' ) );
    }

    /**
     * The inverse of the above.
     */
    inline QUrl udsNameToResourceUri( const QString& name )
    {
        return QUrl::fromEncoded( QByteArray::fromPercentEncoding( name.toAscii(), '_' ) );
    }

    /**
     * Returns an empty string for sparql query URLs.
     */
    inline QString extractPlainQuery( const KUrl& url ) {
        if( url.queryItems().contains( "query" ) ) {
            return url.queryItem( "query" );
        }
        else if ( !url.queryItems().contains( "sparql" ) ) {
            return url.path().section( '/', 0, 0, QString::SectionSkipEmpty );
        }
        else {
            return QString();
        }
    }

    /**
     * Extract SPARQL query from a nepomuksearch query URL.
     */
    inline QString queryFromUrl( const KUrl& url ) {
        if( url.queryItems().contains( "sparql" ) ) {
            return url.queryItem( "sparql" );
        }
        else {
            Query::Query query = Query::QueryParser::parseQuery( extractPlainQuery( url ) );
            query.addRequestProperty( Query::Query::RequestProperty( Nepomuk::Vocabulary::NIE::url(), true ) );
            return query.toSparqlQuery();
        }
    }
}

#endif
