/*
 * Copyright (c) 2010 Ivan Cukic <ivan.cukic(at)kde.org>
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

#ifndef ACTIVITYMANAGER_H
#define ACTIVITYMANAGER_H

#include <kdedmodule.h>
#include <KComponentData>
#include <QStringList>

class KDE_EXPORT ActivityManager : public KDEDModule
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.ActivityManager")

public:
    /**
     * Creates a new ActivityManager
     */
    ActivityManager(QObject *parent, const QList<QVariant>&);

    /**
     * Destroys this ActivityManager
     */
    virtual ~ActivityManager();

public Q_SLOTS:
    /**
     * @returns the list of all existing activities
     */
    QStringList AvailableActivities() const;

    /**
     * @returns the id of the current activity
     */
    QString CurrentActivity() const;

    /**
     * Sets the current activity
     * @param id id of the activity to make current
     * @returns true if successful
     */
    bool SetCurrentActivity(const QString & id);

    /**
     * Adds a new activity
     * @param name name of the activity
     * @returns id of the newly created activity
     */
    QString AddActivity(const QString & name);

    /**
     * Removes the specified activity
     * @param id id of the activity to delete
     */
    void RemoveActivity(const QString & id);

    /**
     * @returns the name of the specified activity
     * @param id id of the activity
     */
    QString ActivityName(const QString & id) const;

    /**
     * Sets the name of the specified activity
     * @param id id of the activity
     * @param name name to be set
     */
    void SetActivityName(const QString & id, const QString & name);

    // Resource related functions

    /**
     * Should be called when the client application
     * opens a new resource identifiable by an uri.
     * @param ID of the window that registers the resource
     * @param uri uri of the resource
     */
    void RegisterResourceWindow(uint wid, const QString & uri);

    /**
     * Should be called when the client application
     * closes a resource previously registered with
     * registerResource.
     * @param ID of the window that unregisters the resource
     * @param uri uri of the resource
     */
    void UnregisterResourceWindow(uint wid, const QString & uri);

    /**
     * @returns the list of activities of a currently
     * registered resource. If back storage (Nepomuk)
     * doesn't exist, the result will contain only
     * the activities that were associated with the
     * specified resource in the current session.
     */
    QStringList ActivitiesForResource(const QString & uri) const;

    /**
     * @returns whether the back store is available
     */
    bool IsBackstoreAvailable() const;

    /**
     * Registers a new activity controller
     * @param service D-Bus service name
     */
    void RegisterActivityController(const QString & service);

    /**
     * @returns the list of registered controllers
     */
    QStringList RegisteredActivityControllers() const;

    // Temporary functions
    QString _allInfo() const;
    QString _serviceIteration() const;

Q_SIGNALS:
    /**
     * This signal is emitted when the global
     * activity is changed
     * @param id id of the new current activity
     */
    void CurrentActivityChanged(const QString & id);

    /**
     * Emitted when an activity name is changed
     */
    void ActivityNameChanged(const QString & id, const QString & name);

protected Q_SLOTS:
    void checkBackstoreAvailability(const QString &, const QString &, const QString &);
    void backstoreIsOnline();
    void backstoreIsOffline();
    void activityControllerUnregistered(const QString &name);

private:
    class Private;
    Private * const d;
};

#endif /*ACTIVITYMANAGER_H*/
