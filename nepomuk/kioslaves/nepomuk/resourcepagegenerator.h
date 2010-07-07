/*
   Copyright 2009 Sebastian Trueg <trueg@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _RESOURCE_PAGE_GENERATOR_H_
#define _RESOURCE_PAGE_GENERATOR_H_

#include <Nepomuk/Resource>

#include <QtCore/QList>
#include <KUrl>

class QByteArray;

namespace Nepomuk {
    namespace Types {
        class Entity;
    }

    class ResourcePageGenerator
    {
    public:
        ResourcePageGenerator( const Nepomuk::Resource& res );
        ~ResourcePageGenerator();

        enum Flag {
            NoFlags = 0x0,
            ShowUris = 0x1
        };
        Q_DECLARE_FLAGS( Flags, Flag )

        void setFlags( Flags flags ) {
            m_flags = flags;
        }

        void setFlagsFromUrl( const KUrl& url );

        /**
         * Constructs the URL that represents the current configuration.
         */
        KUrl url() const;

        QByteArray generatePage() const;

    private:
        QString resourceLabel( const Resource& res ) const;
        QString entityLabel( const Nepomuk::Types::Entity& e ) const;
        QString typesToHtml( const QList<QUrl>& types ) const;
        QString encodeUrl( const QUrl& u ) const;
        QString createConfigureBoxHtml() const;

        Nepomuk::Resource m_resource;
        Flags m_flags;
    };
}

Q_DECLARE_OPERATORS_FOR_FLAGS( Nepomuk::ResourcePageGenerator::Flags )

#endif
