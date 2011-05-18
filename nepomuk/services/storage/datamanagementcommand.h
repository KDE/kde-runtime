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

#ifndef DATAMANAGEMENTCOMMAND_H
#define DATAMANAGEMENTCOMMAND_H

#include <QRunnable>
#include <QtCore/QVariant>
#include <QtDBus/QDBusMessage>

#include "dbustypes.h"
#include "datamanagementmodel.h"
#include "simpleresource.h"
#include "simpleresourcegraph.h"


namespace Nepomuk {

QUrl decodeUrl(const QString& urlsString);
QList<QUrl> decodeUrls(const QStringList& urlStrings);
QString encodeUrl(const QUrl& url);

class DataManagementCommand : public QRunnable
{
public:
    DataManagementCommand(DataManagementModel* model, const QDBusMessage& msg);
    virtual ~DataManagementCommand();

    void run();

    DataManagementModel* model() const { return m_model; }

protected:
    /**
     * Reimplement this method. Return the original value from the call.
     * It will be properly converted automatically.
     * Error handling is done automatically, too.
     * Only method calls to model() are supported.
     */
    virtual QVariant runCommand() = 0;

private:
    DataManagementModel* m_model;
    QDBusMessage m_msg;
};

class AddPropertyCommand : public DataManagementCommand
{
public:
    AddPropertyCommand(const QList<QUrl>& res,
                       const QUrl& property,
                       const QVariantList& values,
                       const QString& app,
                       Nepomuk::DataManagementModel* model,
                       const QDBusMessage& msg)
        : DataManagementCommand(model, msg),
          m_resources(res),
          m_property(property),
          m_values(values),
          m_app(app) {}

private:
    QVariant runCommand() {
        model()->addProperty(m_resources, m_property, m_values, m_app);
        return QVariant();
    }

    QList<QUrl> m_resources;
    QUrl m_property;
    QVariantList m_values;
    QString m_app;
};

class SetPropertyCommand : public DataManagementCommand
{
public:
    SetPropertyCommand(const QList<QUrl>& res,
                       const QUrl& property,
                       const QVariantList& values,
                       const QString& app,
                       Nepomuk::DataManagementModel* model,
                       const QDBusMessage& msg)
        : DataManagementCommand(model, msg),
          m_resources(res),
          m_property(property),
          m_values(values),
          m_app(app) {}

private:
    QVariant runCommand() {
        model()->setProperty(m_resources, m_property, m_values, m_app);
        return QVariant();
    }

    QList<QUrl> m_resources;
    QUrl m_property;
    QVariantList m_values;
    QString m_app;
};

class CreateResourceCommand : public DataManagementCommand
{
public:
    CreateResourceCommand(const QList<QUrl>& resources,
                          const QString& label,
                          const QString& desc,
                          const QString& app,
                          Nepomuk::DataManagementModel* model,
                          const QDBusMessage& msg)
        : DataManagementCommand(model, msg),
          m_resources(resources),
          m_label(label),
          m_desc(desc),
          m_app(app) {}

private:
    QVariant runCommand() {
        return model()->createResource(m_resources, m_label, m_desc, m_app);
    }

    QList<QUrl> m_resources;
    QString m_label;
    QString m_desc;
    QString m_app;
};

class StoreResourcesCommand : public DataManagementCommand
{
public:
    StoreResourcesCommand(const SimpleResourceGraph& resources,
                          const QString& app,
                          const QHash<QUrl, QVariant>& additionalMetadata,
                          Nepomuk::DataManagementModel* model,
                          const QDBusMessage& msg)
        : DataManagementCommand(model, msg),
          m_resources(resources),
          m_app(app),
          m_additionalMetadata(additionalMetadata) {}

private:
    QVariant runCommand() {
        model()->storeResources(m_resources, m_app, m_additionalMetadata);
        return QVariant();
    }

    SimpleResourceGraph m_resources;
    QString m_app;
    QHash<QUrl, QVariant> m_additionalMetadata;
};

class MergeResourcesCommand : public DataManagementCommand
{
public:
    MergeResourcesCommand(const QUrl& resource1,
                          const QUrl& resource2,
                          const QString& app,
                          Nepomuk::DataManagementModel* model,
                          const QDBusMessage& msg)
        : DataManagementCommand(model, msg),
          m_resource1(resource1),
          m_resource2(resource2),
          m_app(app) {}

private:
    QVariant runCommand() {
        model()->mergeResources(m_resource1, m_resource2, m_app);
        return QVariant();
    }

    QUrl m_resource1;
    QUrl m_resource2;
    QString m_app;
};

class RemoveResourcesByApplicationCommand : public DataManagementCommand
{
public:
    RemoveResourcesByApplicationCommand(const QList<QUrl>& res,
                                        const QString& app,
                                        int flags,
                                        Nepomuk::DataManagementModel* model,
                                        const QDBusMessage& msg)
        : DataManagementCommand(model, msg),
          m_resources(res),
          m_app(app),
          m_flags(flags) {}

private:
    QVariant runCommand() {
        model()->removeDataByApplication(m_resources, m_flags, m_app);
        return QVariant();
    }

