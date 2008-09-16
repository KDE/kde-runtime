/* 
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_VIRTUAL_FOLDER_SERVICE_H_
#define _NEPOMUK_VIRTUAL_FOLDER_SERVICE_H_

#include <Nepomuk/Service>

#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtCore/QMultiHash>

class QDBusObjectPath;
class QDBusMessage;


namespace Nepomuk {
    namespace Search {

        class Folder;
        class FolderConnection;
        class Query;

        class QueryService : public Service
        {
            Q_OBJECT
            Q_CLASSINFO( "D-Bus Interface", "org.kde.nepomuk.QueryService" )

        public:
            QueryService( QObject* parent, const QVariantList& );
            ~QueryService();

            Soprano::Model* mainModel();

            static QueryService* instance();
            
        public Q_SLOTS:
            Q_SCRIPTABLE QDBusObjectPath query( const QString& query, const QStringList& props, const QDBusMessage& msg );
            Q_SCRIPTABLE QDBusObjectPath query( const Nepomuk::Search::Query& query, const QDBusMessage& msg );

        private Q_SLOTS:
            void slotServiceOwnerChanged( const QString& serviceName,
                                          const QString&,
                                          const QString& newOwner );
            void slotFolderDestroyed( QObject* folder );
            void slotFolderConnectionDestroyed( QObject* conn );
        
        private:
            /**
             * Creates a new folder or reuses an existing one.
             */
            Folder* getFolder( const Query& query );

            static QueryService* s_instance;

            QHash<Query, Folder*> m_openFolders;
            QHash<Folder*, Query> m_folderQueryHash;
            QMultiHash<QString, FolderConnection*> m_openConnections;      // maps from DBus service to connection
            QHash<FolderConnection*, QString> m_connectionDBusServiceHash; // maps connections to their using dbus service

            int m_folderConnectionCnt; // only used for unique dbus object path generation
        };
    }
}

#endif
