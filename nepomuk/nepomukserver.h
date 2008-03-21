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

#include <kdedmodule.h>

#include <KSharedConfig>

namespace Soprano {
    class Backend;
}

namespace Nepomuk {

    class Core;
    class StrigiController;
    class OntologyLoader;

    class Server : public KDEDModule
    {
        Q_OBJECT
        Q_CLASSINFO("Bus Interface", "org.kde.NepomukServer")

    public:
        Server();
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

    private Q_SLOTS:
        void slotNepomukCoreInitialized( bool success );

    private:
        void init();
        void startNepomuk();
        void startStrigi();
        void updateStrigiConfig();

        Core* m_core;
        StrigiController* m_strigiController;
        OntologyLoader* m_ontologyLoader;

        bool m_restartStrigiAfterInitialization;

        KSharedConfigPtr m_config;

        static Server* s_self;
    };
}

#endif
