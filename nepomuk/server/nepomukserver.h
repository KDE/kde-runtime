/* This file is part of the KDE Project
   Copyright (c) 2007 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_SERVER_H_
#define _NEPOMUK_SERVER_H_

#include <QtCore/QObject>

#include <KSharedConfig>

namespace Soprano {
    class Backend;
}

namespace Nepomuk {

    class ServiceManager;

    class Server : public QObject
    {
        Q_OBJECT

    public:
        Server( QObject* parent = 0 );
        virtual ~Server();

        KSharedConfig::Ptr config() const;

        static Server* self();

    public Q_SLOTS:
        void enableNepomuk(bool enabled);
        void enableStrigi(bool enabled);
        bool isNepomukEnabled() const;
        bool isStrigiEnabled() const;

        /**
         * \return the name of the default data repository.
         */
        QString defaultRepository() const;
        void reconfigure();
        void quit();

    private:
        void init();

        ServiceManager* m_serviceManager;
        bool m_enabled;

        KSharedConfigPtr m_config;

        const QString m_strigiServiceName;

        static Server* s_self;
    };
}

#endif
