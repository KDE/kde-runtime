/*
   Copyright (C) 2008 by Sebastian Trueg <trueg at kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _NEPOMUK_LEGACY_STORAGE_BRIDGE_H_
#define _NEPOMUK_LEGACY_STORAGE_BRIDGE_H_

#include <Soprano/Server/ServerCore>

namespace Soprano {
    class Model;
    namespace Client {
        class DBusClient;
    }
}

namespace Nepomuk {
    /**
     * Legacy class which makes sure the old org.kde.NepomukServer
     * access to the storage service is still valid for
     * backwards compatibility.
     *
     * \author Sebastian Trueg <trueg@kde.org>
     */
    class LegacyStorageBridge : public Soprano::Server::ServerCore
    {
        Q_OBJECT
    
    public:
        LegacyStorageBridge( QObject* parent = 0 );
        ~LegacyStorageBridge();

        /**
         * Forwards the call to the storage server via the dbus interface.
         */
        Soprano::Model* model( const QString& name );

        /**
         * Forwards the call to the storage server via the dbus interface.
         */
        void removeModel( const QString& name );

        /**
         * Forwards the call to the storage server via the dbus interface.
         */
        QStringList allModels() const;

    private:
        void initClient();
        Soprano::Client::DBusClient* m_client;
    };
}

#endif
