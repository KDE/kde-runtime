/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

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

#include "dbustypes.h"

#include <QtCore/QStringList>

#include <KUrl>

QString Nepomuk::DBus::convertUri(const QUrl& uri)
{
    return KUrl(uri).url();
}

QStringList Nepomuk::DBus::convertUriList(const QList<QUrl>& uris)
{
    QStringList uriStrings;
    foreach(const QUrl& uri, uris)
        uriStrings << convertUri(uri);
    return uriStrings;
}

QHash<QString, QDBusVariant> Nepomuk::DBus::convertMetadataHash(const QHash<QUrl, QVariant>& metadata)
{
    QHash<QString, QDBusVariant> dbusHash;
    QHash<QUrl, QVariant>::const_iterator end = metadata.constEnd();
    for(QHash<QUrl, QVariant>::const_iterator it = metadata.constBegin();
        it != end; ++it) {
        dbusHash.insertMulti(KUrl(it.key()).url(), QDBusVariant(it.value()));
    }
    return dbusHash;
}

QList<QDBusVariant> Nepomuk::DBus::convertVariantList(const QVariantList& vl)
{
    QList<QDBusVariant> dbusVl;
    foreach(const QVariant& v, vl)
        dbusVl << QDBusVariant(v);
    return dbusVl;
}
