/* This file is part of the KDE Project
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

#ifndef _SERVICE_MANAGER_H_
#define _SERVICE_MANAGER_H_

#include <QtCore/QObject>
#include <QtDBus/QDBusMessage>

namespace Nepomuk {

    class ServiceController;

    /**
     * Manages all Nepomuk services.
     * Uses ServiceController to control the runtime
     * of each service.
     */
    class ServiceManager : public QObject
    {
        Q_OBJECT

    public:
        ServiceManager( QObject* parent = 0 );
        ~ServiceManager();

        static ServiceManager* self() { return s_self; }
        static void messageFilter( const QDBusMessage& );

        /**
         * Even uninitialized services are running.
         *
         * \return A list of names of all running services.
         *
         * \sa isServiceRunning, isServiceInitialized
         */
        QStringList runningServices() const;

        /**
         * The services that are scheduled to be started but are
         * waiting for dependancies to get initialized.
         */
        QStringList pendingServices() const;

        /**
         * All services that are available in the system. Started
         * and not started. This list does only include valid
         * services, i.e. those that have a proper dependency tree.
         *
         * \return A list of names of all running services.
         */
        QStringList availableServices() const;

        /**
         * \return \p true if the service identified by \p servicename
         * is running and initialized.
         *
         * \sa isServiceRunning
         */
        bool isServiceInitialized( const QString& servicename ) const;

        /**
         * \return \p true if the service identified by \p servicename
         * is running.
         *
         * \sa isServiceInitialized
         */
        bool isServiceRunning( const QString& servicename ) const;

    Q_SIGNALS:
        /**
         * Emitted once a new service finished its initialization and
         * is ready for use.
         */
        void serviceInitialized( const QString& name );

        /**
         * Emitted once a service has been stopped.
         */
        void serviceStopped( const QString& name );

    public Q_SLOTS:
        /**
         * Starts all autoload services.
         */
        void startAllServices();

        /**
         * Stops all services.
         */
        void stopAllServices();

        /**
         * Start a specific service. Blocks until the service
         * and all dependencies are running.
         */
        bool startService( const QString& name );

        /**
         * Stop a specific service. Blocks until the service
         * and all services depending on it have been stopped.
         */
        bool stopService( const QString& name );

        /**
         * \return \p true if the service indicated by \a name
         * is started automatically.
         */
        bool isServiceAutostarted( const QString& name );

        /**
         * Set the service indicated by \a name to be autostarted.
         */
        void setServiceAutostarted( const QString& name, bool autostart );

    private:
        static ServiceManager* s_self;

        class Private;
        Private* const d;

        Q_PRIVATE_SLOT( d, void _k_serviceInitialized(ServiceController*) )
        Q_PRIVATE_SLOT( d, void _k_serviceStopped(ServiceController*) )
    };
}

#endif
