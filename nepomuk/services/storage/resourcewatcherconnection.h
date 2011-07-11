/*
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


#ifndef RESOURCEWATCHERCONNECTION_H
#define RESOURCEWATCHERCONNECTION_H

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtDBus/QDBusObjectPath>

class QDBusServiceWatcher;

namespace Nepomuk {

    class ResourceWatcherManager;

    class ResourceWatcherConnection : public QObject
    {
        Q_OBJECT
        Q_CLASSINFO( "D-Bus Interface", "org.kde.nepomuk.ResourceWatcherConnection" )

    public:
        ResourceWatcherConnection( ResourceWatcherManager* parent, bool hasProperties );
        ~ResourceWatcherConnection();

    signals:
        Q_SCRIPTABLE void resourceCreated( const QString & uri, const QStringList& types );
        Q_SCRIPTABLE void resourceRemoved( const QString & uri, const QStringList& types );
        Q_SCRIPTABLE void resourceTypeAdded( const QString & resUri, const QString & type );
        Q_SCRIPTABLE void resourceTypeRemoved( const QString & resUri, const QString & type );
        Q_SCRIPTABLE void propertyAdded( const QString & resource,
                                         const QString & property,
                                         const QDBusVariant & value );
        Q_SCRIPTABLE void propertyRemoved( const QString & resource,
                                           const QString & property,
                                           const QDBusVariant & value );

    public Q_SLOTS:
        Q_SCRIPTABLE void close();

    public:
        bool hasProperties() const;

        QDBusObjectPath registerDBusObject(const QString &dbusClient, int id);

    private:
        QString m_objectPath;
        bool m_hasProperties;

        ResourceWatcherManager* m_manager;
        QDBusServiceWatcher* m_serviceWatcher;

        friend class ResourceWatcherManager;
    };

}

#endif // RESOURCEWATCHERCONNECTION_H
