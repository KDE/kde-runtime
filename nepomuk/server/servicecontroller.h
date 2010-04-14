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

#ifndef _SERVICE_CONTROLLER_H_
#define _SERVICE_CONTROLLER_H_

#include <QtCore/QObject>
#include <KService>

namespace Nepomuk {
    class ServiceController : public QObject
    {
        Q_OBJECT

    public:
        ServiceController( KService::Ptr service, QObject* parent );
        ~ServiceController();

        KService::Ptr service() const;

        /**
         * The name of the service. This equals
         * service()->desktopEntryName().
         */
        QString name() const;

        /**
         * All the service's direct dependencies.
         */
        QStringList dependencies() const;

        bool autostart() const;
        bool startOnDemand() const;
        bool runOnce() const;

        void setAutostart( bool enable );

        /**
         * Make sure the service is running. This will attach to an already running
         * instance or simple return \p true in case the service has been started
         * already.
         */
        bool start();
        void stop();

        bool isRunning() const;
        bool isInitialized() const;

        /**
         * Wait for the service to become initialized.
         * Will return immeadetely if the service has
         * not been started or is already initialized.
         *
         * A service is initialized once it is registered
         * with D-Bus.
         */
        bool waitForInitialized( int timeout = 30000 );

    Q_SIGNALS:
        /**
         * Emitted once the service has been initialized
         * properly, i.e. once its D-Bus interface is active.
         */
        void serviceInitialized( ServiceController* );
        
    private Q_SLOTS:
        void slotProcessFinished( bool );
        void slotServiceRegistered( const QString& serviceName );
        void slotServiceUnregistered( const QString& serviceName );
        void slotServiceInitialized( bool success );
        
    private:
        void createServiceControlInterface();

        class Private;
        Private* const d;
    };
}

#endif
