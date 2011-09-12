/*****************************************************************************
*   Copyright (C) 2009 Shaun Reich <shaun.reich@kdemail.net>                 *
*   Copyright (C) 2006-2008 Rafael Fernández López <ereslibre@kde.org>       *
*   Copyright (C) 2001 George Staikos <staikos@kde.org>                      *
*   Copyright (C) 2000 Matej Koss <koss@miesto.sk>                           *
*   Copyright (C) 2000 David Faure <faure@kde.org>                           *
*                                                                            *
*   This program is free software; you can redistribute it and/or            *
*   modify it under the terms of the GNU General Public License as           *
*   published by the Free Software Foundation; either version 2 of           *
*   the License, or (at your option) any later version.                      *
*                                                                            *
*   This program is distributed in the hope that it will be useful,          *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*   GNU General Public License for more details.                             *
*                                                                            *
*   You should have received a copy of the GNU General Public License        *
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
*****************************************************************************/

#include "uiserver.h"
#include "jobviewadaptor.h"
#include "jobview_interface.h"
#include "requestviewcallwatcher.h"

#include <klocale.h>
#include <kdebug.h>

#include <QtDBus/QDBusPendingReply>

JobView::JobView(uint jobId, QObject *parent)
    : QObject(parent),
    m_capabilities(-1),
    m_percent(-1),
    m_totalAmount(0),
    m_processAmount(0),
    m_jobId(jobId),
    m_state(Running),
    m_isTerminated(false),
    m_currentPendingCalls(0)
{
    new JobViewV2Adaptor(this);

    m_objectPath.setPath(QString("/JobViewServer/JobView_%1").arg(m_jobId));
    QDBusConnection::sessionBus().registerObject(m_objectPath.path(), this);
}

JobView::~JobView()
{
}

void JobView::terminate(const QString &errorMessage)
{
    QDBusConnection::sessionBus().unregisterObject(m_objectPath.path(), QDBusConnection::UnregisterTree);


    typedef QPair<QString, QDBusAbstractInterface*> iFacePair;
    foreach(const iFacePair &pair, m_objectPaths) {
        kDebug(7024) << "making async call of terminate for: " << pair.first;
        pair.second->asyncCall(QLatin1String("terminate"), errorMessage);
    }

    m_error = errorMessage;

    if (m_currentPendingCalls < 1) {
        // if hit it means a job exists for *something* but can't be terminated properly
        // because the async call to create the job didn't come back fast enough.
        // (thus addJobContact wasn't called before this was hit).
//        Q_ASSERT(!m_objectPaths.isEmpty());

        // no more calls waiting. Lets mark ourselves for deletion.
        emit finished(this);
    }

    m_isTerminated = true;
}

void JobView::requestSuspend()
{
    emit suspendRequested();
}

void JobView::requestResume()
{
    emit resumeRequested();
}

void JobView::requestCancel()
{
    emit cancelRequested();
}

void JobView::setSuspended(bool suspended)
{
    typedef QPair<QString, QDBusAbstractInterface*> iFacePair;
    foreach(const iFacePair &pair, m_objectPaths) {
        pair.second->asyncCall(QLatin1String("setSuspended"), suspended);
    }

    m_state = suspended ? Suspended : Running;
    emit changed(m_jobId);
}

uint JobView::state() const
{
    return m_state;
}

void JobView::setTotalAmount(qulonglong amount, const QString &unit)
{
    typedef QPair<QString, QDBusAbstractInterface*> iFacePair;
    foreach(const iFacePair &pair, m_objectPaths) {
        pair.second->asyncCall(QLatin1String("setTotalAmount"), amount, unit);
    }

    m_totalAmount = amount;
    m_totalUnit = unit;

    if (unit == "bytes") {
        m_sizeTotal = amount ? KGlobal::locale()->formatByteSize(amount) : QString();

    } else if (unit == "files") {
        m_sizeTotal = amount ? i18np("%1 file", "%1 files", amount) : QString();

    } else if (unit == "dirs") {
        m_sizeTotal = amount ? i18np("%1 folder", "%1 folders", amount) : QString();

    }
    emit changed(m_jobId);
}

QString JobView::sizeTotal() const
{
    return m_sizeTotal;
}

