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

#ifndef _NEPOMUK_STANDARD_QUERIES_H_
#define _NEPOMUK_STANDARD_QUERIES_H_

#include <Nepomuk/Query/FileQuery>
#include <Nepomuk/Query/ComparisonTerm>
#include <Nepomuk/Query/AndTerm>
#include <Nepomuk/Query/OrTerm>
#include <Nepomuk/Query/LiteralTerm>
#include <Nepomuk/Query/NegationTerm>
#include <Nepomuk/Query/ResourceTerm>

#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/RDF>

#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NUAO>

namespace Nepomuk {
    /**
     * Creates a query that returns all files sorted by descending modification date.
     */
    inline Query::Query lastModifiedFilesQuery() {
        Query::ComparisonTerm lastModifiedTerm( Nepomuk::Vocabulary::NIE::lastModified(), Query::Term() );
        lastModifiedTerm.setSortWeight( 1, Qt::DescendingOrder );
        Query::FileQuery lastModifiedQuery( lastModifiedTerm );
        lastModifiedQuery.setFileMode( Query::FileQuery::QueryFiles );
        return lastModifiedQuery;
    }

    /**
     * Creates a query that returns all files sorted by descending score (as calculated
     * by the DataMaintenanceService)
     */
    inline Query::Query mostImportantFilesQuery() {
        Query::ComparisonTerm fancyTerm( Soprano::Vocabulary::NAO::score(), Query::Term() );
        fancyTerm.setSortWeight( 1, Qt::DescendingOrder );
        Query::FileQuery fancyQuery( fancyTerm );
        fancyQuery.setFileMode( Query::FileQuery::QueryFiles );
        return fancyQuery;
    }

    /**
     * Creates a query that returns all files with a usage count of 0
     * sorted by descending modification date.
     */
    inline Query::Query neverOpenedFilesQuery() {
        // there are two ways a usage count of 0 can be expressed:
        // 1. property with value 0
        // 2. no property at all
        Query::OrTerm usageCntTerm(
            Query::ComparisonTerm(
                Nepomuk::Vocabulary::NUAO::usageCount(),
                Query::LiteralTerm( 0 ),
                Query::ComparisonTerm::Equal ),
            Query::NegationTerm::negateTerm(
                Query::ComparisonTerm(
                    Nepomuk::Vocabulary::NUAO::usageCount(),
                    Query::Term() ) ) );

        // Before we had the data management service there was no usage count
        // tracking. Thus, in order not to return all files we should filter
        // out all files that were created before we started tracking usage.
        // However, by default we only show the top 10 results. Thus, in the
        // worst case this query will return the same as lastModifiedFilesQuery().
        Query::ComparisonTerm modDateTerm(
            Nepomuk::Vocabulary::NIE::lastModified(),
            Query::Term() );
        modDateTerm.setSortWeight( 1, Qt::DescendingOrder );

        Query::FileQuery query( Query::AndTerm( usageCntTerm, modDateTerm ) );
        query.setFileMode( Query::FileQuery::QueryFiles );
        return query;
    }
}

#endif
