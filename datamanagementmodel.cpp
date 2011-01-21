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

#include "datamanagementmodel.h"

class Nepomuk::DataManagementModel::Private
{
public:

};

Nepomuk::DataManagementModel::DataManagementModel(QObject *parent)
    : Soprano::FilterModel(parent),
      d(new Private())
{
}

Nepomuk::DataManagementModel::~DataManagementModel()
{
    delete d;
}

void Nepomuk::DataManagementModel::addProperty(const QList<QUrl> &resources, const QUrl &property, const QVariantList &values, const QString &app)
{
    // 1. check cardinality (via property cache/tree)
    // 2. if cardinality 1 (we only have 1 and none):
    //    2.1. make sure values.count() == 1 - otherwise fail
    //    2.2. make sure no value exists - otherwise fail
    // 3. create a new graph with all the necessary data and the app
    // 4. check if the app exists, if not create it in the new graph
    // 5. add the new triples to the new graph
    // 6. update resources' mtime
}

void Nepomuk::DataManagementModel::setProperty(const QList<QUrl> &resources, const QUrl &property, const QVariantList &values, const QString &app)
{
    // 1. check cardinality (via property cache/tree)
    // 2. if cardinality 1 (we only have 1 and none):
    //    2.1. make sure values.count() == 1 - otherwise fail
    // 3. get existing values
    // 4. make the diffs between existing and new ones
    // 5. remove the ones that are not valid anymore
    // 6. create a new graph with all the necessary data and the app
    // 7. check if the app exists, if not create it in the new graph
    // 8. add the new triples to the new graph
    // 9. update resources' mtime
}

void Nepomuk::DataManagementModel::removeProperty(const QList<QUrl> &resources, const QUrl &property, const QVariantList &values, const QString &app)
{
    // 1. remove the triples
    // 2. remove trailing graphs
    // 3. update resource mtime
}

void Nepomuk::DataManagementModel::removeProperties(const QList<QUrl> &resources, const QList<QUrl> &properties, const QString &app)
{
    // 1. remove the triples
    // 2. remove trailing graphs
    // 3. update resource mtime
}

QUrl Nepomuk::DataManagementModel::createResource(const QList<QUrl> &types, const QString &label, const QString &description, const QString &app)
{
    // 1. create an new graph
    // 2. check if the app exists, if not create it in the new graph
    // 3. create the new resource in the new graph
    // 4. return the resource's URI
}

void Nepomuk::DataManagementModel::removeResources(const QList<QUrl> &resources, const QString &app, bool force)
{
    Q_UNUSED(app);
    // 1. get all sub-resources and check if they are used by some other resource (not in the list of resources to remove)
    //    for the latter one can use a bif:exists and a !filter(?s in <s>, <s>, ...) - based on the value of force
    // 2. remove the resources and the sub-resources
    // 3. drop trailing graphs (could be optimized by enumerating the graphs up front)
}

void Nepomuk::DataManagementModel::removeDataByApplication(const QList<QUrl> &resources, const QString &app, bool force)
{
}

void Nepomuk::DataManagementModel::removeDataByApplication(const QString &app, bool force)
{
}

void Nepomuk::DataManagementModel::removePropertiesByApplication(const QList<QUrl> &resources, const QList<QUrl> &properties, const QString &app)
{
}
