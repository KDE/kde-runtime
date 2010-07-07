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

#ifndef _NEPOMUK_QUERY_SERVICE_H_
#define _NEPOMUK_QUERY_SERVICE_H_

#include <Nepomuk/Service>
#include <Nepomuk/Query/Query>

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

        public Q_SLOTS:
            /**
             * Create a query folder from encoded query \p query.
             */
            Q_SCRIPTABLE QDBusObjectPath query( const QString& query, const QDBusMessage& msg );

            /**
             * Create a query folder from desktop query string \p query which needs to be parsable by QueryParser.
             */
            Q_SCRIPTABLE QDBusObjectPath desktopQuery( const QString& query, const QDBusMessage& msg );

            /**
             * Create a query folder from SPARQL query string \p query. The optional properties in \p requestProps are assumed
             * to be part of \p query already as done by Query::toSparqlQuery. They are simple needed to map bindings to properties.
             */
            Q_SCRIPTABLE QDBusObjectPath sparqlQuery( const QString& query, const RequestPropertyMapDBus& requestProps, const QDBusMessage& msg );

        private Q_SLOTS:
            void slotFolderAboutToBeDeleted( Nepomuk::Query::Folder* folder );

        private:
            /**
             * Creates a new folder or reuses an existing one.
             */
            Folder* getFolder( const Query& query );

            /**
             * Creates a new folder or reuses an existing one.
             */
            Folder* getFolder( const QString& sparql, const RequestPropertyMap& requestProps );

            QHash<QString, Folder*> m_openSparqlFolders;
            QHash<Query, Folder*> m_openQueryFolders;

            int m_folderConnectionCnt; // only used for unique dbus object path generation
        };
    }
}

#endif
