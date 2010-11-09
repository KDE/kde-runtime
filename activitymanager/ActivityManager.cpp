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

#include "ActivityManager.h"
#include "ActivityManager_p.h"

#include <QUuid>
#include <QDBusConnection>

#include <KConfig>
#include <KConfigGroup>
#include <KCrash>
#include <KUrl>
#include <KDebug>

#include <KWindowSystem>

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Resource>

#include "activitymanageradaptor.h"
#include "EventProcessor.h"

#include "config-features.h"

#ifdef HAVE_NEPOMUK
    #define NEPOMUK_RUNNING d->nepomukInitialized()
#else
    #define NEPOMUK_RUNNING false
#endif

#define ACTIVITIES_PROTOCOL "activities://"

// copied from kdelibs\kdeui\notifications\kstatusnotifieritemdbus_p.cpp
// if there is a common place for such definitions please move
#ifdef Q_OS_WIN64
__inline int toInt(WId wid)
{
	return (int)((__int64)wid);
}

#else
__inline int toInt(WId wid)
{
	return (int)wid;
}
#endif

// Private

ActivityManagerPrivate::ActivityManagerPrivate(ActivityManager * parent)
    : haveSessions(false),
    config("activitymanagerrc"),
    m_nepomukInitCalled(false),
    q(parent)
{
    // Initializing config
    connect(&configSyncTimer, SIGNAL(timeout()),
             this, SLOT(configSync()));

    configSyncTimer.setSingleShot(true);
    configSyncTimer.setInterval(2 * 60 * 1000);

    kDebug() << "reading activities:";
    foreach(const QString & activity, activitiesConfig().keyList()) {
        kDebug() << activity;
        activities[activity] = ActivityManager::Stopped;
    }

    foreach(const QString & activity, mainConfig().readEntry("runningActivities", activities.keys())) {
        kDebug() << "setting" << activity << "as" << "running";
        if (activities.contains(activity)) {
            activities[activity] = ActivityManager::Running;
        }
    }

    currentActivity = mainConfig().readEntry("currentActivity", QString());
    kDebug() << "currentActivity is" << currentActivity;

    connect(KWindowSystem::self(), SIGNAL(windowRemoved(WId)),
            this, SLOT(windowClosed(WId)));

    //listen to ksmserver for starting/stopping
    ksmserverInterface = new QDBusInterface("org.kde.ksmserver", "/KSMServer", "org.kde.KSMServerInterface");
    if (ksmserverInterface->isValid()) {
        ksmserverInterface->setParent(this);
        connect(ksmserverInterface, SIGNAL(subSessionOpened()), this, SLOT(startCompleted()));
        connect(ksmserverInterface, SIGNAL(subSessionClosed()), this, SLOT(stopCompleted()));
        connect(ksmserverInterface, SIGNAL(subSessionCloseCanceled()), this, SLOT(stopCancelled())); //spelling fail :)
        haveSessions = true;
    } else {
        kDebug() << "couldn't connect to ksmserver! session stuff won't work";
        //note: in theory it's nice to try again later
        //but in practice, ksmserver is either there or it isn't (killing it logs you out)
        //so in this case there's no point. :)
        ksmserverInterface->deleteLater();
        ksmserverInterface = 0;
    }

}

ActivityManagerPrivate::~ActivityManagerPrivate()
{
    configSync();
}

void ActivityManagerPrivate::windowClosed(WId windowId)
{
    if (!resourcesForWindow.contains(windowId)) {
        return;
    }

    foreach(const KUrl & uri, resourcesForWindow[windowId]) {
        q->NotifyResourceClosed(toInt(windowId), uri.url());
    }
}

void ActivityManagerPrivate::setActivityState(const QString & id, ActivityManager::State state)
{
    if (activities[id] == state) return;

    kDebug() << "Set the state of" << id << "to" << state;

    /**
     * Treating 'Starting' as 'Running', and 'Stopping' as 'Stopped'
     * as far as the config file is concerned
     */
    bool configNeedsUpdating = ((activities[id] & 4) != (state & 4));

    activities[id] = state;

    switch (state) {
        case ActivityManager::Running:
            kDebug() << "sending ActivityStarted signal";
            emit q->ActivityStarted(id);
            break;

        case ActivityManager::Stopped:
            kDebug() << "sending ActivityStopped signal";
            emit q->ActivityStopped(id);
            break;

        default:
            break;
    }

    kDebug() << "sending ActivityStateChanged signal";
    emit q->ActivityStateChanged(id, state);

    if (configNeedsUpdating) {
        mainConfig().writeEntry("runningActivities",
                activities.keys(ActivityManager::Running) +
                activities.keys(ActivityManager::Starting));
        scheduleConfigSync();
    }
}

