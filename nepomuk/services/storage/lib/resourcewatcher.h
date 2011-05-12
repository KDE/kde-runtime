/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2011  Vishesh Handa <handa.vish@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef RESOURCEWATCHER_H
#define RESOURCEWATCHER_H

#include <Nepomuk/Types/Class>
#include <Nepomuk/Types/Property>
#include <Nepomuk/Resource>

#include "nepomukdatamanagement_export.h"

namespace Nepomuk {

    /**
     * \class ResourceWatcher resourcewatcher.h
     *
     * A Resource watcher is an object that be used to selectively monitor the nepomuk repository
     * for changes. Resources may be monitored on the basis of types, properties, and uris. 
     *
     * Changes may be monitored in one of the following ways -
     * 1. By Resource Uri -
     *    Specify the exact Nepomuk::Resource that needs to be watched. Changes in all its properties
     *    except for meta-properties will be notified through propertyAdded/propertyRemoved.
     *    Notifications will also be sent if any of the watched resources is deleted.
     *
     * 2. By Resource uri and properties -
     *    Specify the exact Nepomuk::Resources and properties. Notifications will only include changes
     *    in those exact properties in the specified resources.
     *
     * 3. By Types -
     *    Specific types may be specified via add/setType. If types are set, then notifications will be
     *    sent for all new resources of that type.
     *
     * 4. By Types and properties -
     *    Both the types and properties may be specified. Notifications will be sent for property changes
     *    in resource with the specified types.
     *
     * \code
     * Nepomuk::ResourceWatcher rw;
     * rw.addResource( res );
     * rw.addProperty( Types::Property(NMM:performer()) );
     * rw.watch();
     * \endcode
     * 
     * \author Vishesh Handa <handa.vish@gmail.com>
     */
    class NEPOMUK_DATA_MANAGEMENT_EXPORT ResourceWatcher : public QObject
    {
        Q_OBJECT
    public:
        ResourceWatcher( QObject* parent = 0 );
        virtual ~ResourceWatcher();

        void addType( const Types::Class & type );
        void addResource( const Nepomuk::Resource & res );
        void addProperty( const Types::Property & property );

        void setTypes( const QList<Types::Class> & types_ );
        void setResources( const QList<Nepomuk::Resource> & resources_ );
        void setProperties( const QList<Types::Property> & properties_ );
        
        QList<Types::Class> types() const;
        QList<Nepomuk::Resource> resources() const;
        QList<Types::Property> properties() const;

    public Q_SLOTS:
        bool start();
        void stop();

    Q_SIGNALS:
        /**
         * This signal is emitted when a new resource is created.
         */
        void resourceCreated( const Nepomuk::Resource & resource, const QList<QUrl>& types );

        /**
         * This signal is emitted when a resource is deleted. 
         */
        void resourceRemoved( const QUrl & uri, const QList<QUrl>& types );

        /**
         * Emitted on addition of any statement of the form -
         * <res> rdf:type <types>, where <types> may be any of the set types
         */
        void resourceTypeAdded( const Nepomuk::Resource & res, const Types::Class & type );

        void resourceTypeRemoved( const Nepomuk::Resource & res, const Types::Class & type );

        /**
         * Emitted when a property is added to the list of resources being watched OR
         * when any resource whose property is being watched is modified.
         */
        void propertyAdded( const Nepomuk::Resource & resource,
                            const Types::Property & property,
                            const QVariant & value );
        void propertyRemoved( const Nepomuk::Resource & resource,
                              const Types::Property & property,
                              const QVariant & value );

    private Q_SLOTS:
        void slotResourceCreated(const QString& res, const QStringList& types);
        void slotResourceRemoved(const QString& res, const QStringList& types);
        void slotResourceTypeAdded(const QString& res, const QString& type);
        void slotResourceTypeRemoved(const QString& res, const QString& type);
        void slotPropertyAdded(const QString& res, const QString& prop, const QVariant& object);
        void slotPropertyRemoved(const QString&, const QString&, const QVariant&);
        
    private:
        class Private;
        Private * d;
    };
}

#endif // RESOURCEWATCHER_H
