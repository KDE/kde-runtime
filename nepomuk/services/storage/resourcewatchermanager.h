/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2011 Vishesh Handa <handa.vish@gmail.com>
    Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

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

    class ResourceWatcherManager : public QObject, protected QDBusContext
    {
        Q_OBJECT
        Q_CLASSINFO( "D-Bus Interface", "org.kde.nepomuk.ResourceWatcher" )

    public:
        ResourceWatcherManager( QObject* parent = 0 );
        ~ResourceWatcherManager();

        void addProperty(const Soprano::Node res, const QUrl& property, const Soprano::Node& value);
        void removeProperty(const Soprano::Node res, const QUrl& property, const Soprano::Node& value);
        void createResource(const QUrl& uri, const QList<QUrl>& types);
        void removeResource(const QUrl& uri, const QList<QUrl>& types);

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

        QMultiHash<QUrl, ResourceWatcherConnection*> m_resHash;
        QMultiHash<QUrl, ResourceWatcherConnection*> m_propHash;
        QMultiHash<QUrl, ResourceWatcherConnection*> m_typeHash;

        // only used to generate unique dbus paths
        int m_connectionCount;

        friend class ResourceWatcherConnection;
    };

}

#endif // RESOURCEWATCHMANAGER_H
