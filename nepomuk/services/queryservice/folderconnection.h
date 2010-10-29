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

#ifndef _NEPOMUK_VIRTUAL_FOLDER_CONNECTION_H_
#define _NEPOMUK_VIRTUAL_FOLDER_CONNECTION_H_

#include <QtCore/QObject>
#include <QtDBus/QDBusObjectPath>

class QUrl;
class QDBusServiceWatcher;

namespace Nepomuk {
    namespace Query {

        class Result;
        class ResultIterator;
        class Folder;

        class FolderConnection : public QObject
        {
            Q_OBJECT

        public:
            FolderConnection( Folder* parentFolder );
            ~FolderConnection();

            QDBusObjectPath registerDBusObject( const QString& dbusClient, int id );

        public Q_SLOTS:
            /// List all entries in the folder, used by the public kdelibs API
            void list();

            /// internal API used by the kio kded module to listen for changes
            void listen();

            /// close the connection to the folder. Will delete this connection
            void close();

            /// \return \p true if the initial listing has been finished and the finishedListing()
            /// has been emitted
            bool isListingFinished() const;

            /// \return the SPARQL query of the folder
            QString queryString() const;

        Q_SIGNALS:
            void newEntries( const QList<Nepomuk::Query::Result>& );
            void entriesRemoved( const QStringList& );
            void resultCount( int count );
            void totalResultCount( int count );
            void finishedListing();

        private Q_SLOTS:
            void slotEntriesRemoved( const QList<QUrl>& entries );
            void slotFinishedListing();

        private:
            Folder* m_folder;
            QDBusServiceWatcher* m_serviceWatcher;
        };
    }
}

#endif