void JobView::setProcessedAmount(qulonglong amount, const QString &unit)
{
    typedef QPair<QString, QDBusAbstractInterface*> iFacePair;
    foreach(const iFacePair &pair, m_objectPaths) {
        pair.second->asyncCall(QLatin1String("setProcessedAmount"), amount, unit);
    }

    m_processAmount = amount;
    m_processUnit = unit;

    if (unit == "bytes") {
        m_sizeProcessed = amount ? KGlobal::locale()->formatByteSize(amount) : QString();

    } else if (unit == "files") {
        m_sizeProcessed = amount ? i18np("%1 file", "%1 files", amount) : QString();

    } else if (unit == "dirs") {
        m_sizeProcessed = amount ? i18np("%1 folder", "%1 folders", amount) : QString();
    }
    emit changed(m_jobId);
}

QString JobView::sizeProcessed() const
{
    return m_sizeProcessed;
}

void JobView::setPercent(uint value)
{
    typedef QPair<QString, QDBusAbstractInterface*> iFacePair;
    foreach(const iFacePair &pair, m_objectPaths) {
        pair.second->asyncCall(QLatin1String("setPercent"), value);
    }

    m_percent = value;
    emit changed(m_jobId);
}

uint JobView::percent() const
{
    return m_percent;
}

void JobView::setSpeed(qulonglong bytesPerSecond)
{
    typedef QPair<QString, QDBusAbstractInterface*> iFacePair;
    foreach(const iFacePair &pair, m_objectPaths) {
        pair.second->asyncCall(QLatin1String("setSpeed"), bytesPerSecond);
    }

    m_speed = bytesPerSecond ? KGlobal::locale()->formatByteSize(bytesPerSecond) : QString();
    emit changed(m_jobId);
}

QString JobView::speed() const
{
    return m_speed;
}

void JobView::setInfoMessage(const QString &infoMessage)
{
    typedef QPair<QString, QDBusAbstractInterface*> iFacePair;
    foreach(const iFacePair &pair, m_objectPaths) {
        pair.second->asyncCall(QLatin1String("setInfoMessage"), infoMessage);
    }

    m_infoMessage = infoMessage;
    emit changed(m_jobId);
}

QString JobView::infoMessage() const
{
    return m_infoMessage;
}

bool JobView::setDescriptionField(uint number, const QString &name, const QString &value)
{
    typedef QPair<QString, QDBusAbstractInterface*> iFacePair;
    foreach(const iFacePair &pair, m_objectPaths) {
        pair.second->asyncCall(QLatin1String("setDescriptionField"), number, name, value);
    }

    if (m_descFields.contains(number)) {
        m_descFields[number].first = name;
        m_descFields[number].second = value;
    } else {
        QPair<QString, QString> tempDescField(name, value);
        m_descFields.insert(number, tempDescField);
    }
    emit changed(m_jobId);
    return true;
}

void JobView::clearDescriptionField(uint number)
{
    typedef QPair<QString, QDBusAbstractInterface*> iFacePair;
    foreach(const iFacePair &pair, m_objectPaths) {
        pair.second->asyncCall(QLatin1String("clearDescriptionField"), number);
    }

    if (m_descFields.contains(number)) {
        m_descFields.remove(number);
    }
    emit changed(m_jobId);
}

void JobView::setAppName(const QString &appName)
{
    typedef QPair<QString, QDBusAbstractInterface*> iFacePair;
    foreach(const iFacePair &pair, m_objectPaths) {
        pair.second->asyncCall(QLatin1String("setAppName"), appName);
    }

    m_applicationName = appName;
}

QString JobView::appName() const
{
    return m_appIconName;
}

void JobView::setAppIconName(const QString &appIconName)
{
    typedef QPair<QString, QDBusAbstractInterface*> iFacePair;
    foreach(const iFacePair &pair, m_objectPaths) {
        pair.second->asyncCall(QLatin1String("setAppIconName"), appIconName);
    }

    m_appIconName = appIconName;
}

QString JobView::appIconName() const
{
    return m_appIconName;
}

void JobView::setCapabilities(int capabilities)
{
    typedef QPair<QString, QDBusAbstractInterface*> iFacePair;
    foreach(const iFacePair &pair, m_objectPaths) {
        pair.second->asyncCall(QLatin1String("setCapabilities"), capabilities);
    }

    m_capabilities = capabilities;
}

int JobView::capabilities() const
{
    return m_capabilities;
}

QString JobView::error() const
{
    return m_error;
}

