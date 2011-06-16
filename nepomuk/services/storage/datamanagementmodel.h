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

#ifndef DATAMANAGEMENTMODEL_H
#define DATAMANAGEMENTMODEL_H

#include <Soprano/FilterModel>

#include <QtCore/QDateTime>

namespace Nepomuk {

class ClassAndPropertyTree;
class ResourceMerger;
class SimpleResourceGraph;

class DataManagementModel : public Soprano::FilterModel
{
    Q_OBJECT

public:
    DataManagementModel(ClassAndPropertyTree* tree, Soprano::Model* model, QObject *parent = 0);
    ~DataManagementModel();

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

public Q_SLOTS:
    /**
     * \name Basic API
     */
    //@{
    /**
     * Add \p property with \p values to each resource
     * from \p resources. Existing values will not be touched.
     * If a cardinality is breached an error will be thrown.
     */
    void addProperty(const QList<QUrl>& resources,
                     const QUrl& property,
                     const QVariantList& values,
                     const QString& app);

    /**
     * Set, ie. overwrite properties. Set \p property with
     * \p values for each resource from \p resources. Existing
     * values will be replaced.
     */
    void setProperty(const QList<QUrl>& resources,
                     const QUrl& property,
                     const QVariantList& values,
                     const QString& app);

    /**
     * Remove the property \p property with \p values from each
     * resource in \p resources.
     */
    void removeProperty(const QList<QUrl>& resources,
                        const QUrl& property,
                        const QVariantList& values,
                        const QString& app);

    /**
     * Remove all statements involving any proerty from \p properties from
     * all resources in \p resources.
     */
    void removeProperties(const QList<QUrl>& resources,
                          const QList<QUrl>& properties,
                          const QString& app);

    /**
     * Create a new resource with several \p types.
     */
    QUrl createResource(const QList<QUrl>& types,
                        const QString& label,
                        const QString& description,
                        const QString& app);

    /**
     * Remove resources from the database.
     * \param resources The URIs of the resources to be removed.
     * \param app The calling application.
     * \param force Force deletion of the resource and all sub-resources.
     * If false sub-resources will be kept if they are still referenced by
     * other resources.
     */
    void removeResources(const QList<QUrl>& resources,
                         RemovalFlags flags,
                         const QString& app);
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
    void removeDataByApplication(const QList<QUrl>& resources,
                                 RemovalFlags flags,
                                 const QString& app);

    /**
     * Remove all information from the database which
     * has been created by a specific application.
     * \param app The application for which data should be removed.
     * \param force Force deletion of the resource and all sub-resources.
     * If false sub-resources will be kept if they are still referenced by
     * resources that have been created by other applications.
     */
    void removeDataByApplication(RemovalFlags flags,
                                 const QString& app);

    /**
     * Remove all statements involving any proerty from \p properties from
     * all resources in \p resources if it was created by application \p app.
     */
    void removePropertiesByApplication(const QList<QUrl>& resources,
                                       const QList<QUrl>& properties,
                                       const QString& app);

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
    void storeResources(const SimpleResourceGraph& resources,
                        const QString& app,
                        const QHash<QUrl, QVariant>& additionalMetadata = QHash<QUrl, QVariant>() );

    /**
     * Merges two resources into one. Properties from \p resource1
     * take precedence over that from \p resource2 (for properties with cardinality 1).
     */
    void mergeResources(const QUrl& resource1, const QUrl& resource2, const QString& app);

    /**
     * Import an RDF graph from a URL.
     * \param url The url from which the graph should be loaded. This does not have to be local.
     * \param app The calling application
     * \param serialization The RDF serialization used for the file. If Soprano::SerializationUnknown a crude automatic
     * detection based on file extension is used.
     * \param userSerialization If \p serialization is Soprano::SerializationUser this value is used. See Soprano::Parser
     * for details.
     * \param additionalMetadata Additional metadata for the added resources. This can include
     * such details as the creator of the data or details on the method of data recovery.
     * One typical usecase is that the file indexer uses (rdf:type, nrl:DiscardableInstanceBase)
     * to state that the provided information can be recreated at any time. Only built-in types
     * such as int, string, or url are supported.
     */
    void importResources(const QUrl& url, const QString& app,
                         Soprano::RdfSerialization serialization,
                         const QString& userSerialization = QString(),
                         const QHash<QUrl, QVariant>& additionalMetadata = QHash<QUrl, QVariant>());

    /**
     * Describe a set of resources, i.e. retrieve all their properties.
     * \param resources The resource URIs of the resources to describe.
     * \param includeSubResources If \p true sub resources will be included.
     */
    SimpleResourceGraph describeResources(const QList<QUrl>& resources,
                                          bool includeSubResources) const;
    //@}

private:
    QUrl createGraph(const QString& app = QString(), const QHash<QUrl, QVariant>& additionalMetadata = QHash<QUrl, QVariant>());
    QUrl createGraph(const QString& app, const QMultiHash<QUrl, Soprano::Node>& additionalMetadata);

