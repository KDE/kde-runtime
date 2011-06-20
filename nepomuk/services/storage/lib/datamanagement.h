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

#include <Soprano/Global>

#include "nepomukdatamanagement_export.h"

class KJob;
class KUrl;

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
        NoRemovalFlags = 0,

        /**
         * Remove sub resources of the resources specified in the parameters.
         * This will remove sub-resources that are not referenced by any resource
         * which will not be deleted.
         */
        RemoveSubResoures = 1
    };
    Q_DECLARE_FLAGS(RemovalFlags, RemovalFlag)

    /**
     * The identification mode used by storeResources().
     * This states which given resources should be merged
     * with existing ones that match.
     */
    enum StoreIdentificationMode {
        /// This is the default mode. Only new resources without a resource URI are identified. All others
        /// are just saved with their given URI
        IdentifyNew = 0,

        /// Try to identify all resources, even if they have a resource URI. This only differs
        /// from IdentifyNew for resources that have a resource URI that does NOT exists yet.
        IdentifyAll = 1,

        /// All resources are treated as new ones. The only exception are those with a defined
        /// resource URI.
        IdentifyNone = 2
    };

    /**
     * Flags to influence the behaviour of storeResources()
     */
    enum StoreResourcesFlag {
        /// No flags - default behaviour
        NoStoreResourcesFlags = 0,

        /// By default storeResources() will only append data and fail if properties with
        /// cardinality 1 already have a value. This flag changes the behaviour to force the
        /// new values instead.
        OverwriteProperties = 1
    };
    Q_DECLARE_FLAGS(StoreResourcesFlags, StoreResourcesFlag)

    /**
     * \name Basic API
     */
    //@{
    /**
     * Add \p property with \p values to each resource
     * from \p resources. Existing values will not be touched.
     * If a cardinality is breached an error will be thrown.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* addProperty(const QList<QUrl>& resources,
                                                     const QUrl& property,
                                                     const QVariantList& values,
                                                     const KComponentData& component = KGlobal::mainComponent());

    /**
     * Set, ie. overwrite properties. Set \p property with
     * \p values for each resource from \p resources. Existing
     * values will be replaced.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* setProperty(const QList<QUrl>& resources,
                                                     const QUrl& property,
                                                     const QVariantList& values,
                                                     const KComponentData& component = KGlobal::mainComponent());

    /**
     * Remove the property \p property with \p values from each
     * resource in \p resources.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* removeProperty(const QList<QUrl>& resources,
                                                        const QUrl& property,
                                                        const QVariantList& values,
                                                        const KComponentData& component = KGlobal::mainComponent());

    /**
     * Remove all statements involving any proerty from \p properties from
     * all resources in \p resources.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* removeProperties(const QList<QUrl>& resources,
                                                          const QList<QUrl>& properties,
                                                          const KComponentData& component = KGlobal::mainComponent());

    /**
     * Create a new resource with several \p types.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT CreateResourceJob* createResource(const QList<QUrl>& types,
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
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* removeResources(const QList<QUrl>& resources,
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
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* removeDataByApplication(const QList<QUrl>& resources,
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
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* removeDataByApplication(RemovalFlags flags = NoRemovalFlags,
                                                                 const KComponentData& component = KGlobal::mainComponent());

    /**
     * Merges two resources into one. Properties from \p resource1
     * take precedence over that from \p resource2 (for properties with cardinality 1).
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* mergeResources(const QUrl& resource1,
                                                        const QUrl& resource2,
                                                        const KComponentData& component = KGlobal::mainComponent());

    /**
     * \param resources The resources to be merged. Blank nodes will be converted into new
     * URIs (unless the corresponding resource already exists).
     * \param identificationMode This method can try hard to avoid duplicate resources by looking
     * for already existing duplicates based on nrl:IdentifyingProperty. By default it only looks
     * for duplicates of resources that do not have a resource URI (SimpleResource::uri()) defined.
     * This behaviour can be changed with this parameter.
     * \param flags Additional flags to change the behaviour of the method.
     * \param additionalMetadata Additional metadata for the added resources. This can include
     * such details as the creator of the data or details on the method of data recovery.
     * One typical usecase is that the file indexer uses (rdf:type, nrl:DiscardableInstanceBase)
     * to state that the provided information can be recreated at any time. Only built-in types
     * such as int, string, or url are supported.
     * \param component The component representing the calling service/application. Normally there
     * is no need to change this from the default.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* storeResources(const SimpleResourceGraph& resources,
                                                        StoreIdentificationMode identificationMode = IdentifyNew,
                                                        StoreResourcesFlags flags = NoStoreResourcesFlags,
                                                        const QHash<QUrl, QVariant>& additionalMetadata = QHash<QUrl, QVariant>(),
                                                        const KComponentData& component = KGlobal::mainComponent());

    /**
     * Import an RDF graph from a URL.
     * \param url The url from which the graph should be loaded. This does not have to be local.
     * \param serialization The RDF serialization used for the file. If Soprano::SerializationUnknown a crude automatic
     * detection based on file extension is used.
     * \param userSerialization If \p serialization is Soprano::SerializationUser this value is used. See Soprano::Parser
     * for details.
     * \param identificationMode This method can try hard to avoid duplicate resources by looking
     * for already existing duplicates based on nrl:IdentifyingProperty. By default it only looks
     * for duplicates of resources that do not have a resource URI (SimpleResource::uri()) defined.
     * This behaviour can be changed with this parameter.
     * \param flags Additional flags to change the behaviour of the method.
     * \param additionalMetadata Additional metadata for the added resources. This can include
     * such details as the creator of the data or details on the method of data recovery.
     * One typical usecase is that the file indexer uses (rdf:type, nrl:DiscardableInstanceBase)
     * to state that the provided information can be recreated at any time. Only built-in types
     * such as int, string, or url are supported.
     * \param component The component representing the calling service/application. Normally there
     * is no need to change this from the default.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT KJob* importResources(const KUrl& url,
                                                         Soprano::RdfSerialization serialization,
                                                         const QString& userSerialization = QString(),
                                                         StoreIdentificationMode identificationMode = IdentifyNew,
                                                         StoreResourcesFlags flags = NoStoreResourcesFlags,
                                                         const QHash<QUrl, QVariant>& additionalMetadata = QHash<QUrl, QVariant>(),
                                                         const KComponentData& component = KGlobal::mainComponent());

    /**
     * Describe a set of resources, i.e. retrieve all their properties.
     * \param resources The resource URIs of the resources to describe.
     * \param includeSubResources If \p true sub resources will be included.
     */
    NEPOMUK_DATA_MANAGEMENT_EXPORT DescribeResourcesJob* describeResources(const QList<QUrl>& resources,
                                                                           bool includeSubResources);
    //@}
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Nepomuk::RemovalFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(Nepomuk::StoreResourcesFlags)

#endif
