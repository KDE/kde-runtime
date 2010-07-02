/*
  * This file is part of the KDE project
  * Copyright (C) 2009 Shaun Reich <shaun.reich@kdemail.net>
  * Copyright (C) 2006-2008 Rafael Fernández López <ereslibre@kde.org>
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

#include "progresslistmodel.h"

#include <QDBusServiceWatcher>

#include <KDebug>

#include "jobviewserveradaptor.h"
#include "kuiserveradaptor.h"
#include "jobviewserver_interface.h"
#include "requestviewcallwatcher.h"
#include "uiserver.h"

ProgressListModel::ProgressListModel(QObject *parent)
        : QAbstractItemModel(parent), m_jobId(1),
          m_uiServer(0)
{
    m_serviceWatcher = new QDBusServiceWatcher(this);
    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(m_serviceWatcher, SIGNAL(serviceUnregistered(const QString &)), this, SLOT(serviceUnregistered(const QString &)));

    // Register necessary services and D-Bus adaptors.
    new JobViewServerAdaptor(this);
    new KuiserverAdaptor(this);

    QDBusConnection sessionBus = QDBusConnection::sessionBus();

    if (!sessionBus.registerService(QLatin1String("org.kde.kuiserver"))) {
        kDebug(7024) <<
        "********** Error, we have failed to register service org.kde.kuiserver. Perhaps something  has already taken it?";
    }

    if (!sessionBus.registerService(QLatin1String("org.kde.JobViewServer"))) {
        kDebug(7024) <<
        "********** Error, we have failed to register service JobViewServer. Perhaps something already has taken it?";
    }

    if (!sessionBus.registerObject(QLatin1String("/JobViewServer"), this)) {
        kDebug(7024) <<
        "********** Error, we have failed to register object /JobViewServer.";
    }

    /* unused
    if (m_registeredServices.isEmpty() && !m_uiServer) {
        m_uiServer = new UiServer(this);
    }
    */
}

ProgressListModel::~ProgressListModel()
{
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    sessionBus.unregisterService("org.kde.JobViewServer");
    sessionBus.unregisterService("org.kde.kuiserver");

    qDeleteAll(m_jobViews);
    qDeleteAll(m_registeredServices);

    delete m_uiServer;
}

QModelIndex ProgressListModel::parent(const QModelIndex&) const
{
    return QModelIndex();
}

QDBusObjectPath ProgressListModel::requestView(const QString &appName, const QString &appIconName, int capabilities)
{
    return newJob(appName, appIconName, capabilities);
}

Qt::ItemFlags ProgressListModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsEnabled;
}

int ProgressListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant ProgressListModel::data(const QModelIndex &index, int role) const
{
    QVariant result;

    if (!index.isValid()) {
        return result;
    }

    JobView *jobView = m_jobViews.at(index.row());
    Q_ASSERT(jobView);

    switch (role) {
    case JobView::Capabilities:
        result = jobView->capabilities();
        break;
    case JobView::ApplicationName:
        result = jobView->appName();
        break;
    case JobView::Icon:
        result = jobView->appIconName();
        break;
    case JobView::SizeTotal:
        result = jobView->sizeTotal();
        break;
    case JobView::SizeProcessed:
        result = jobView->sizeProcessed();
        break;
    case JobView::TimeTotal:

        break;
    case JobView::TimeElapsed:

        break;
    case JobView::Speed:
        result = jobView->speed();
        break;
    case JobView::Percent:
        result = jobView->percent();
        break;
    case JobView::InfoMessage:
        result = jobView->infoMessage();
        break;
    case JobView::DescFields:

        break;
    case JobView::State:
        result = jobView->state();
        break;
    case JobView::JobViewRole:
        result = QVariant::fromValue<JobView*>(jobView);
        break;
    default:
        break;
    }

    return result;
}

QModelIndex ProgressListModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    if (row >= m_jobViews.count() || column > 0)
        return QModelIndex();

        return createIndex(row, column);
}

QModelIndex ProgressListModel::indexForJob(JobView *jobView) const
{
    int index = m_jobViews.indexOf(jobView);

    if (index != -1) {
        return createIndex(index, 0, jobView);
    } else {
        return QModelIndex();
    }
}

int ProgressListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_jobViews.count();
}

