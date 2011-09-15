/*  This file is part of the KDE project

    Copyright (c) 2011 Lamarque V. Souza <lamarque@gmail.com>

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
    License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#include "wicdstatus.h"
#include "wicdcustomtypes.h"

#include <QtDBus/QDBusReply>
#include <QDBusMetaType>

#include <KDebug>

#define WICD_DBUS_SERVICE "org.wicd.daemon"
#define WICD_DAEMON_DBUS_PATH "/org/wicd/daemon"
#define WICD_DAEMON_DBUS_INTERFACE "org.wicd.daemon"

WicdStatus::WicdStatus( QObject *parent )
    : SystemStatusInterface( parent ),
      m_wicd( WICD_DBUS_SERVICE,
                 WICD_DAEMON_DBUS_PATH,
                 WICD_DAEMON_DBUS_INTERFACE,
                 QDBusConnection::systemBus() ),
      cachedState(Solid::Networking::Unknown)
{
    qDBusRegisterMetaType<WicdConnectionInfo>();
    QDBusConnection::systemBus().connect(WICD_DBUS_SERVICE, WICD_DAEMON_DBUS_PATH, WICD_DAEMON_DBUS_INTERFACE,
                                         "StatusChanged", this, SLOT(wicdStateChanged()));
    wicdStateChanged();
}

Solid::Networking::Status WicdStatus::status() const
{
    return cachedState;
}

bool WicdStatus::isSupported() const
{
    return m_wicd.isValid();
}

void WicdStatus::wicdStateChanged()
{
    Solid::Networking::Status status = Solid::Networking::Unknown;
    QDBusMessage message = m_wicd.call("GetConnectionStatus");

    if (message.arguments().count() == 0) {
        emit statusChanged( status );
        return;
    }

    if (!message.arguments().at(0).isValid()) {
        emit statusChanged( status );
        return;
    }

    WicdConnectionInfo s;
    message.arguments().at(0).value<QDBusArgument>() >> s;
    kDebug() << "State: " << s.status << " Info: " << s.info;

    switch (static_cast<Wicd::ConnectionStatus>(s.status)) {
    case Wicd::CONNECTING:
        status = Solid::Networking::Connecting;
        break;
    case Wicd::WIRED:
    case Wicd::WIRELESS:
        status = Solid::Networking::Connected;
        break;
    case Wicd::NOT_CONNECTED:
        status = Solid::Networking::Unconnected;
        break;
    default:
    case Wicd::SUSPENDED:
        status = Solid::Networking::Unknown;
        break;
    }

    emit statusChanged( status );
}

#include "wicdstatus.moc"