    QList<QUrl> m_resources;
    QString m_app;
    DataManagementModel::RemovalFlags m_flags;
};

class RemoveDataByApplicationCommand : public DataManagementCommand
{
public:
    RemoveDataByApplicationCommand(const QString& app,
                                   int flags,
                                   Nepomuk::DataManagementModel* model,
                                   const QDBusMessage& msg)
        : DataManagementCommand(model, msg),
          m_app(app),
          m_flags(flags) {}

private:
    QVariant runCommand() {
        model()->removeDataByApplication(m_flags, m_app);
        return QVariant();
    }

    QString m_app;
    DataManagementModel::RemovalFlags m_flags;
};

class RemovePropertiesCommand : public DataManagementCommand
{
public:
    RemovePropertiesCommand(const QList<QUrl>& res,
                            const QList<QUrl>& properties,
                            const QString& app,
                            Nepomuk::DataManagementModel* model,
                            const QDBusMessage& msg)
        : DataManagementCommand(model, msg),
          m_resources(res),
          m_properties(properties),
          m_app(app) {}

private:
    QVariant runCommand() {
        model()->removeProperties(m_resources, m_properties, m_app);
        return QVariant();
    }

    QList<QUrl> m_resources;
    QList<QUrl> m_properties;
    QString m_app;
};

class RemovePropertiesByApplicationCommand : public DataManagementCommand
{
public:
    RemovePropertiesByApplicationCommand(const QList<QUrl>& res,
                                         const QList<QUrl>& properties,
                                         const QString& app,
                                         Nepomuk::DataManagementModel* model,
                                         const QDBusMessage& msg)
        : DataManagementCommand(model, msg),
          m_resources(res),
          m_properties(properties),
          m_app(app) {}

private:
    QVariant runCommand() {
        model()->removePropertiesByApplication(m_resources, m_properties, m_app);
        return QVariant();
    }

    QList<QUrl> m_resources;
    QList<QUrl> m_properties;
    QString m_app;
};

class RemovePropertyCommand : public DataManagementCommand
{
public:
    RemovePropertyCommand(const QList<QUrl>& res,
                          const QUrl& property,
                          const QVariantList& values,
                          const QString& app,
                          Nepomuk::DataManagementModel* model,
                          const QDBusMessage& msg)
        : DataManagementCommand(model, msg),
          m_resources(res),
          m_property(property),
          m_values(values),
          m_app(app) {}

private:
    QVariant runCommand() {
        model()->removeProperty(m_resources, m_property, m_values, m_app);
        return QVariant();
    }

    QList<QUrl> m_resources;
    QUrl m_property;
    QVariantList m_values;
    QString m_app;
};

class RemoveResourcesCommand : public DataManagementCommand
{
public:
    RemoveResourcesCommand(const QList<QUrl>& res,
                           const QString& app,
                           int flags,
                           Nepomuk::DataManagementModel* model,
                           const QDBusMessage& msg)
        : DataManagementCommand(model, msg),
          m_resources(res),
          m_app(app),
          m_flags(flags) {}

private:
    QVariant runCommand() {
        model()->removeResources(m_resources, m_flags, m_app);
        return QVariant();
    }

    QList<QUrl> m_resources;
    QString m_app;
    DataManagementModel::RemovalFlags m_flags;
};

class DescribeResourcesCommand : public DataManagementCommand
{
public:
    DescribeResourcesCommand(const QList<QUrl>& res,
                             bool includeSubResources,
                             Nepomuk::DataManagementModel* model,
                             const QDBusMessage& msg)
        : DataManagementCommand(model, msg),
          m_resources(res),
          m_includeSubResources(includeSubResources) {}

private:
    QVariant runCommand() {
        return QVariant::fromValue(model()->describeResources(m_resources, m_includeSubResources).toList());
    }

    QList<QUrl> m_resources;
    bool m_includeSubResources;
};

class ImportResourcesCommand : public DataManagementCommand
{
public:
    ImportResourcesCommand(const QUrl& url,
                           Soprano::RdfSerialization serialization,
                           const QString& userSerialization,
                           const PropertyHash& additionalProps,
                           const QString& app,
                           Nepomuk::DataManagementModel* model,
                           const QDBusMessage& msg)
        : DataManagementCommand(model, msg),
          m_url(url),
          m_serialization(serialization),
          m_userSerialization(userSerialization),
          m_additionalProperties(additionalProps),
          m_app(app) {}

private:
    QVariant runCommand() {
        model()->importResources(m_url, m_app, m_serialization, m_userSerialization, m_additionalProperties);
        return QVariant();
    }

    QUrl m_url;
    Soprano::RdfSerialization m_serialization;
    QString m_userSerialization;
    PropertyHash m_additionalProperties;
    QString m_app;
};
}

#endif
