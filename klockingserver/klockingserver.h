/*
    This file is part of the KDE Locking Server

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

#ifndef KLOCKINGSERVER_H
#define KLOCKINGSERVER_H

#include <QtCore/QMap>
#include <QtDBus/QtDBus>

#include <kdedmodule.h>

class KLockingServer : public KDEDModule, protected QDBusContext
{
    Q_OBJECT

    public:
        KLockingServer(QObject* parent, const QList<QVariant>&);
        ~KLockingServer();

    public Q_SLOTS:
        /**
         * This method is called whenever a client tries to gain the
         * lock for the given @p resource.
         */
        void lock(const QString &resource);

        /**
         * This method is called whenever a client releases the
         * lock for the given @p resource.
         */
        void unlock(const QString &resource);

    Q_SIGNALS:
        /**
         * This dbus signal is emitted whenever the lock for @p resource
         * has been granted to the @p service.
         */
        void lockGranted(const QString &service, const QString &resource);

    private Q_SLOTS:
        /**
         * This slot is called whenever a @p client has been disappeared from
         * the dbus system.
         */
        void clientUnregistered(const QString &client, const QString &oldOwner, const QString &newOwner);

    private:
        /**
         * The entry of the lock table.
         */
        struct LockTableEntry
        {
            /**
             * The current owner of the lock.
             */
            QString owner;

            /**
             * The list of clients that are waiting for
             * the lock to be unlocked.
             */
            QStringList queue;
        };

        /**
         * The lock table.
         *
         * The key is the resource name and the values the
         * lock table entries.
         */
        QMap<QString, LockTableEntry> m_lockTable;

        /**
         * A hash table for fast lookup whether a dbus client is
         * one of our clients.
         */
        QSet<QString> m_clients;
};

#endif