KConfigGroup ActivityManagerPrivate::activitiesConfig()
{
    return KConfigGroup(&config, "activities");
}

KConfigGroup ActivityManagerPrivate::mainConfig()
{
    return KConfigGroup(&config, "main");
}

void ActivityManagerPrivate::ensureCurrentActivityIsRunning()
{
    QStringList runningActivities = q->ListActivities(ActivityManager::Running);

    if (!runningActivities.contains(currentActivity)) {
        if (runningActivities.size() > 0) {
            currentActivity = runningActivities.first();
        } else {
            currentActivity.clear();
        }
    }
}

bool ActivityManagerPrivate::setCurrentActivity(const QString & id)
{
    if (id.isEmpty()) {
        currentActivity.clear();

    } else {
        if (!activities.contains(id)) {
            return false;
        }

        q->StartActivity(id);

        currentActivity = id;
        mainConfig().writeEntry("currentActivity", id);

        scheduleConfigSync();
    }

    emit q->CurrentActivityChanged(id);
    return true;
}

QString ActivityManagerPrivate::activityName(const QString & id)
{
    return activitiesConfig().readEntry(id, QString());
}

void ActivityManagerPrivate::scheduleConfigSync()
{
    if (!configSyncTimer.isActive()) {
        configSyncTimer.start();
    }
}

void ActivityManagerPrivate::configSync()
{
    configSyncTimer.stop();
    config.sync();
}

#ifdef HAVE_NEPOMUK
Nepomuk::Resource ActivityManagerPrivate::activityResource(const QString & id) const
{
    kDebug() << "testing for nepomuk";

    if (nepomukInitialized()) {
        return Nepomuk::Resource(KUrl(ACTIVITIES_PROTOCOL + id));
    } else {
        return Nepomuk::Resource();
    }
}

/* lazy init of nepomuk */
bool ActivityManagerPrivate::nepomukInitialized() const
{
    if (m_nepomukInitCalled) return
        Nepomuk::ResourceManager::instance()->initialized();

    m_nepomukInitCalled = true;

    return (Nepomuk::ResourceManager::instance()->init() == 0);
}
#endif // HAVE_NEPOMUK

// Main

ActivityManager::ActivityManager()
    : d(new ActivityManagerPrivate(this))
{

    QDBusConnection dbus = QDBusConnection::sessionBus();
    new ActivityManagerAdaptor(this);
    dbus.registerObject("/ActivityManager", this);

    // TODO: Sync activities in nepomuk with currently existing ones
    // but later, when we are sure nepomuk is running

    // ensureCurrentActivityIsRunning();

    KCrash::setFlags(KCrash::AutoRestart);
}

ActivityManager::~ActivityManager()
{
    delete d;
}

void ActivityManager::Start()
{
    // doing absolutely nothing
}

void ActivityManager::Stop()
{
    d->configSync();
    QCoreApplication::quit();
}

bool ActivityManager::IsBackstoreAvailable() const
{
    return NEPOMUK_RUNNING;
}


// workspace activities control

QString ActivityManager::CurrentActivity() const
{
    return d->currentActivity;
}

bool ActivityManager::SetCurrentActivity(const QString & id)
{
    kDebug() << id;

    if (id.isEmpty()) {
        return false;
    }

    return d->setCurrentActivity(id);
}

QString ActivityManager::AddActivity(const QString & name)
{
    kDebug() << name;

    QString id;

    // Ensuring a new Uuid. The loop should usually end after only
    // one iteration
    QStringList existingActivities = d->activities.keys();
    while (id.isEmpty() || existingActivities.contains(id)) {
        id = QUuid::createUuid();
        id.replace(QRegExp("[{}]"), QString());
    }

    d->setActivityState(id, Invalid);

    SetActivityName(id, name);

    StartActivity(id);

    emit ActivityAdded(id);

    d->configSync();
    return id;
}

