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

#include "activitymanager.h"
#include "activitymanageradaptor.h"

#include "nepomukactivitiesservice_interface.h"
#include "activitycontroller_interface.h"

#include <stdint.h>

#include <QUuid>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>

#include <KPluginFactory>
#include <KPluginLoader>
#include <KConfig>
#include <KConfigGroup>
#include <KDebug>

#define ActivityManagerServicePath "org.kde.ActivityManager"
#define NepomukActivitiesServicePath "org.kde.nepomuk.services.nepomukactivitiesservice"

K_PLUGIN_FACTORY(ActivityManagerFactory,
                 registerPlugin<ActivityManager>();)
K_EXPORT_PLUGIN(ActivityManagerFactory("activitymanager"))

class ActivityManager::Private
{
public:
    explicit Private()
      : activitiesStore(NULL),
        dbusWatcher(NULL),
        config("activitymanagerrc")
    {
        // loading the list of activities
        availableActivities = activitiesConfig().keyList();

        // loading current activity
        currentActivity = mainConfig().readEntry("currentActivity", QString());
        if (!availableActivities.contains(currentActivity)) {
            currentActivity.clear();
        }
    }

    ~Private()
    {
        config.sync();
    }

    KConfigGroup activitiesConfig()
    {
        return KConfigGroup(&config, "activities");
    }

    KConfigGroup mainConfig()
    {
        return KConfigGroup(&config, "main");
    }


    #define notifyActivityControllers( Signal ) \
        foreach (const QString & service, registeredActivityControllers) {       \
            org::kde::ActivityController client(service, "/ActivityController", \
                    QDBusConnection::sessionBus());                             \
            client. Signal ;                                                    \
        }

    void emitActivityAdded(const QString & id)
    {
        notifyActivityControllers( ActivityAdded(id) );
    }

    void emitActivityRemoved(const QString & id)
    {
        notifyActivityControllers( ActivityRemoved(id) );
    }

    void emitResourceWindowRegistered(uint wid, const QString & uri)
    {
        notifyActivityControllers( ResourceWindowRegistered(wid, uri) );
    }

    void emitResourceWindowUnregistered(uint wid, const QString & uri)
    {
        notifyActivityControllers( ResourceWindowUnregistered(wid, uri) );
    }

    #undef notifyActivityControllers

    QString activityName(const QString & id);

    void initializeStore();

    org::kde::nepomuk::services::NepomukActivitiesService * activitiesStore;
    QDBusConnectionInterface * dbusInterface;
    QDBusServiceWatcher * dbusWatcher;
    QDBusServiceWatcher * controllerServiceWatcher;

    QStringList availableActivities;
    QString currentActivity;
    QStringList registeredActivityControllers;

    QHash < QString, QSet < QString > > resourceActivities;
    QHash < WId, QSet < QString > > resourceWindows;

    KConfig config;
};

