/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef QUERYUTILS_H
#define QUERYUTILS_H

#include "kext.h"

#include <KUrl>
#include <KDebug>

#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/OptionalTerm>
#include <Nepomuk/Query/ComparisonTerm>
#include <Nepomuk/Query/AndTerm>
#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Vocabulary/NFO>


namespace Nepomuk {
    namespace Query {
        /**
         * KIO specific query handling shared by the KIO slave and the kded search module
         */
        bool parseQueryUrl( const KUrl& url, Query& query, QString& sparqlQuery )
        {
            // parse URL (this may fail in which case we fall back to pure SPARQL below)
            query = Nepomuk::Query::Query::fromQueryUrl( url );

            // request properties to easily create UDSEntry instances
            QList<Query::RequestProperty> reqProperties;
            // local URL
            reqProperties << Query::RequestProperty( Nepomuk::Vocabulary::NIE::url(), !query.isFileQuery() );
#ifdef Q_OS_UNIX
            if( query.isFileQuery() ) {
                // file size
                ComparisonTerm contentSizeTerm( Nepomuk::Vocabulary::NIE::contentSize(), Term() );
                contentSizeTerm.setVariableName( QLatin1String("size") );
                // mimetype
                ComparisonTerm mimetypeTerm( Nepomuk::Vocabulary::NIE::mimeType(), Term() );
                mimetypeTerm.setVariableName( QLatin1String("mime") );
                // mtime
                ComparisonTerm mtimeTerm( Nepomuk::Vocabulary::NIE::lastModified(), Term() );
                mtimeTerm.setVariableName( QLatin1String("mtime") );
                // mode
                ComparisonTerm modeTerm( Nepomuk::Vocabulary::KExt::unixFileMode(), Term() );
                modeTerm.setVariableName( QLatin1String("mode") );
                // user
                ComparisonTerm userTerm( Nepomuk::Vocabulary::KExt::unixFileOwner(), Term() );
                userTerm.setVariableName( QLatin1String("user") );
                // group
                ComparisonTerm groupTerm( Nepomuk::Vocabulary::KExt::unixFileGroup(), Term() );
                groupTerm.setVariableName( QLatin1String("group") );

                // instead of separate request properties we use one optional and term. That way
                // all or none of the properties will be bound which makes handling the data in
                // SearchFolder::statResult much simpler.
                AndTerm filePropertiesTerm;
                filePropertiesTerm.addSubTerm( contentSizeTerm );
                filePropertiesTerm.addSubTerm( mimetypeTerm );
                filePropertiesTerm.addSubTerm( mtimeTerm );
                filePropertiesTerm.addSubTerm( modeTerm );
                filePropertiesTerm.addSubTerm( userTerm );
                filePropertiesTerm.addSubTerm( groupTerm );
                query = query && OptionalTerm::optionalizeTerm( filePropertiesTerm );
            }
#endif // Q_OS_UNIX
            query.setRequestProperties( reqProperties );

            if ( query.isValid() ) {
                kDebug() << "Extracted query" << query;
            }
            else {
                // the URL contains pure sparql.
                sparqlQuery = Nepomuk::Query::Query::sparqlFromQueryUrl( url );
                kDebug() << "Extracted SPARL query" << sparqlQuery;
            }

            return query.isValid() || !sparqlQuery.isEmpty();
        }
    }
}

#endif // QUERYUTILS_H
