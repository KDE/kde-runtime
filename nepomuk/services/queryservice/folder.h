/*
   Copyright (c) 2008-2010 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_VIRTUAL_FOLDER_H_
#define _NEPOMUK_VIRTUAL_FOLDER_H_

#include <QtCore/QObject>

#include <Nepomuk/Query/Result>
#include <Nepomuk/Query/Query>

#include <QtCore/QHash>
#include <QtCore/QTimer>
#include <QtCore/QPointer>

#include <KUrl>

namespace Soprano {
    namespace Util {
        class AsyncQuery;
    }
}

namespace Nepomuk {
    namespace Query {

        class SearchRunnable;
        class FolderConnection;

        /**
         * One search folder which automatically updates itself.
         * Once all connections to the folder have been deleted,
         * the folder deletes itself.
         */
        class Folder : public QObject
        {
            Q_OBJECT

        public:
            Folder( const Query& query, QObject* parent = 0 );
            Folder( const QString& sparqlQuery, const RequestPropertyMap& requestProps, QObject* parent = 0 );
            ~Folder();

            bool isSparqlQueryFolder() const { return m_isSparqlQueryFolder; }

            /**
             * \return A list of all cached results in the folder.
             * If initial listing is not finished yet, the results found
             * so far are returned.
             */
            QList<Result> entries() const;

            /**
             * \return true if the initial listing is done, ie. further
             * signals only mean a change in the folder.
             */
            bool initialListingDone() const;

            QList<FolderConnection*> openConnections() const;

            Query query() const { return m_query; }

            QString sparqlQuery() const;

            RequestPropertyMap requestPropertyMap() const;

            /**
             * Get the result count. This value will not be available
             * right away since it is calculated async.
             *
             * As soon as the count is ready resultCount() is emitted.
             *
             * \return The result count or -1 in case it is not ready yet.
             */
            int getResultCount() const { return m_resultCount; }

            /**
             * Get the total result count. This value will not be available
             * right away since it is calculated async.
             *
             * As soon as the total count is ready totalCount() is emitted.
             *
             * \return The total result count or -1 in case it is not ready yet.
             */
            int getTotalResultCount() const { return m_totalResultCount; }

            void addResult( const Result& result );

            void listingFinished();

            /// called by the CountQueryRunnable
            void countQueryFinished( int count );
            /// called by the CountQueryRunnable
            void totalCountQueryFinished( int count );

        public Q_SLOTS:
            void update();

        Q_SIGNALS:
            void newEntries( const QList<Nepomuk::Query::Result>& entries );
            void entriesRemoved( const QList<QUrl>& entries );

            /**
             * Emitted once the result count is available.
             *
             * \sa getResultCount()
             */
            void resultCount( int count );

            /**
             * Emitted once the total result count is available.
             *
             * \sa getTotalResultCount()
             */
            void totalResultCount( int count );
            void finishedListing();
            void aboutToBeDeleted( Nepomuk::Query::Folder* );

        private Q_SLOTS:
            void slotStorageChanged();
            void slotUpdateTimeout();

        private:
            void init();

            /**
             * Called by the FolderConnection constructor.
             */
            void addConnection( FolderConnection* );

            /**
             * Called by the FolderConnection destructor.
             */
            void removeConnection( FolderConnection* );

            /// true if this query has been created with a string query rather than a Query object
            const bool m_isSparqlQueryFolder;

            /// if valid m_sparqlQuery and m_requestProperties are ignored
            Query m_query;

            /// only used if m_query is not valid
            QString m_sparqlQuery;

            /// only used if m_query is not valid
            RequestPropertyMap m_requestProperties;

            /// all listening connections
            QList<FolderConnection*> m_connections;

            /// the result count. -1 until determined
            int m_resultCount;

            /// the total result count. -1 until determined
            int m_totalResultCount;

            /// Query used to get the total count. Only non-null during execution
            QPointer<Soprano::Util::AsyncQuery> m_countQuery;

            /// true once the initial listing is done and only updates are to be signalled
            bool m_initialListingDone;

            /// the actual current results
            QHash<QUrl, Result> m_results;

            /// the results gathered during an update, needed to find removed items
            QHash<QUrl, Result> m_newResults;

            /// the runnable doing work at the moment or 0 if idle
            SearchRunnable* m_currentSearchRunnable;

            /// did the nepomuk store change after the last update - used for caching of update signals via m_updateTimer
            bool m_storageChanged;

            /// used to ensure that we do not update all the time if the storage changes a lot
            QTimer m_updateTimer;

            // for addConnection and removeConnection
            friend class FolderConnection;
        };
    }
}

#endif