    /**
     * Splits \p graph into two. This essentially copies the graph metadata to a new graph and metadata graph pair.
     * The newly created graph will set as being maintained by \p appRes.
     *
     * \param graph The graph that should be split/copied.
     * \param metadataGraph The metadata graph of graph. This can be empty.
     * \param appRes The application resource which will be added as maintaining the newly created graph. Can be empty.
     *
     * \return The URI of the newly created graph.
     */
    QUrl splitGraph(const QUrl& graph, const QUrl& metadataGraph, const QUrl& appRes);

    QUrl findApplicationResource(const QString& app, bool create = true);

    /**
     * Updates the modification date of \p resource to \p date.
     * Adds the new statement in \p graph.
     */
    Soprano::Error::ErrorCode updateModificationDate( const QUrl& resource, const QUrl& graph, const QDateTime& date = QDateTime::currentDateTime(), bool includeCreationDate = false );

    /**
     * Updates the modification date of \p resources to \p date.
     * Adds the new statement in \p graph.
     */
    Soprano::Error::ErrorCode updateModificationDate( const QSet<QUrl>& resources, const QUrl& graph, const QDateTime& date = QDateTime::currentDateTime(), bool includeCreationDate = false );

    /**
     * Removes all the graphs from \p graphs which do not contain any statements
     */
    void removeTrailingGraphs( const QSet<QUrl>& graphs );

    /**
     * Adds for each resource in \p resources a property for each node in nodes. \p nodes cannot be empty.
     * This method is used in the public setProperty and addProperty slots to avoid a lot of code duplication.
     *
     * \param resources A hash mapping the resources provided by the client to the actual resource URIs. This hash is created via resolveUrls() and can
     *                  contain empty values which means that the resource corresponding to a file URL does not exist yet.
     *                  This hash cannot be empty.
     * \param property The property to use. This cannot be empty.
     * \param nodes A hash mapping value nodes as created via resolveNodes from the output of ClassAndPropertyTree::variantToNodeSet. Like \p resources
     *              this hash might contain empty values which refer to non-existing file resources. This cannot be empty.
     * \param app The calling application.
     */
    void addProperty(const QHash<QUrl, QUrl>& resources, const QUrl& property, const QHash<Soprano::Node, Soprano::Node>& nodes, const QString& app);

    /**
     * Checks if resource \p res actually exists. A resource exists if any information other than the standard metadata
     * (nao:created, nao:creator, nao:lastModified, nao:userVisible) or the nie:url is defined.
     */
    bool doesResourceExist(const QUrl& res, const QUrl& graph = QUrl()) const;

    /**
     * Resolves a local file url to its resource URI. Returns \p url if it is not a file URL and
     * an empty QUrl in case \p url is a file URL but has no resource URI in the model.
     *
     * \param statLocalFiles If \p true the method will check if local files exist and set an error
     * if not.
     */
    QUrl resolveUrl(const QUrl& url, bool statLocalFiles = false) const;

    /**
     * Resolves local file URLs through nie:url.
     * \return a Hash mapping \p urls to their actual resource URIs or an empty QUrl if the resource does not exist.
     *
     * This method does check if the local file exists and may set an error.
     */
    QHash<QUrl, QUrl> resolveUrls(const QList<QUrl>& urls) const;

    /**
     * Resolves local file URLs through nie:url.
     * \return a Hash mapping \p nodes to the nodes that should actually be added to the model or an empty node if the resource for a file URL
     * does not exist yet.
     *
     * This method does check if the local file exists and may set an error.
     */
    QHash<Soprano::Node, Soprano::Node> resolveNodes(const QSet<Soprano::Node>& nodes) const;

    /**
     * Updates the nie:url of a local file resource.
     * \return \p true if the url has been updated and nothing else needs to be done, \p false
     * if the update was not handled. An error also results in a return value of \p true.
     *
     * \param resource The resource to update. Both file URLs and resource URIs are supported. Thus, there is no need to resolve the URL
     * before calling this method.
     * \param nieUrl The new nie:url to assign to the resource.
     */
    bool updateNieUrlOnLocalFile(const QUrl& resource, const QUrl& nieUrl);

    ClassAndPropertyTree * classAndPropertyTree();
    
    enum UriType {
        GraphUri,
        ResourceUri
    };
    QUrl createUri(UriType type);

    /**
     * Checks if any of the provided resources has a protected type (class, property, graph), ie. one
     * of the resources should not be changed through the standard API.
     *
     * If the method returns \p true it has already set an appropriate error.
     */
    bool containsResourceWithProtectedType(const QSet<QUrl>& resources) const;

    class Private;
    Private* const d;

    friend class ResourceMerger;
};
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Nepomuk::DataManagementModel::RemovalFlags)

#endif
