/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011 Vishesh Handa <handa.vish@gmail.com>

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

namespace Soprano {
    class Statement;
}

namespace Nepomuk {

    class ResourceWatcherModel;
    
    class ResourceWatcherConnection : public QObject
    {
        Q_OBJECT
    public:
        ResourceWatcherConnection( QObject* parent, bool hasProperties );

    signals:
        Q_SCRIPTABLE void resourceDeleted( const QString & uri );

        Q_SCRIPTABLE void resourceTypeCreated( const QString & resUri, const QString & type );

        Q_SCRIPTABLE void propertyAdded( const QString & resource,
                                         const QString & property,
                                         const QString & value );
        Q_SCRIPTABLE void propertyRemoved( const QString & resource,
                                           const QString & property,
                                           const QString & value );
    public:
        QDBusObjectPath dBusPath() const;
        bool hasProperties() const;
        
    private:
        QString m_objectPath;
        bool m_hasProperties;

        static int Id;
        friend class ResourceWatcherManager;
    };

}

#endif // RESOURCEWATCHERCONNECTION_H
