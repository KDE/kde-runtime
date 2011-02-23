/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

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

#ifndef DBUSOPERATORS_H
#define DBUSOPERATORS_H

#include <QtDBus/QDBusArgument>
#include <QtCore/QUrl>

#include "simpleresource.h"


QDBusArgument& operator<<( QDBusArgument& arg, const Nepomuk::PropertyHash& ph )
{
    arg.beginMap( QVariant::String, qMetaTypeId<QDBusVariant>());
    for(Nepomuk::PropertyHash::const_iterator it = ph.constBegin();
        it != ph.constEnd(); ++it) {
        arg.beginMapEntry();
        arg << QString::fromAscii(it.key().toEncoded());
        arg << QDBusVariant(it.value());
        arg.endMapEntry();
    }
    arg.endMap();
    return arg;
}

const QDBusArgument& operator>>( const QDBusArgument& arg, Nepomuk::PropertyHash& ph )
{
    ph.clear();
    arg.beginMap();
    while(!arg.atEnd()) {
        QString key;
        QDBusVariant value;
        arg.beginMapEntry();
        arg >> key >> value;
        ph.insert( QUrl::fromEncoded(key.toAscii()), value.variant() );
        arg.endMapEntry();
    }
    arg.endMap();
    return arg;
}

QDBusArgument& operator<<( QDBusArgument& arg, const Nepomuk::SimpleResource& res )
{
    arg.beginStructure();
    arg << QString::fromAscii(res.uri().toEncoded());
    arg << res.properties();
    arg.endStructure();
    return arg;
}

const QDBusArgument& operator>>( const QDBusArgument& arg, Nepomuk::SimpleResource& res )
{
    arg.beginStructure();
    QString uriS;
    Nepomuk::PropertyHash props;
    arg >> uriS;
    res.setUri( QUrl::fromEncoded(uriS.toAscii()) );
    arg >> props;
    res.setProperties(props);
    arg.endStructure();
    return arg;
}

#endif // DBUSOPERATORS_H