void ActivityManager::RemoveActivity(const QString & id)
{
    kDebug() << id;

    if (d->activities.size() < 2 ||
            !d->activities.contains(id)) {
        return;
    }

    // If the activity is running, stash it
    StopActivity(id);

    d->setActivityState(id, Invalid);

    // Removing the activity
    d->activities.remove(id);
    d->activitiesConfig().deleteEntry(id);

    // If the removed activity was the current one,
    // set another activity as current
    if (d->currentActivity == id) {
        d->ensureCurrentActivityIsRunning();
    }

    if (d->transitioningActivity == id) {
        //very unlikely, but perhaps not impossible
        //but it being deleted doesn't mean ksmserver is un-busy..
        //in fact, I'm not quite sure what would happen.... FIXME
        //so I'll just add some output to warn that it happened.
        kDebug() << "deleting activity in transition. watch out!";
    }

    emit ActivityRemoved(id);
    d->configSync();
}

void ActivityManager::StartActivity(const QString & id)
{
    kDebug() << id;

    if (!d->activities.contains(id) ||
            d->activities[id] == Running) {
        return;
    }

    if (!d->transitioningActivity.isEmpty()) {
        kDebug() << "busy!!";
        //TODO: implement a queue instead
        return;
    }

    bool called = false;
    // start the starting :)
    if (d->haveSessions) {
        QDBusInterface kwin("org.kde.kwin", "/KWin", "org.kde.KWin");
        if (kwin.isValid()) {
            QDBusMessage reply = kwin.call("startActivity", id);
            if (reply.type() == QDBusMessage::ErrorMessage) {
                kDebug() << "dbus error:" << reply.errorMessage();
            } else {
                called = true;
                kDebug() << "dbus succeeded";
            }
        } else {
            kDebug() << "couldn't get kwin interface";
        }
    }

    if (called) {
        d->transitioningActivity = id;
        d->setActivityState(id, Starting);
    } else {
        //maybe they use compiz?
        //go ahead without the session
        d->setActivityState(id, Running);
        //and because we skipped "starting", do this manually:
        emit ActivityStarted(id);
    }
    d->configSync(); //force immediate sync
}

void ActivityManagerPrivate::startCompleted()
{
    if (transitioningActivity.isEmpty()) {
        kDebug() << "huh?";
        return;
    }
    setActivityState(transitioningActivity, ActivityManager::Running);
    transitioningActivity.clear();
}

void ActivityManager::StopActivity(const QString & id)
{
    kDebug() << id;

    if (!d->activities.contains(id) ||
            d->activities[id] == Stopped) {
        return;
    }

    if (!d->transitioningActivity.isEmpty()) {
        kDebug() << "busy!!";
        //TODO: implement a queue instead
        return;
    }

    bool called = false;
    // start the stopping :)
    if (d->haveSessions) {
        QDBusInterface kwin("org.kde.kwin", "/KWin", "org.kde.KWin");
        if (kwin.isValid()) {
            QDBusMessage reply = kwin.call("stopActivity", id);
            if (reply.type() == QDBusMessage::ErrorMessage) {
                kDebug() << "dbus error:" << reply.errorMessage();
            } else {
                called = true;
                kDebug() << "dbus succeeded";
            }
        } else {
            kDebug() << "couldn't get kwin interface";
        }
    }

    if (called) {
        d->transitioningActivity = id;
        d->setActivityState(id, Stopping);
    } else {
        //maybe they use compiz?
        //go ahead without the session
        d->setActivityState(id, Stopped);
        if (d->currentActivity == id) {
            d->ensureCurrentActivityIsRunning();
        }
        d->configSync(); //force immediate sync
    }
}

void ActivityManagerPrivate::stopCompleted()
{
    if (transitioningActivity.isEmpty()) {
        kDebug() << "huh?";
        return;
    }
    setActivityState(transitioningActivity, ActivityManager::Stopped);
    if (currentActivity == transitioningActivity) {
        ensureCurrentActivityIsRunning();
    }
    transitioningActivity.clear();
    configSync(); //force immediate sync
}

void ActivityManagerPrivate::stopCancelled()
{
    if (transitioningActivity.isEmpty()) {
        kDebug() << "huh?";
        return;
    }
    setActivityState(transitioningActivity, ActivityManager::Running);
    transitioningActivity.clear();
}

