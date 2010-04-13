/*
   Copyright (c) 2008-2009 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_QUERY_SERVICE_H_
#define _NEPOMUK_QUERY_SERVICE_H_

#include <Nepomuk/Service>

#include "dbusoperators_p.h"

#include <KUrl>

#include <QtCore/QVariant>
#include <QtCore/QHash>

class QDBusObjectPath;
class QDBusMessage;
class QDBusServiceWatcher;

namespace Nepomuk {
    namespace Query {

        class Folder;
        class FolderConnection;

        class QueryService : public Service
        {
            Q_OBJECT
            Q_CLASSINFO( "D-Bus Interface", "org.kde.nepomuk.QueryService" )

        public:
            QueryService( QObject* parent, const QVariantList& );
            ~QueryService();

            static QueryService* instance();

        public Q_SLOTS:
            /**
             * Create a query folder from desktop query string \p query. The optional properties in \p requestProps will be
             * added to the Query object once \p query has been parsed by QueryParser.
             */
            Q_SCRIPTABLE QDBusObjectPath query( const QString& query, const QDBusMessage& msg );

            /**
             * Create a query folder from SPARQL query string \p query. The optional properties in \p requestProps are assumed
             * to be part of \p query already as done by Query::toSparqlQuery. They are simple needed to map bindings to properties.
             */
            Q_SCRIPTABLE QDBusObjectPath sparqlQuery( const QString& query, const RequestPropertyMapDBus& requestProps, const QDBusMessage& msg );

        private Q_SLOTS:
            void slotServiceUnregistered( const QString& serviceName );
            void slotFolderDestroyed( QObject* folder );
            void slotFolderConnectionDestroyed( QObject* conn );

        private:
            /**
             * Creates a new folder or reuses an existing one.
             */
            Folder* getFolder( const QString& sparql, const RequestPropertyMap& requestProps );

            static QueryService* s_instance;

            QHash<QString, Folder*> m_openFolders;
            QHash<Folder*, QString> m_folderQueryHash;
            QMultiHash<QString, FolderConnection*> m_openConnections;      // maps from DBus service to connection
            QHash<FolderConnection*, QString> m_connectionDBusServiceHash; // maps connections to their using dbus service

            int m_folderConnectionCnt; // only used for unique dbus object path generation
            QDBusServiceWatcher *m_serviceWatcher;
        };
    }
}

#endif
