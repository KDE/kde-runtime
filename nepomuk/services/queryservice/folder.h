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

#include <QtCore/QHash>
#include <QtCore/QTimer>

#include <KUrl>

#include "searchthread.h"

namespace Nepomuk {
    namespace Query {

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
            Folder( const QString& query, const RequestPropertyMap& requestProps, QObject* parent = 0 );
            ~Folder();

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

        public Q_SLOTS:
            void update();

        Q_SIGNALS:
            void newEntries( const QList<Nepomuk::Query::Result>& entries );
            void entriesRemoved( const QList<QUrl>& entries );
            void finishedListing();

        private Q_SLOTS:
            void slotSearchNewResult( const Nepomuk::Query::Result& );
            void slotSearchFinished();
            void slotStorageChanged();
            void slotUpdateTimeout();

        private:
            /**
             * Called by the FolderConnection constructor.
             */
            void addConnection( FolderConnection* );

            /**
             * Called by the FolderConnection destructor.
             */
            void removeConnection( FolderConnection* );

            QString m_query;
            RequestPropertyMap m_requestProperties;
            QList<FolderConnection*> m_connections;

            bool m_initialListingDone;
            QHash<QUrl, Result> m_results;    // the actual current results
            QHash<QUrl, Result> m_newResults; // the results gathered during an update, needed to find removed items

            SearchThread* m_searchThread;

            bool m_storageChanged;            // did the nepomuk store change after the last update
            QTimer m_updateTimer;             // used to ensure that we do not update all the time if the storage changes a lot

            friend class FolderConnection;    // for addConnection and removeConnection
        };
    }
}

#endif
