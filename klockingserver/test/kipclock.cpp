/*
    This file is part of the KDE

    Copyright (C) 2009 Tobias Koenig (tokoe@kde.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this library; see the file COPYING. If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kipclock.h"

#include <QtCore/QEventLoop>
#include <QtDBus/QDBusInterface>

#include <QtCore/QDebug>

class KIPCLock::Private
{
    public:
        Private(const QString &resource, KIPCLock *parent)
            : m_resource(resource), m_parent(parent)
        {
            m_interface = new QDBusInterface("org.kde.klockingserver", "/modules/klockingserver");
            QDBusConnection::sessionBus().connect("org.kde.klockingserver", "/modules/klockingserver",
                                                  "org.kde.KLockingServer", "lockGranted", m_parent,
                                                  SLOT(_k_lockGranted(const QString&, const QString&)));
        }

        ~Private()
        {
            delete m_interface;
        }

        void _k_lockGranted(const QString &service, const QString &resource)
        {
            if (service == QDBusConnection::sessionBus().baseService() && resource == m_resource)
                emit m_parent->lockGranted(m_parent);
        }

        QString m_resource;
        KIPCLock *m_parent;
        QDBusInterface *m_interface;
};

KIPCLock::KIPCLock(const QString &resource)
    : d(new Private(resource, this))
{
}

KIPCLock::~KIPCLock()
{
    delete d;
}

QString KIPCLock::resource() const
{
    return d->m_resource;
}

void KIPCLock::lock()
{
    d->m_interface->call("lock", QVariant(d->m_resource));
}

void KIPCLock::unlock()
{
    d->m_interface->call("unlock", QVariant(d->m_resource));
}

void KIPCLock::waitForLockGranted()
{
    QEventLoop loop;
    connect(this, SIGNAL(lockGranted(KIPCLock*)), &loop, SLOT(quit()));
    loop.exec();
}

#include "kipclock.moc"
