/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2008 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_SEARCH_QUERY_H_
#define _NEPOMUK_SEARCH_QUERY_H_

#include <QtCore/QSharedDataPointer>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QDebug>

class QUrl;

namespace Nepomuk {
    namespace Search {

        class Term;

        /**
         * \class Query query.h nepomuk/query.h
         *
         * \brief A Nepomuk desktop query.
         *
         * A query can either be based on Term or a more complex
         * SPARQL query.
         *
         * \author Sebastian Trueg <trueg@kde.org>
         */
        class Query
        {
        public:
            enum Type {
                InvalidQuery,
                PlainQuery,
                SPARQLQuery
            };

            /**
             * Create an empty invalid query object.
             */
            Query();

            /**
             * Create a query of type PlainQuery based on
             * \a term.
             */
            Query( const Term& term );

            /**
             * Create a SPARQL query. The query has to have one select variable called "?r"
             */
            explicit Query( const QString& sparqlQuery );

            /**
             * Copy constructor.
             */
            Query( const Query& );

            /**
             * Destructor
             */
            ~Query();

            /**
             * Assignment operator
             */
            Query& operator=( const Query& );

            Type type() const;
            Term term() const;
            QString sparqlQuery() const;
            int limit() const;

            void setTerm( const Term& );
            void setSparqlQuery( const QString& );
            void setLimit( int );

            /**
             * Add a property that should be reported with each search result.
             * \param property The requested property.
             * \param optional If \p true the property is optional, meaning it can
             *        be empty ins earch results.
             */
            void addRequestProperty( const QUrl& property, bool optional = true );
            void clearRequestProperties();

            typedef QPair<QUrl, bool> RequestProperty;

            QList<RequestProperty> requestProperties() const;

            bool operator==( const Query& ) const;
            
            /**
             * Add a folder to the search space. If no folder limits are set, will
             * search the entire Nepomuk database
             * \param folder the folder to search
             * \param include true if we should search this folder, false
             * if we should exclude this folder from search results
             */
            void addFolderLimit( const QUrl& folder, bool include );
            void clearFolderLimits();
            
            typedef QPair<QUrl, bool> FolderLimit;
            
            QList<FolderLimit> folderLimits() const;

        private:
            class Private;
            QSharedDataPointer<Private> d;
        };

        uint qHash( const Nepomuk::Search::Query& );
    }
}

QDebug operator<<( QDebug, const Nepomuk::Search::Query& );

#endif
