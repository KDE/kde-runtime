/*
   Copyright (c) 2008-2009 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _NEPOMUK_SEARCH_DBUS_OPERATORS_H_
#define _NEPOMUK_SEARCH_DBUS_OPERATORS_H_

#include <QtDBus/QDBusArgument>

#include <nepomuk2/result.h>
#include <nepomuk2/query.h>
#include <nepomuk2/term.h>
#include <nepomuk2/nepomuk_export.h>

Q_DECLARE_METATYPE(Nepomuk2::Query::Result)
Q_DECLARE_METATYPE(Soprano::Node)
Q_DECLARE_METATYPE(QList<Nepomuk2::Query::Result>)
typedef QHash<QString, QString> RequestPropertyMapDBus;
Q_DECLARE_METATYPE( RequestPropertyMapDBus )

namespace Nepomuk2 {
    namespace Query {
        /**
         * Register the DBus types necessary for communication with the Nepomuk
         * query service. This method is only made public for the query service
         * itself which links to this library, too.
         */
        void registerDBusTypes();
    }
}

// We export the non-public operators so that we do not need duplicated code in kdebase
NEPOMUK_EXPORT QDBusArgument& operator<<( QDBusArgument& arg, const Soprano::Node& );
NEPOMUK_EXPORT const QDBusArgument& operator>>( const QDBusArgument& arg, Soprano::Node& );

NEPOMUK_EXPORT QDBusArgument& operator<<( QDBusArgument& arg, const Nepomuk2::Query::Result& );
NEPOMUK_EXPORT const QDBusArgument& operator>>( const QDBusArgument& arg, Nepomuk2::Query::Result& );

#endif
