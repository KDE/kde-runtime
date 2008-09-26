/*
   Copyright (C) 2008 by Sebastian Trueg <trueg at kde.org>

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

#include <nepomuk/term.h>
#include <nepomuk/result.h>
#include <nepomuk/query.h>

#include <kio/udsentry.h>
#include <kio/slavebase.h>
#include <Nepomuk/Resource>
#include <KUrl>


uint qHash( const QUrl& );

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

        SearchEntry* findEntry( const QString& name ) const;
        SearchEntry* findEntry( const KUrl& url ) const;

        void list();
        void stat( const QString& name );

    private Q_SLOTS:
        void slotNewEntries( const QList<Nepomuk::Search::Result>& );
        void slotEntriesRemoved( const QList<QUrl>& );
        void slotFinishedListing();
        void slotStatNextResult();

    private:
        // reimplemented from QThread -> does handle the query
        // we run this in a different thread since SlaveBase does
        // not have an event loop but we need to deliver signals
        // anyway
        void run();

        /**
         * Stats the result and returns the entry.
         */
        SearchEntry* statResult( const Search::Result& result );
        void wrap();

        // folder properties
        QString m_name;
        Search::Query m_query;

        // result cache
        QQueue<Search::Result> m_results;
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

        // if set, this is the name that was requested through
        // stat(). Used during initial listing when not all results
        // are available yet
        QString m_nameToStat;

        // true if stating of an entry has been requested (name of entry in m_nameToStat)
        bool m_statEntry;

        // true if listing of entries has been requested
        bool m_listEntries;

        // true if the stat loop is running
        bool m_statingStarted;

        // used to make all calls sync
        QEventLoop m_loop;

        Search::QueryServiceClient* m_client;

        // mutex to protect the results
        QMutex m_resultMutex;

        friend class SearchFolderHelper;
    };

    /**
     * This class has only one purpose: catch the finished
     * signal from the QueryServiceClient and forward it to
     * the folder. We need this class since Qt cannot send
     * events between different threads and we need a queued
     * connection.
     */
    class SearchFolderHelper : public QObject
    {
        Q_OBJECT

    public:
        SearchFolderHelper( SearchFolder* folder )
            : QObject( 0 ),
            m_folder(folder) {}

    private Q_SLOTS:
        void slotFinishedListing() {
            m_folder->slotFinishedListing();
        }

    private:
        SearchFolder* m_folder;
    };
}

#endif