uint JobView::jobId() const
{
    return m_jobId;
}

QDBusObjectPath JobView::objectPath() const
{
    return m_objectPath;
}

void JobView::setDestUrl(const QDBusVariant &destUrl)
{
    m_destUrl = destUrl.variant();
    emit destUrlSet();
}

QVariant JobView::destUrl() const
{
    return m_destUrl;
}

void JobView::addJobContact(const QString& objectPath, const QString& address)
{
    org::kde::JobViewV2 *client =
            new org::kde::JobViewV2(address, objectPath, QDBusConnection::sessionBus());

    QPair<QString, QDBusAbstractInterface*> pair(objectPath, client);

    //propagate any request signals from the client's job, up to us, then to the parent KJob
    //otherwise e.g. the pause button on plasma's tray would be broken.
    connect(client, SIGNAL(suspendRequested()), this, SIGNAL(suspendRequested()));
    connect(client, SIGNAL(resumeRequested()), this, SIGNAL(resumeRequested()));
    connect(client, SIGNAL(cancelRequested()), this, SIGNAL(cancelRequested()));
    Q_ASSERT(!m_objectPaths.contains(address));
    m_objectPaths.insert(address, pair);

    //If the job already has any information, send it to the contact
    if (m_capabilities > -1) {
        client->asyncCall(QLatin1String("setCapabilities"), m_capabilities);
    }

    if (!m_applicationName.isEmpty()) {
        client->asyncCall(QLatin1String("setAppName"), m_applicationName);
    }

    if (!m_appIconName.isEmpty()) {
        client->asyncCall(QLatin1String("setAppIconName"), m_appIconName);
    }

    if (m_percent > -1) {
        client->asyncCall(QLatin1String("setPercent"), m_percent);
    }

    if (!m_infoMessage.isEmpty()) {
        client->asyncCall(QLatin1String("setInfoMessage"), m_infoMessage);
    }

    if (!m_descFields.isEmpty()) {
        QHashIterator<uint, QPair <QString, QString > > i(m_descFields);
        while (i.hasNext()) {
            i.next();
            client->asyncCall(QLatin1String("setDescriptionField"), i.key(), i.value().first, i.value().second);
        }
    }

    if (m_state == Suspended) {
        client->asyncCall(QLatin1String("setSuspended"), true);
    }

    if (m_processAmount > 0) {
        client->asyncCall(QLatin1String("setProcessedAmount"), m_processAmount, m_processUnit);
    }

    if (m_totalAmount > 0) {
        client->asyncCall(QLatin1String("setTotalAmount"), m_totalAmount, m_totalUnit);
    }
}

void JobView::pendingCallStarted()
{
    ++m_currentPendingCalls;
}

void JobView::pendingCallFinished(RequestViewCallWatcher* watcher)
{
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;

    // note: this is the *remote* jobview objectpath, not the kuiserver one.
    QDBusObjectPath objectPath = reply.argumentAt<0>();
    QString address = watcher->service();

    Q_ASSERT(!reply.isError());
    Q_ASSERT(reply.isValid());

    --m_currentPendingCalls;

    if (m_isTerminated) {

        // do basically the same as terminate() except only for service
        // since this one missed out.

        org::kde::JobViewV2 *client = new org::kde::JobViewV2(address, objectPath.path(), QDBusConnection::sessionBus());

        kDebug(7024) << "making async terminate call to objectPath: " << objectPath.path();
        kDebug(7024) << "this was because a pending call was finished, but the job was already terminated before it returned.";
        kDebug(7024) << "current pending calls left: " << m_currentPendingCalls;

        // forcibly set the percent (should be 100). Since the job missed out on that too.
        client->asyncCall(QLatin1String("setPercent"), m_percent);
        client->asyncCall(QLatin1String("terminate"), m_error);

        if (m_currentPendingCalls < 1) {
            kDebug() << "no more async calls left pending..emitting finished so we can have ourselves deleted.";
            emit finished(this);
        }
    } else {
        // add this job contact because we are _not_ just terminating here.
        // we'll need it for regular things like speed changes, etc.
        kDebug(7024) << "adding job contact for address: " << address << " objectPath: " << objectPath.path();
        addJobContact(objectPath.path(), address);
    }
}

void JobView::serviceDropped(const QString &address)
{
    m_objectPaths.remove(address);
    --m_currentPendingCalls;
}

#include "jobview.moc"
