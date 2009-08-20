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

#ifndef _NEPOMUK_KIO_NEPOMUK_SEARCH_H_
#define _NEPOMUK_KIO_NEPOMUK_SEARCH_H_

#include <kio/forwardingslavebase.h>

#include "term.h"
#include "searchfolder.h"

#include <QtCore/QQueue>

namespace Nepomuk {
    namespace Search {
        class Query;
    }

    class SearchProtocol : public KIO::ForwardingSlaveBase
    {
        Q_OBJECT

    public:
        SearchProtocol( const QByteArray& poolSocket, const QByteArray& appSocket );
        virtual ~SearchProtocol();

        /**
         *
         */
        void listDir( const KUrl& url );

        /**
         * Will be forwarded for files.
         */
        void get( const KUrl& url );

        /**
         * Not supported.
         */
        void put( const KUrl& url, int permissions, KIO::JobFlags flags );

        /**
         * Files will be forwarded.
         * Folders will be created as virtual folders.
         */
        void mimetype( const KUrl& url );

        /**
         * Files will be forwarded.
         * Folders will be created as virtual folders.
         */
        void stat( const KUrl& url );

    protected:
        /**
         * reimplemented from ForwardingSlaveBase
         */
        bool rewriteUrl( const KUrl& url, KUrl& newURL );

    private:
        bool ensureNepomukRunning();
        void listRoot();
        void listQuery( const QString& query );
        void listActions();
        void listDefaultSearches();
        void listDefaultSearch( const QString& path );
        void addDefaultSearch( const QString& name, const Search::Query& );

        SearchFolder* extractSearchFolder( const KUrl& url );

        /**
         * Get (possibly cached) query results
         */
        SearchFolder* getQueryResults( const QString& query );
        SearchFolder* getDefaultQueryFolder( const QString& name );

        // the default search folders
        QHash<QString, Nepomuk::Search::Query> m_defaultSearches;

        // cache of all on-the-fly search folders
        QHash<QString, SearchFolder*>  m_searchCache;

        // queue to remember the order of the search cache
        // when enforcing the cache max
        QQueue<QString> m_searchCacheNameQueue;

        // cache for the default searches, needs to differ from
        // on-the-fly cache because of the name
        QHash<QString, SearchFolder*>  m_defaultSearchCache;
    };
}

#endif
