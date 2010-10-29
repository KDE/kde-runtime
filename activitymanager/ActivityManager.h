/*
 *   Copyright (C) 2010 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef ACTIVITY_MANAGER_H_
#define ACTIVITY_MANAGER_H_

#define ActivityManagerServicePath "org.kde.ActivityManager"

#include <QObject>
#include <QString>
#include <QStringList>

class ActivityManagerPrivate;

/**
 * Service for tracking the user actions and managing the
 * activities
 */
class ActivityManager: public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.ActivityManager")

public:


    /**
     * Activity state
     * @note: Do not change the values, needed for bit-operations
     */
    enum State {
        Invalid  = 0,
        Running  = 2,
        Starting = 3,
        Stopped  = 4,
        Stopping = 5
    };

    /**
     * Creates new ActivityManager
     */
    ActivityManager();

    /**
     * Destroys this interface
     */
    virtual ~ActivityManager();



// service control methods
public Q_SLOTS:
    /**
     * Does nothing. If the service is not running, the D-Bus daemon
     * will automatically create it
     */
    void Start();

    /**
     * Stops the service
     */
    void Stop();



// workspace activities control
public Q_SLOTS:
    /**
     * @returns the id of the current activity, empty string if none
     */
    QString CurrentActivity() const;

    /**
     * Sets the current activity
     * @param id id of the activity to make current
     */
    bool SetCurrentActivity(const QString & id);

    /**
     * Adds a new activity
     * @param name name of the activity
     * @returns id of the newly created activity
     */
    QString AddActivity(const QString & name);

    /**
     * Starts the specified activity
     * @param id id of the activity to stash
     */
    void StartActivity(const QString & id);

    /**
     * Stops the specified activity
     * @param id id of the activity to stash
     */
    void StopActivity(const QString & id);

    /**
     * @returns the state of the activity
     * @param id id of the activity
     */
    int ActivityState(const QString & id) const;

    /**
     * Removes the specified activity
     * @param id id of the activity to delete
     */
    void RemoveActivity(const QString & id);

    /**
     * @returns the list of all existing activities
     */
    QStringList ListActivities() const;

    /**
     * @returns the list of activities with the specified state
     * @param state state
     */
    QStringList ListActivities(int state) const;

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

    /**
     * @returns the description of the specified activity
     * @param id id of the activity
     */
    QString ActivityDescription(const QString & id) const;

    /**
     * Sets the description of the specified activity
     * @param id id of the activity
     * @param description description to be set
     */
    void SetActivityDescription(const QString & id, const QString & description);

    /**
     * @returns the icon of the specified activity
     * @param id id of the activity
     */
    QString ActivityIcon(const QString & id) const;

    /**
     * Sets the icon of the specified activity
     * @param id id of the activity
     * @param icon icon to be set
     */
    void SetActivityIcon(const QString & id, const QString & icon);


    /**
     * @returns whether the backstore (Nepomuk) is available
     */
    bool IsBackstoreAvailable() const;

Q_SIGNALS:
    /**
     * This signal is emitted when the global
     * activity is changed
     * @param id id of the new current activity
     */
    void CurrentActivityChanged(const QString & id);

    /**
     * This signal is emitted when a new activity is created
     * @param id id of the activity
     */
    void ActivityAdded(const QString & id);

    /**
     * This signal is emitted when an activity is started
     * @param id id of the activity
     */
    void ActivityStarted(const QString & id);

    /**
     * This signal is emitted when an activity is stashed
     * @param id id of the activity
     */
    void ActivityStopped(const QString & id);

    /**
     * This signal is emitted when an activity is deleted
     * @param id id of the activity
     */
    void ActivityRemoved(const QString & id);

    /**
     * Emitted when an activity name, description and/or icon are changed
     * @param id id of the changed activity
     */
    void ActivityChanged(const QString & id);

    /**
     * Emitted when the state of activity is changed
     */
    void ActivityStateChanged(const QString & id, int state);

// Resource related mothods
public Q_SLOTS:
    /**
     * Should be called when the client application accesses
     * the file but is not interested in registering more advanced
     * events like open/modify/close.
     * @param application unformatted name of the application
     * @param uri uri of the resource
     */
    void NotifyResourceAccessed(const QString & application, const QString & uri);

    /**
     * Should be called when the client application
     * opens a new resource identifiable by an uri.
     * @param application unformatted name of the application
     * @param windowId ID of the window that registers the resource
     * @param uri uri of the resource
     */
    void NotifyResourceOpened(const QString & application, uint windowId, const QString & uri);

    /**
     * Should be called when the client application
     * modifies a resource already registered with NotifyResourceOpened
     * @param windowId ID of the window that registers the resource
     * @param uri uri of the resource
     */
    void NotifyResourceModified(uint windowId, const QString & uri);

    /**
     * Should be called when the client application
     * closes a resource previously registered with
     * NotifyResourceOpened.
     * @param ID of the window that unregisters the resource
     * @param uri uri of the resource
     */
    void NotifyResourceClosed(uint windowId, const QString & uri);

    /**
     * @returns the list of activities that are associated with
     * the specified resource
     * @param uri uri of the resource
     */
    QStringList ActivitiesForResource(const QString & uri) const;

Q_SIGNALS:
    /**
     * @see NotifyResourceAccessed
     */
    void ResourceAccessed(const QString & application, const QString & uri);

    /**
     * @see NotifyResourceOpened
     */
    void ResourceOpened(const QString & application, uint windowId, const QString & uri);

    /**
     * @see NotifyResourceModified
     */
    void ResourceModified(uint windowId, const QString & uri);

    /**
     * @see NotifyResourceClosed
     */
    void ResourceClosed(uint windowId, const QString & uri);



private:
    friend class ActivityManagerPrivate;
    class ActivityManagerPrivate * const d;
};

#endif // ACTIVITY_MANAGER_H_
