/*
   Copyright (C) 2008-2009 by Sebastian Trueg <trueg at kde.org>

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

#ifndef _NEPOMUK_SEARCH_FOLDER_H_
#define _NEPOMUK_SEARCH_FOLDER_H_

#include <QtCore/QThread>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QEventLoop>
#include <QtCore/QQueue>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

#include "term.h"
#include "result.h"
#include "query.h"

#include <kio/udsentry.h>
#include <kio/slavebase.h>
#include <Nepomuk/Resource>
#include <KUrl>


namespace Nepomuk {
    namespace Search {
        class QueryServiceClient;
    }

    class SearchFolder;

    class SearchEntry
    {
    public:
        SearchEntry( const QUrl& uri,
                     const KIO::UDSEntry& = KIO::UDSEntry() );

        QUrl resource() const { return m_resource; }
        KIO::UDSEntry entry() const { return m_entry; }

    private:
        QUrl m_resource;
        KIO::UDSEntry m_entry;

        friend class SearchFolder;
    };


    class SearchFolder : public QThread
    {
        Q_OBJECT

    public:
        SearchFolder();
        SearchFolder( const QString& name, const Search::Query& query, KIO::SlaveBase* slave );
        ~SearchFolder();

        Search::Query query() const { return m_query; }
        QString name() const { return m_name; }
        QList<SearchEntry*> entries() const { return m_entries.values(); }

        SearchEntry* findEntry( const QString& name );
        SearchEntry* findEntry( const KUrl& url );

        void list();
        void stat( const QString& name );

    private Q_SLOTS:
        /// connected to the QueryServiceClient in the search thread
        void slotNewEntries( const QList<Nepomuk::Search::Result>& );

        /// connected to the QueryServiceClient in the search thread
        void slotEntriesRemoved( const QList<QUrl>& );

        /// connected to the QueryServiceClient in the search thread
        void slotFinishedListing();

    private:
        // reimplemented from QThread -> does handle the query
        // we run this in a different thread since SlaveBase does
        // not have an event loop but we need to deliver signals
        // anyway
        void run();

        /**
         * This method will stat all entries in m_resultsQueue until
         * the search thread is finished. It is run in the main thread.
         */
        void statResults();

        /**
         * Stats the result and returns the entry.
         */
        SearchEntry* statResult( const Search::Result& result );

        // folder properties
        QString m_name;
        Search::Query m_query;

        // result cache, filled by the search thread
        QQueue<Search::Result> m_resultsQueue;

        // final results, filled by the main thread in statResults
        QHash<QString, SearchEntry*> m_entries;
        QHash<QUrl, QString> m_resourceNameMap;

        // used to make sure we have unique names
        // FIXME: the changed names could clash again!
        QHash<QString, int> m_nameCntHash;

        // true if the client listed all results and new
        // ones do not need to be listed but emitted through
        // KDirNotify
        bool m_initialListingFinished;

        // the parent slave used for listing and stating
        KIO::SlaveBase* m_slave;

        // true if listing of entries has been requested
        bool m_listEntries;

        Search::QueryServiceClient* m_client;

        // mutex to protect the results
        QMutex m_resultMutex;

        // used to wait in the main thread for the serch thread to deliver results
        QWaitCondition m_resultWaiter;
    };
}

#endif
