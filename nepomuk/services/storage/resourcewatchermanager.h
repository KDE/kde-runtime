/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2011 Vishesh Handa <handa.vish@gmail.com>
    Copyright (C) 2011-2012 Sebastian Trueg <trueg@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef RESOURCEWATCHMANAGER_H
#define RESOURCEWATCHMANAGER_H

#include <QtCore/QMultiHash>
#include <QtCore/QSet>

#include <Soprano/FilterModel>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusContext>

namespace Nepomuk {

    class ResourceWatcherConnection;
    class DataManagementModel;

    class ResourceWatcherManager : public QObject, protected QDBusContext
    {
        Q_OBJECT
        Q_CLASSINFO( "D-Bus Interface", "org.kde.nepomuk.ResourceWatcher" )

    public:
        ResourceWatcherManager( DataManagementModel* parent = 0 );
        ~ResourceWatcherManager();

        void addStatement(const Soprano::Statement &st);
        // IDEA: would it be more efficient to have three lists/sets: keptValues, newValues, removedValues?
        void changeProperty(const QUrl& res,
                            const QUrl& property,
                            const QList<Soprano::Node>& addedValues,
                            const QList<Soprano::Node>& removedValues);
        void changeProperty(const QMultiHash<QUrl, Soprano::Node>& oldValues, const QUrl& property,
                            const QList<Soprano::Node>& nodes);
        void createResource(const QUrl& uri, const QList<QUrl>& types);
        void removeResource(const QUrl& uri, const QList<QUrl>& types);

        /// to be called whenever something changes (preferably after calling any of the above)
        void changeSomething();

    signals:
        /**
         * A special signal which is emitted whenever something changes in the store.
         * Normally specifying in detail what to watch for by creating a connection
         * is recommended. However, if one really wants to watch for any changes at all
         * this signal is more efficient than a connection since it wraps several changes
         * done with one command into one notification.
         */
        Q_SCRIPTABLE void somethingChanged();

    public slots:
        /**
         * Used internally by watch() and by the unit tests to create watcher connections.
         */
        ResourceWatcherConnection* createConnection(const QList<QUrl>& resources,
                                                    const QList<QUrl>& properties,
                                                    const QList<QUrl>& types );

        /**
         * The main DBus methods exposed by the ResourceWatcher
         */
        Q_SCRIPTABLE QDBusObjectPath watch( const QStringList& resources,
                                            const QStringList& properties,
                                            const QStringList& types );

    private:
        /// called by ResourceWatcherConnection destructor
        void removeConnection(ResourceWatcherConnection*);

        /// called by ResourceWatcherConnection
        void setResources(ResourceWatcherConnection* conn, const QStringList& resources);
        void addResource(ResourceWatcherConnection* conn, const QString& resource);
        void removeResource(ResourceWatcherConnection* conn, const QString& resource);
        void setProperties(ResourceWatcherConnection* conn, const QStringList& propertys);
        void addProperty(ResourceWatcherConnection* conn, const QString& property);
        void removeProperty(ResourceWatcherConnection* conn, const QString& property);
        void setTypes(ResourceWatcherConnection* conn, const QStringList& types);
        void addType(ResourceWatcherConnection* conn, const QString& type);
        void removeType(ResourceWatcherConnection* conn, const QString& type);

        QSet<QUrl> getTypes(const Soprano::Node& res) const;

        /// called by changeProperty
        void changeTypes(const QUrl& res, const QSet<QUrl> &resTypes, const QSet<QUrl> &addedTypes, const QSet<QUrl> &removedTypes);

        /// this checks if the connection watches one of the types or no types at all (wildcard)
        bool connectionWatchesOneType(ResourceWatcherConnection* con, const QSet<QUrl> &types) const;

        DataManagementModel* m_model;

        QMultiHash<QUrl, ResourceWatcherConnection*> m_resHash;
        QMultiHash<QUrl, ResourceWatcherConnection*> m_propHash;
        QMultiHash<QUrl, ResourceWatcherConnection*> m_typeHash;
        QSet<ResourceWatcherConnection*> m_watchAllConnections;

        // only used to generate unique dbus paths
        int m_connectionCount;

        friend class ResourceWatcherConnection;
    };

}

#endif // RESOURCEWATCHMANAGER_H