QDBusObjectPath ProgressListModel::newJob(const QString &appName, const QString &appIcon, int capabilities)
{
    // Since s_jobId is an unsigned int, if we received an overflow and go back to 0,
    // be sure we do not assign 0 to a valid job, 0 is reserved only for
    // reporting problems.
    if (!m_jobId) ++m_jobId;
    JobView *newJob = new JobView(m_jobId);
    ++m_jobId;

    newJob->setAppName(appName);
    newJob->setAppIconName(appIcon);
    newJob->setCapabilities(capabilities);

    beginInsertRows(QModelIndex(), 0, 0);
    m_jobViews.prepend(newJob);
    endInsertRows();

    //The model will now get notified when a job changes -- so it can emit dataChanged(..)
    connect(newJob, SIGNAL(changed(uint)), this, SLOT(jobChanged(uint)));
    connect(newJob, SIGNAL(finished(JobView*)), this, SLOT(jobFinished(JobView*)));
    connect(newJob, SIGNAL(destUrlSet()), this, SLOT(emitJobUrlsChanged()));
    connect(this, SIGNAL(serviceDropped(const QString&)), newJob, SLOT(serviceDropped(const QString&)));

    //Forward this new job over to existing DBus clients.
    foreach(QDBusAbstractInterface* interface, m_registeredServices) {

        QDBusPendingCall pendingCall = interface->asyncCall("requestView", appName, appIcon, capabilities);
        RequestViewCallWatcher *watcher = new RequestViewCallWatcher(newJob, interface->service(), pendingCall, this);
        newJob->pendingCallStarted();

        connect(watcher, SIGNAL(callFinished(RequestViewCallWatcher*)),
                newJob, SLOT(pendingCallFinished(RequestViewCallWatcher*)));
    }

    return newJob->objectPath();
}

QStringList ProgressListModel::gatherJobUrls()
{
    QStringList jobUrls;

    foreach(JobView* jobView, m_jobViews) {
        jobUrls.append(jobView->destUrl().toString());
    }
    return jobUrls;
}

void ProgressListModel::jobFinished(JobView *jobView)
{
        // Job finished, delete it if we are not in self-ui mode, *and* the config option to keep finished jobs is set
        //TODO: does not check for case for the config
        if (!m_uiServer) {
            kDebug(7024) << "removing jobview from list, it finished";
            m_jobViews.removeOne(jobView);
            //job dies, dest. URL's change..
            emit jobUrlsChanged(gatherJobUrls());
        }
}

void ProgressListModel::jobChanged(uint jobId)
{
    emit dataChanged(createIndex(jobId - 1, 0), createIndex(jobId + 1, 0));
    layoutChanged();
}

void ProgressListModel::emitJobUrlsChanged()
{
    emit jobUrlsChanged(gatherJobUrls());
}

void ProgressListModel::registerService(const QString &service, const QString &objectPath)
{
    QDBusConnection sessionBus = QDBusConnection::sessionBus();

    if (!service.isEmpty() && !objectPath.isEmpty()) {
        if (sessionBus.interface()->isServiceRegistered(service).value() &&
                !m_registeredServices.contains(service)) {

            org::kde::JobViewServer *client =
                new org::kde::JobViewServer(service, objectPath, sessionBus);

            if (client->isValid()) {

                if (m_uiServer) {
                    delete m_uiServer;
                    m_uiServer = 0;
                }

                m_serviceWatcher->addWatchedService(service);
                m_registeredServices.insert(service, client);


                //tell this new client to create all of the same jobs that we currently have.
                //also connect them so that when the method comes back, it will return a
                //QDBusObjectPath value, which is where we can contact that job, within "org.kde.JobViewV2"
                //TODO: KDE5 remember to replace current org.kde.JobView interface with the V2 one.. (it's named V2 for compat. reasons).

                //TODO: this falls victim to what newJob used to be vulnerable to...async calls returning too slowly and a terminate ensuing before that.
                // it may not be a problem (yet), though.
                foreach(JobView* jobView, m_jobViews) {

                    QDBusPendingCall pendingCall = client->asyncCall("requestView", jobView->appName(), jobView->appIconName(), jobView->capabilities());

                    RequestViewCallWatcher *watcher = new RequestViewCallWatcher(jobView, service, pendingCall, this);
                    connect(watcher, SIGNAL(callFinished(RequestViewCallWatcher*)),
                            this, SLOT(pendingCallFinished(RequestViewCallWatcher*)));
                }
            } else {
                delete client;
            }
        }
    }
}

bool ProgressListModel::requiresJobTracker()
{
    return m_registeredServices.isEmpty();
}

void ProgressListModel::serviceUnregistered(const QString &name)
{
    m_serviceWatcher->removeWatchedService(name);
    if (m_registeredServices.contains(name)) {

        emit serviceDropped(name);
        m_registeredServices.remove(name);

        /* unused
        if (m_registeredServices.isEmpty()) {
            //the last service dropped, we *need* to show our GUI
            m_uiServer = new UiServer(this);
        }
         */
    }
}

#include "progresslistmodel.moc"
