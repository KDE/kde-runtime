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

#include <QtCore/QString>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>

#include <QDebug>

DBusInterface::DBusInterface(QObject* parent):
    QObject(parent),
    m_iface(0),
    m_service(QString()),
    m_path(QString()),
    m_interfaceName(QString())
{
}

QDBusInterface* DBusInterface::interface()
{
    if (!m_iface) {
        m_iface = new QDBusInterface(m_service, m_path, m_interfaceName);
    }
    
    return m_iface;
}

const QString& DBusInterface::service() const
{
    return m_service;
}

const QString& DBusInterface::path() const
{
    return m_path;
}

const QString& DBusInterface::interfaceName() const
{
    return m_interfaceName;
}

void DBusInterface::setService(const QString& service)
{
    m_service = service;
}

void DBusInterface::setPath(const QString& path)
{
    m_path = path;
}

void DBusInterface::setInterfaceName(const QString& interface)
{
    m_interfaceName = interface;
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
    return interface()->asyncCall(method, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
}

QDBusMessage DBusInterface::call(
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
    return interface()->call(method, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
}


#include "dbusinterface.moc"
