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

#ifndef SIMPLERESOURCEGRAPH_H
#define SIMPLERESOURCEGRAPH_H

#include <QtCore/QSharedDataPointer>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QUrl>
#include <QtCore/QMetaType>

#include <KGlobal>

#include "nepomukdatamanagement_export.h"

class KJob;

namespace Nepomuk {
class SimpleResource;

class NEPOMUK_DATA_MANAGEMENT_EXPORT SimpleResourceGraph
{
public:
    SimpleResourceGraph();
    SimpleResourceGraph(const SimpleResource& resource);
    SimpleResourceGraph(const QList<SimpleResource>& resources);
    SimpleResourceGraph(const QSet<SimpleResource>& resources);
    SimpleResourceGraph(const SimpleResourceGraph& other);
    ~SimpleResourceGraph();

    SimpleResourceGraph& operator=(const SimpleResourceGraph& other);

    /**
     * Adds a resource to the graph. An invalid resource will get a
     * new blank node as resource URI.
     */
    void insert(const SimpleResource& res);
    SimpleResourceGraph& operator<<(const SimpleResource& res);

    void remove(const QUrl& uri);
    void remove(const SimpleResource& res);

    void clear();

    int count() const;
    bool isEmpty() const;

    bool contains(const SimpleResource& res) const;
    bool contains(const QUrl& res) const;

    SimpleResource operator[](const QUrl& uri) const;

    QSet<SimpleResource> toSet() const;
    QList<SimpleResource> toList() const;

    QUrl createBlankNode();

    /**
     * Save the graph to the Nepomuk database.
     * \return A job that will perform the saving
     * and emit the result() signal once done.
     * Use the typical KJob error handling methods.
     *
     * \param component The component which should be given to
     * Nepomuk for it to relate the newly created data to it.
     *
     * \sa Nepomuk::storeResources()
     */
    KJob* save(const KComponentData& component = KGlobal::mainComponent()) const;

private:
    class Private;
    QSharedDataPointer<Private> d;
};

NEPOMUK_DATA_MANAGEMENT_EXPORT QDebug operator<<(QDebug& dbg, const Nepomuk::SimpleResourceGraph& graph);
}

#endif
