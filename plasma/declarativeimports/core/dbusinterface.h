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

#ifndef DBUSINTERFACE_H
#define DBUSINTERFACE_H

#include <QtCore/QObject>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusPendingCall>

class QDBusInterface;

class DBusInterface : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString service READ service WRITE setService)
    Q_PROPERTY(QString path READ path WRITE setPath)
    Q_PROPERTY(QString interfaceName READ interfaceName WRITE setInterfaceName)
    
    QDBusInterface *m_iface;
    bool m_ifaceParamsChanged;
    QString m_service, m_path, m_interfaceName;
    
protected:
    QDBusInterface *interface();
    
public:    
    DBusInterface(QObject *parent = 0);
    
    const QString &service() const;
    const QString &path() const;
    const QString &interfaceName() const;
    void setService(const QString &service);
    void setPath(const QString &path);
    void setInterfaceName(const QString &interface);
    
    Q_INVOKABLE QDBusPendingCall asyncCall(
        const QString & method,
        const QVariant & arg1 = QVariant(),
        const QVariant & arg2 = QVariant(),
        const QVariant & arg3 = QVariant(),
        const QVariant & arg4 = QVariant(),
        const QVariant & arg5 = QVariant(),
        const QVariant & arg6 = QVariant(),
        const QVariant & arg7 = QVariant(),
        const QVariant & arg8 = QVariant()
    );
    Q_INVOKABLE QDBusMessage call(
        const QString & method,
        const QVariant & arg1 = QVariant(),
        const QVariant & arg2 = QVariant(),
        const QVariant & arg3 = QVariant(),
        const QVariant & arg4 = QVariant(),
        const QVariant & arg5 = QVariant(),
        const QVariant & arg6 = QVariant(),
        const QVariant & arg7 = QVariant(),
        const QVariant & arg8 = QVariant()
    );
};

#endif // DBUSINTERFACE_H
