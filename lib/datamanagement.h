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

#ifndef DATAMANAGEMENT_H
#define DATAMANAGEMENT_H

#include <QtCore/QObject>

#include <KComponentData>
#include <KGlobal>

class KJob;

namespace Nepomuk {
    class DescribeResourcesJob;
    class CreateResourceJob;
    class SimpleResourceGraph;

    /**
     * Flags to influence the behaviour of several data mangement
     * methods.
     */
    enum RemovalFlag {
        /// No flags - default behaviour
        NoRemovalFlags = 0x0,

        /**
         * Remove sub resources of the resources specified in the parameters.
         * This will remove sub-resources that are not referenced by any resource
         * which will not be deleted.
         */
        RemoveSubResoures = 0x1
    };
    Q_DECLARE_FLAGS(RemovalFlags, RemovalFlag)

    /**
     * \name Basic API
     */
    //@{
    /**
     * Add \p property with \p values to each resource
     * from \p resources. Existing values will not be touched.
     * If a cardinality is breached an error will be thrown.
     */
    KJob* addProperty(const QList<QUrl>& resources,
                      const QUrl& property,
                      const QVariantList& values,
                      const KComponentData& component = KGlobal::mainComponent());

    /**
     * Set, ie. overwrite properties. Set \p property with
     * \p values for each resource from \p resources. Existing
     * values will be replaced.
     */
    KJob* setProperty(const QList<QUrl>& resources,
                      const QUrl& property,
                      const QVariantList& values,
                      const KComponentData& component = KGlobal::mainComponent());

    /**
     * Remove the property \p property with \p values from each
     * resource in \p resources.
     */
    KJob* removeProperty(const QList<QUrl>& resources,
                         const QUrl& property,
                         const QVariantList& values,
                         const KComponentData& component = KGlobal::mainComponent());

    /**
     * Remove all statements involving any proerty from \p properties from
     * all resources in \p resources.
     */
    KJob* removeProperties(const QList<QUrl>& resources,
                           const QList<QUrl>& properties,
                           const KComponentData& component = KGlobal::mainComponent());

    /**
     * Create a new resource with several \p types.
     */
    CreateResourceJob* createResource(const QList<QUrl>& types,
                                      const QString& label,
                                      const QString& description,
                                      const KComponentData& component = KGlobal::mainComponent());

    /**
     * Remove resources from the database.
     * \param resources The URIs of the resources to be removed.
     * \param app The calling application.
     * \param force Force deletion of the resource and all sub-resources.
     * If false sub-resources will be kept if they are still referenced by
     * other resources.
     */
    KJob* removeResources(const QList<QUrl>& resources,
                          RemovalFlags flags = NoRemovalFlags,
                          const KComponentData& component = KGlobal::mainComponent());
    //@}

    /**
     * \name Advanced API
     */
    //@{
    /**
     * Remove all information about resources from the database which
     * have been created by a specific application.
     * \param resources The URIs of the resources to be removed.
     * \param app The application for which data should be removed.
     * \param force Force deletion of the resource and all sub-resources.
     * If false sub-resources will be kept if they are still referenced by
     * other resources.
     */
    KJob* removeDataByApplication(const QList<QUrl>& resources,
                                  RemovalFlags flags = NoRemovalFlags,
                                  const KComponentData& component = KGlobal::mainComponent());

    /**
     * Remove all information from the database which
     * has been created by a specific application.
     * \param app The application for which data should be removed.
     * \param force Force deletion of the resource and all sub-resources.
     * If false sub-resources will be kept if they are still referenced by
     * resources that have been created by other applications.
     */
    KJob* removeDataByApplication(RemovalFlags flags = NoRemovalFlags,
                                  const KComponentData& component = KGlobal::mainComponent());

    /**
     * Remove all statements involving any proerty from \p properties from
     * all resources in \p resources if it was created by application \p app.
     */
    KJob* removePropertiesByApplication(const QList<QUrl>& resources,
                                        const QList<QUrl>& properties,
                                        const KComponentData& component = KGlobal::mainComponent());

    /**
     * Merges two resources into one. Properties from \p resource1
     * take precedence over that from \p resource2 (for properties with cardinality 1).
     */
    KJob* mergeResources(const QUrl& resource1,
                         const QUrl& resource2,
                         const KComponentData& component = KGlobal::mainComponent());

    /**
     * \param resources The resources to be merged. Blank nodes will be converted into new
     * URIs (unless the corresponding resource already exists).
     * \param app The calling application
     * \param additionalMetadata Additional metadata for the added resources. This can include
     * such details as the creator of the data or details on the method of data recovery.
     * One typical usecase is that the file indexer uses (rdf:type, nrl:DiscardableInstanceBase)
     * to state that the provided information can be recreated at any time. Only built-in types
     * such as int, string, or url are supported.
     */
    KJob* storeResources(const SimpleResourceGraph& resources,
                         const QHash<QUrl, QVariant>& additionalMetadata = QHash<QUrl, QVariant>(),
                         const KComponentData& component = KGlobal::mainComponent());

    /**
     * Describe a set of resources, i.e. retrieve all their properties.
     * \param resources The resource URIs of the resources to describe.
     * \param includeSubResources If \p true sub resources will be included.
     */
    DescribeResourcesJob* describeResources(const QList<QUrl>& resources,
                                            bool includeSubResources);
    //@}
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Nepomuk::RemovalFlags)

#endif
