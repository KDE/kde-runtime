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

#include "simpleresource.h"

namespace Nepomuk {
class DataManagementModel : public Soprano::FilterModel
{
    Q_OBJECT

public:
    DataManagementModel(Soprano::Model* model, QObject *parent = 0);
    ~DataManagementModel();

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
                         const QString& app,
                         bool force);
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
                                 const QString& app,
                                 bool force);

    /**
     * Remove all information from the database which
     * has been created by a specific application.
     * \param app The application for which data should be removed.
     * \param force Force deletion of the resource and all sub-resources.
     * If false sub-resources will be kept if they are still referenced by
     * resources that have been created by other applications.
     */
    void removeDataByApplication(const QString& app,
                                 bool force);

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
    void mergeResources(const SimpleResourceGraph& resources,
                        const QString& app,
                        const QHash<QUrl, QVariant>& additionalMetadata);

    /**
     * Describe a set of resources, i.e. retrieve all their properties.
     * \param resources The resource URIs of the resources to describe.
     * \param includeSubResources If \p true sub resources will be included.
     */
    SimpleResourceGraph describeResources(const QList<QUrl>& resources,
                                          bool includeSubResources);
    //@}

private:
    QUrl createGraph(const QString& app, const QHash<QUrl, QVariant>& additionalMetadata);

    /**
     * Updates the modification date of \p resource to \p date.
     * Adds the new statement in \p graph
     */
    Soprano::Error::ErrorCode updateModificationDate( const QUrl& resource, const QUrl& graph, const QDateTime& date = QDateTime::currentDateTime() );

    /**
     * Removes all the graphs from \p graphs which do not contain any statements
     */
    void removeTrailingGraphs( const QSet<QUrl> graphs );
    
    enum UriType {
        GraphUri,
        ResourceUri
    };
    QUrl createUri(UriType type);

    class Private;
    Private* const d;
};
}

#endif
