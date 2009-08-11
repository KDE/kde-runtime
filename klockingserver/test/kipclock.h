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

#ifndef KIPCLOCK_H
#define KIPCLOCK_H

#include <QtCore/QObject>

/**
 * @short A class for serializing access to a resource via IPC.
 *
 * This class can be used to serialize access to a resource between
 * multiple applications. Instead of using lock files, which could
 * become stale easily, the KDE Locking Server is used to serialize
 * the access. KIPCLock encapsulates the communication with the locking
 * server.
 *
 * Example:
 *
 * @code
 *
 * KIPCLock *lock = new KIPCLock("myresource");
 * connect(lock, SIGNAL(lockGranted(KIPCLock *lock)), this, SLOT(doCriticlTask(KIPCLock *lock)));
 * lock->lock();
 *
 * ...
 *
 * ... ::doCriticalTask(KIPCLock *lock)
 * {
 *    // change common resource
 *
 *    lock->unlock();
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class KIPCLock : public QObject
{
    Q_OBJECT

    public:
        /**
         * Creates a new ipc lock object.
         *
         * @param resource The identifier of the resource that shall be locked.
         *                 This identifier can be any string, however it must be unique for
         *                 the resource and every client that wants to access the resource must
         *                 know it.
         */
        KIPCLock(const QString &resource);

        /**
         * Destroys the ipc lock object.
         */
        ~KIPCLock();

        /**
         * Returns the identifier of the resource the lock is set on.
         */
        QString resource() const;

        /**
         * Requests the lock.
         *
         * The lock is granted as soon as the lockGranted() signal is emitted.
         */
        void lock();

        /**
         * Releases the lock.
         *
         * @note This method should be called as soon as the critical area is left
         *       in your code path and the lock no longer needed.
         */
        void unlock();

        /**
         * Waits for the granting of a lock by starting an internal event loop.
         */
        void waitForLockGranted();

    Q_SIGNALS:
        /**
         * This signal is emitted when the requested lock has been granted.
         *
         * @param lock The lock that has been granted.
         */
        void lockGranted(KIPCLock *lock);

    private:
        class Private;
        Private* const d;

        Q_PRIVATE_SLOT(d, void _k_lockGranted(const QString&, const QString&))
};

#endif