ActivityManager::ActivityManager(QObject *parent, const QList<QVariant>&)
        : KDEDModule(parent),
        d(new Private())
{
    // Registering DBus service
    QDBusConnection conn = QDBusConnection::sessionBus();
    d->dbusWatcher = new QDBusServiceWatcher(
          NepomukActivitiesServicePath, conn,
          QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(d->dbusWatcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            this, SLOT(checkBackstoreAvailability(QString,QString,QString)));
    d->controllerServiceWatcher = new QDBusServiceWatcher(this);
    d->controllerServiceWatcher->setConnection(conn);
    d->controllerServiceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(d->controllerServiceWatcher, SIGNAL(serviceUnregistered(QString)),
            this, SLOT(activityControllerUnregistered(QString)));

    if (conn.interface()->isServiceRegistered(ActivityManagerServicePath)) {
        kError() << "already running";
        return;
    }

    new ActivityManagerAdaptor(this);

    conn.interface()->registerService(ActivityManagerServicePath);
    conn.registerObject("/ActivityManager", this);

    // initializing backstore listener
    checkBackstoreAvailability(QString(), QString(), QString());
}

void ActivityManager::checkBackstoreAvailability(
    const QString & service, const QString & oldOwner, const QString & newOwner)
{
    Q_UNUSED(oldOwner)

    bool enabled = false;

    if (service.isEmpty()) {
        enabled = QDBusConnection::sessionBus().interface()->isServiceRegistered(NepomukActivitiesServicePath).value();
    } else {
        enabled = !newOwner.isEmpty();
    }

    if (enabled) {
        backstoreIsOnline();
    } else {
        backstoreIsOffline();
    }
}

void ActivityManager::backstoreIsOnline()
{
    if (d->activitiesStore) return;

    kDebug() << NepomukActivitiesServicePath;

    d->activitiesStore = new org::kde::nepomuk::services::NepomukActivitiesService(
        NepomukActivitiesServicePath, "/nepomukactivitiesservice",
        QDBusConnection::sessionBus()
    );

    QStringList storeActivities = d->activitiesStore->listAvailable();
    kDebug() << "storeActivities" << storeActivities;

    // TODO: Should we delete the activities form the activitiesStore
    // or load the missing ones as existing?
    foreach (const QString & id, storeActivities) {
        if (!d->availableActivities.contains(id)) {
            d->activitiesStore->remove(id);
        }
    }

    foreach (const QString & id, d->availableActivities) {
        if (!storeActivities.contains(id)) {
            d->activitiesStore->add(id, d->activityName(id));
        }
    }
}

void ActivityManager::backstoreIsOffline()
{
    if (!d->activitiesStore) return;

    delete d->activitiesStore;
}

ActivityManager::~ActivityManager()
{
    delete d;
}

QStringList ActivityManager::AvailableActivities() const
{
    return d->availableActivities;
}

QString ActivityManager::CurrentActivity() const
{
    return d->currentActivity;
}

bool ActivityManager::SetCurrentActivity(const QString & id)
{
    if (!d->availableActivities.contains(id)) {
        return false;
    }

    d->currentActivity = id;
    d->mainConfig().writeEntry("currentActivity", id);

    emit CurrentActivityChanged(id);
    return true;
}

QString ActivityManager::AddActivity(const QString & name)
{
    QString id;

    while (id.isEmpty() || d->availableActivities.contains(id)) {
        id = QUuid::createUuid();
        id.replace(QRegExp("[{}]"), QString());
    }

    d->availableActivities << id;

    SetActivityName(id, name);

    d->emitActivityAdded(id);

    return id;
}

void ActivityManager::RemoveActivity(const QString & id)
{
    if (d->availableActivities.size() < 2 ||
            !d->availableActivities.contains(id)) {
        return;
    }

    d->availableActivities.removeAll(id);
    d->activitiesConfig().deleteEntry(id);

    if (d->activitiesStore) {
        d->activitiesStore->remove(id);
    }

    if (d->currentActivity == id) {
        SetCurrentActivity(d->availableActivities.first());
    }

    d->emitActivityRemoved(id);
}

QString ActivityManager::Private::activityName(const QString & id)
{
    return activitiesConfig().readEntry(id, QString());
}

QString ActivityManager::ActivityName(const QString & id) const
{
    return d->activityName(id);
}

void ActivityManager::SetActivityName(const QString & id, const QString & name)
{
    if (!d->availableActivities.contains(id)) {
        return;
    }

    d->activitiesConfig().writeEntry(id, name);

    if (d->activitiesStore) {
        d->activitiesStore->add(id, name);
    }

    emit ActivityNameChanged(id, name);
}

QString ActivityManager::ActivityIcon(const QString & id) const
{
    if (!d->availableActivities.contains(id) ||
            !d->activitiesStore) {
        return QString();
    }

    return d->activitiesStore->icon(id);
}

void ActivityManager::SetActivityIcon(const QString & id, const QString & icon)
{
    if (!d->availableActivities.contains(id) ||
            !d->activitiesStore) {
        return;
    }

    d->activitiesStore->setIcon(id, icon);
}

void ActivityManager::RegisterResourceWindow(uint wid, const QString & uri)
{
    d->resourceWindows[(WId)wid] << uri;
    d->resourceActivities[uri] << CurrentActivity();

    d->emitResourceWindowRegistered(wid, uri);
}

void ActivityManager::UnregisterResourceWindow(uint wid, const QString & uri)
{
    d->resourceWindows[(WId)wid].remove(uri);
    if (uri.isEmpty() || d->resourceWindows[(WId)wid].size() == 0) {
        d->resourceWindows.remove((WId)wid);
    }

    d->emitResourceWindowUnregistered(wid, uri);
}

QStringList ActivityManager::ActivitiesForResource(const QString & uri) const
{
    QSet < QString > result = d->resourceActivities.value(uri);

    if (d->activitiesStore) {
        QStringList fromStore = d->activitiesStore->forResource(uri);
        result += QSet < QString > ::fromList(fromStore);
    }

    return result.toList();
}

bool ActivityManager::IsBackstoreAvailable() const
{
    return d->activitiesStore != NULL;
}

void ActivityManager::RegisterActivityController(const QString & service)
{
    if (!d->registeredActivityControllers.contains(service) &&
        QDBusConnection::sessionBus().interface()->isServiceRegistered(service).value()) {
        kDebug() << "Registering" << service << "as an activity controller";

        org::kde::ActivityController client(service, "/ActivityController",
                QDBusConnection::sessionBus());
        if (client.isValid()) {
            d->registeredActivityControllers << service;
            d->controllerServiceWatcher->addWatchedService(service);
        }
    }
}

QStringList ActivityManager::RegisteredActivityControllers() const
{
    return d->registeredActivityControllers;
}

void ActivityManager::activityControllerUnregistered(const QString &name)
{
    d->registeredActivityControllers.removeAll(name);
}

QString ActivityManager::_allInfo() const
{
    QStringList result;

    {
        QHashIterator < QString, QSet < QString > > i (d->resourceActivities);
        while (i.hasNext()) {
            i.next();

            QStringList list = i.value().toList();
            result << i.key() + ':' + list.join(" ") + '\n';
        }
    }

    {
        QHashIterator < WId, QSet < QString > > i (d->resourceWindows);
        while (i.hasNext()) {
            i.next();

            QStringList list = i.value().toList();
            result << QString::number((intptr_t)i.key()) + ':' + list.join(" ") + '\n';
        }
    }

    return result.join("\n");
}

QString ActivityManager::_serviceIteration() const
{
    return "0.1.0";
}

#include "activitymanager.moc"
