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

#ifndef SIMPLERESOURCE_H
#define SIMPLERESOURCE_H

#include <QtCore/QUrl>
#include <QtCore/QMultiHash>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>

#include <Soprano/Statement>

#include "nepomukdatamanagement_export.h"

namespace Nepomuk {

typedef QMultiHash<QUrl, QVariant> PropertyHash;

class NEPOMUK_DATA_MANAGEMENT_EXPORT SimpleResource
{
public:
    SimpleResource(const QUrl& uri = QUrl());
    SimpleResource(const SimpleResource& other);
    virtual ~SimpleResource();
    
    SimpleResource& operator=(const SimpleResource& other);

    bool operator==(const SimpleResource& other) const;

    QUrl uri() const;
    void setUri( const QUrl & uri );

    bool contains(const QUrl& property) const;
    bool contains(const QUrl& property, const QVariant& value) const;

    void setProperties(const PropertyHash& properties);
    void addProperty(const QUrl& property, const QVariant& value);

    PropertyHash properties() const;

    QList<Soprano::Statement> toStatementList() const;
    bool isValid() const;

private:
    class Private;
    QSharedDataPointer<Private> d;
};

uint qHash(const SimpleResource& res);
}

#endif
