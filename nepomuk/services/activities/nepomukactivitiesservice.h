/*
 * Copyright (c) 2010 Ivan Cukic <ivan.cukic(at)kde.org>
 * Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef NEPOMUK_ACTIVITIES_SERVICE_H
#define NEPOMUK_ACTIVITIES_SERVICE_H

#include <Nepomuk/Service>
#include <Nepomuk/Resource>

#include <QtCore/QVariant>
#include <QtCore/QUrl>

#include <KConfigGroup>

/**
 * Nepomuk service for storing workspace activities
 */
class NepomukActivitiesService: public Nepomuk::Service {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.nepomuk.services.NepomukActivitiesService")

public:
    /**
     * Creates a new ActivitiesService
     */
    NepomukActivitiesService(QObject* parent, const QVariantList & args);

    /**
     * Destroys this ActivitiesService
     */
    ~NepomukActivitiesService();

public Q_SLOTS:
    /**
     * @returns the list of all activities
     */
    QStringList listAvailable() const;

    /**
     * Creates a new activity. If the activity already
     * exists, only the name is set
     * @param activityId id to be used
     * @param activityName name of the activity
     */
    void add(const QString & activityId,
            const QString & activityName);

    /**
     * Remove the specified activity
     * @param activityId id of the activity to delete
     */
    void remove(const QString & activityId);

    /**
     * @returns the name of the specified activity
     * @param activityId id of the activity
     */
    QString name(const QString & activityId) const;

    /**
     * Sets the name of the specified activity
     * @param activityId id of the activity
     * @param name name to be set
     */
    void setName(const QString & activityId, const QString & name);

    /**
     * @returns the icon of the specified activity
     * @param activityId id of the activity
     */
    QString icon(const QString & activityId) const;

    /**
     * Sets the icon of the specified activity.
     * @param activityId id of the activity
     * @param icon path or freedesktop.org icon name
     */
    void setIcon(const QString & activityId, const QString & icon);

    /**
     * @returns the Nepomuk URI of the specified activity
     * @param activityId id of the activity
     */
    QString resourceUri(const QString & activityId) const;

    /**
     * @returns activity uri for the specified activity
     * @param activityId id of the activity
     */
    QString uri(const QString & activityId) const;

    /**
     * Links the resource and the activity. (Adds 'is related to' link
     * from the resource to the activity)
     * @param activityID id of the activity
     * @param resourceUri Nepomuk resource URI
     * @param typeUri type to assign to the resource to be linked
     */
    void associateResource(const QString & activityID,
        const QString & resourceUri, const QString & typeUri = QString());

    /**
     * Unlinks the resource and the activity. (Removes 'is related to' link
     * from the resource to the activity)
     * @param activityID id of the activity
     * @param resourceUri Nepomuk resource URI
     */
    void disassociateResource(const QString & activityID,
        const QString & resourceUri);

    /**
     * @returns the list of resources that are related to
     * the specified activity
     * @param activityId id of the activity
     */
    QStringList associatedResources(const QString & activityId, const QString & resourceType = QString()) const;

    /**
     * @returns the list of activities associated with the
     * specified resource
     */
    QStringList forResource(const QString & uri) const;

    /**
     * This function is for debugging purposes only,
     * and will be deleted
     */
    void _deleteAll();

    /**
     * This function is for debugging purposes only,
     * and will be deleted
     */
    QString _serviceIteration() const;

private:
    Soprano::Model * m_model;

    Nepomuk::Resource activityResource(const QString & activityId) const;
    QString activityId(const Nepomuk::Resource & resource) const;

    QString urlForResource(const Nepomuk::Resource & resource) const;

};

#endif // NEPOMUK_ACTIVITIES_SERVICE_H
