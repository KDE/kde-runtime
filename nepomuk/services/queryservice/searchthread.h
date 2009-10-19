/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_SEARCH_THREAD_H_
#define _NEPOMUK_SEARCH_THREAD_H_

#include <QtCore/QThread>
#include <QtCore/QHash>
#include <QtCore/QUrl>
#include <QtCore/QPair>

#include "query.h"
#include "term.h"
#include "result.h"



namespace Soprano {
    class QueryResultIterator;
}

namespace Nepomuk {
    namespace Search {

        class SearchNode
        {
        public:
            enum Type {
                Unknown,
                Lucene,
                Sparql
            };

            SearchNode( const Term& t,
                        Type tt = Unknown,
                        const QList<Nepomuk::Search::Query::FolderLimit>& l = QList<Nepomuk::Search::Query::FolderLimit>(),
                        const QList<SearchNode>& sub = QList<SearchNode>() )
                : term(t),
                type(tt),
                folderLimits(l),
                subNodes(sub) {
            }

            Term term;
            Type type;
            QList<Nepomuk::Search::Query::FolderLimit> folderLimits;
            QList<SearchNode> subNodes;
        };

        class SearchThread : public QThread
        {
            Q_OBJECT

        public:
            SearchThread( QObject* parent = 0 );
            ~SearchThread();

            /**
             * Use instead of QThread::start()
             */
            void query( const Query& query, double cutOffScore );
            void cancel();

            double cutOffScore() const { return m_cutOffScore; }

        signals:
            void newResult( const Nepomuk::Search::Result& result );

        protected:
            void run();

        private:
            /**
             * Makes sure each contains or euality term has a property set instead of
             * a fuzzy field name. However, nonexisting fields will not be changed!
             *
             * If a field matches multiple properties an OR term will be constructed to include
             * them all.
             *
             * Works recursively on all term types.
             */
            Nepomuk::Search::Term resolveFields( const Term& term );

            /**
             * Resolves the values in the contains and equality terms.
             * This means that terms that refer to properties that have resource ranges
             * are replaced by terms that name the actual resources.
             *
             * Be aware that resolveFields needs to be run before this method.
             *
             * Works recursively on all term types.
             */
            Nepomuk::Search::Term resolveValues( const Term& term );

            /**
             * Optimizes a query, i.e. combines OR and AND terms where possible.
             */
            Nepomuk::Search::Term optimize( const Term& term );

            /**
             * Try to split the query into two (or more) subqueries, one of which will be
             * executed against the lucene index and one against the soprano store.
             */
            SearchNode splitLuceneSparql( const Term& term, const QList<Nepomuk::Search::Query::FolderLimit>& folderLimits );

            QList<QUrl> matchFieldName( const QString& field );
            QHash<QUrl, Result> search( const SearchNode& node, double baseScore, bool reportResults = false );
            QHash<QUrl, Result> andSearch( const QList<SearchNode>& terms, double baseScore, bool reportResults = false );
            QHash<QUrl, Result> orSearch( const QList<SearchNode>& terms, double baseScore, bool reportResults = false );
            QHash<QUrl, Nepomuk::Search::Result> sparqlQuery( const QString& query, double baseScore, bool reportResults );
            QHash<QUrl, Nepomuk::Search::Result> luceneQuery( const QString& query, double baseScore, bool reportResults );

            QString buildRequestPropertyVariableList() const;
            QString buildRequestPropertyPatterns() const;
            Nepomuk::Search::Result extractResult( const Soprano::QueryResultIterator& it ) const;
            void fetchRequestPropertiesForResource( Result& uri );

            QString createSparqlQuery( const SearchNode& node );

            Query m_searchTerm;
            double m_cutOffScore;

            // status
            int m_numResults;
            bool m_canceled;

            // cache of all prefixes that are supported
            QHash<QString, QUrl> m_prefixes;
        };
    }
}

#endif
