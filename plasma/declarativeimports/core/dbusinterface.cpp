/*
    Copyright (C) 2011  Luiz Romário Santana Rios <luizromario@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "dbusinterface.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>

DBusInterface::DBusInterface(QObject* parent): QObject(parent)
{

}

const QString& DBusInterface::service() const
{
    return m_service;
}

const QString& DBusInterface::path() const
{
    return m_path;
}

const QString& DBusInterface::interface() const
{
    return m_interface;
}

void DBusInterface::setService(const QString& service)
{
    m_service = service;
}

void DBusInterface::setPath(const QString& path)
{
    m_path = path;
}

void DBusInterface::setInterface(const QString& interface)
{
    m_interface = interface;
}

QDBusPendingCall DBusInterface::asyncCall(
    const QString& method,
    const QVariant& arg1,
    const QVariant& arg2,
    const QVariant& arg3,
    const QVariant& arg4,
    const QVariant& arg5,
    const QVariant& arg6,
    const QVariant& arg7,
    const QVariant& arg8
)
{
    if (!m_iface) {
        m_iface = new QDBusInterface(m_service, m_path, m_interface, QDBusConnection::sessionBus(), this);
    }
    
    return m_iface->asyncCall(method, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
}