int ActivityManager::ActivityState(const QString & id) const
{
    kDebug() << id << "- is it in" << d->activities << "?";
    if (!d->activities.contains(id)) {
        return Invalid;
    } else {
        kDebug() << "state of" << id << "is" << d->activities[id];
        return d->activities[id];
    }
}

QStringList ActivityManager::ListActivities() const
{
    return d->activities.keys();
}

QStringList ActivityManager::ListActivities(int state) const
{
    return d->activities.keys((State)state);
}

QString ActivityManager::ActivityName(const QString & id) const
{
    return d->activityName(id);
}

void ActivityManager::SetActivityName(const QString & id, const QString & name)
{
    kDebug() << id << name;

    if (!d->activities.contains(id)) {
        return;
    }

    d->activitiesConfig().writeEntry(id, name);

#ifdef HAVE_NEPOMUK
    if (NEPOMUK_RUNNING) {
        d->activityResource(id).setLabel(name);
    }
#endif

    d->scheduleConfigSync();

    kDebug() << "emit ActivityChanged" << id;
    emit ActivityChanged(id);
}

QString ActivityManager::ActivityDescription(const QString & id) const
{
    if (!NEPOMUK_RUNNING || !d->activities.contains(id)) {
        return QString();
    }

#ifdef HAVE_NEPOMUK
    return d->activityResource(id).description();
#endif
}

void ActivityManager::SetActivityDescription(const QString & id, const QString & description)
{
    kDebug() << id << description;

    if (!NEPOMUK_RUNNING || !d->activities.contains(id)) {
        return;
    }

#ifdef HAVE_NEPOMUK
    d->activityResource(id).setDescription(description);
#endif

    kDebug() << "emit ActivityChanged" << id;
    emit ActivityChanged(id);
}

QString ActivityManager::ActivityIcon(const QString & id) const
{
    if (!NEPOMUK_RUNNING || !d->activities.contains(id)) {
        return QString();
    }

#ifdef HAVE_NEPOMUK
    QStringList symbols = d->activityResource(id).symbols();

    if (symbols.isEmpty()) {
        return QString();
    } else {
        return symbols.first();
    }
#else
    return QString();
#endif
}

void ActivityManager::SetActivityIcon(const QString & id, const QString & icon)
{
    kDebug() << id << icon;

    if (!NEPOMUK_RUNNING || !d->activities.contains(id)) {
        return;
    }

#ifdef HAVE_NEPOMUK
    d->activityResource(id).setSymbols(QStringList() << icon);

    kDebug() << "emit ActivityChanged" << id;
    emit ActivityChanged(id);
#endif
}


// Resource related mothods

void ActivityManager::NotifyResourceAccessed(const QString & application, const QString & uri)
{
    EventProcessor::addEvent(application, uri, EventProcessor::Accessed);
}

void ActivityManager::NotifyResourceClosed(uint _windowId, const QString & uri)
{
    WId windowId = (WId)_windowId;
    d->resourcesForWindow[windowId].remove(KUrl(uri));

    EventProcessor::addEvent(d->applicationForWindow[windowId], uri, EventProcessor::Closed);

    if (d->resourcesForWindow[windowId].size() == 0) {
        d->resourcesForWindow.remove(windowId);
        d->applicationForWindow.remove(windowId);
    }
}

void ActivityManager::NotifyResourceModified(uint windowId, const QString & uri)
{
    EventProcessor::addEvent(d->applicationForWindow[(WId)windowId], uri, EventProcessor::Modified);
}

void ActivityManager::NotifyResourceOpened(const QString & application, uint _windowId, const QString & uri)
{
    WId windowId = (WId)_windowId;
    if (!d->applicationForWindow.contains(windowId)) {
        d->applicationForWindow[windowId] = application;
    }

    KUrl kuri(uri);
    d->resourcesForWindow[windowId] << kuri;
    d->activitiesForUrl[kuri] << CurrentActivity();

    EventProcessor::addEvent(application, uri, EventProcessor::Opened);
}

QStringList ActivityManager::ActivitiesForResource(const QString & uri) const
{
    return d->activitiesForUrl.value(uri).toList();
}


