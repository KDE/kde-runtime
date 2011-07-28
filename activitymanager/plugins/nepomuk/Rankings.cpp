/*
 *   Copyright (C) 2011 Ivan Cukic ivan.cukic(at)kde.org
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Rankings.h"
#include "rankingsadaptor.h"

#include <QDBusConnection>
#include <KDebug>

Rankings * Rankings::s_instance = NULL;

inline uint qHash(const Rankings::QueryParams & item)
{
    return qHash(item.activity) ^ qHash(item.application) ^ qHash(item.resourceType);
}

inline bool operator == (const Rankings::QueryParams & l, const Rankings::QueryParams & r)
{
    return l.activity == r.activity && l.application == r.application && l.resourceType == r.resourceType;
}

void Rankings::init(QObject * parent)
{
    if (s_instance) return;

    s_instance = new Rankings(parent);
}

Rankings * Rankings::self()
{
    return s_instance;
}

Rankings::Rankings(QObject * parent)
    : QObject(parent)
{
    kDebug() << "We are in the Rankings";

    QDBusConnection dbus = QDBusConnection::sessionBus();
    new RankingsAdaptor(this);
    dbus.registerObject("/Rankings", this);
}

Rankings::~Rankings()
{
}

void Rankings::registerClient(const QString & client,
        const QString & activity, const QString & application,
        const QString & resourceType)
{
    kDebug() << client;

    QueryParams params = QueryParams(activity, application, resourceType);
    m_queryParamsForClient[client] = params;
    m_clientsForQueryParams[params] << client;
}

void Rankings::deregisterClient(const QString & client)
{
    QueryParams params = m_queryParamsForClient[client];
    m_queryParamsForClient.remove(client);
    m_clientsForQueryParams[params].removeAll(client);
    if (m_clientsForQueryParams[params].isEmpty()) {
        m_clientsForQueryParams.remove(params);
    }
}

void Rankings::setCurrentActivity(const QString & activity)
{

}
