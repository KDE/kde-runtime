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

#include "klockingserver.h"

#include <kpluginfactory.h>
#include <kpluginloader.h>

#include "klockingserveradaptor.h"

K_PLUGIN_FACTORY(KLockingServerFactory, registerPlugin<KLockingServer>();)
K_EXPORT_PLUGIN(KLockingServerFactory("klockingserver"))

KLockingServer::KLockingServer(QObject* parent, const QList<QVariant>&)
 : KDEDModule(parent)
{
    new KLockingServerAdaptor(this);

    // register separately from kded
    QDBusConnection::sessionBus().registerService("org.kde.klockingserver");

    connect(QDBusConnection::sessionBus().interface(), SIGNAL(serviceOwnerChanged(const QString&, const QString&, const QString&)),
            this, SLOT(clientUnregistered(const QString&, const QString&, const QString&)));
}

KLockingServer::~KLockingServer()
{
}

void KLockingServer::lock(const QString &resource)
{
    const QString client = message().service();

    // check if the resource is locked already
    if (!m_lockTable.contains(resource)) {
        // it is not, so add the current client to the lock table

        LockTableEntry entry;
        entry.owner = client;

        m_lockTable.insert(resource, entry);

        // register client
        m_clients.insert(client);

        // signal the client that he got the lock
        emit lockGranted(client, resource);
    } else {
        // resource is locked already by another client ->

        // add current client to the wait queue
        m_lockTable[resource].queue.append(client);

        // register client
        m_clients.insert(client);
    }
}

void KLockingServer::unlock(const QString &resource)
{
    // check if the resource is locked at all
    if (!m_lockTable.contains(resource)) {
        // hmm, should not happen, but we can ignore that
    } else {
        const QString client = message().service();

        LockTableEntry &entry = m_lockTable[resource];

        // sanity check
        if (entry.owner != client) {
            // somebody else than the owner of the lock tried to
            // release it -> ignore it
            return;
        }

        if (entry.queue.isEmpty()) {
            // there is no other client waiting for the lock
            //  -> remove the entry from the lock table
            m_lockTable.remove(resource);

            // unregister client
            m_clients.remove(client);
        } else {
            // remove first client from the waiting queue and make it
            // the new owner of the lock
            entry.owner = entry.queue.takeFirst();

            // activate the new owner by sending the reply
            emit lockGranted(entry.owner, resource);
        }
    }
}

void KLockingServer::clientUnregistered(const QString &client, const QString &oldOwner, const QString &newOwner)
{
    // first check if the client is one of ours
    if (!m_clients.contains(client) || oldOwner != client || !newOwner.isEmpty() )
        return; // ignore others

    // now we have to iterate over all lock table entries and remove the
    // stale one
    QMutableMapIterator<QString, LockTableEntry> it(m_lockTable);
    while (it.hasNext()) {
        it.next();
        LockTableEntry &entry = it.value();

        // remove client from waiting queue
        entry.queue.removeAll(client);

        // check if client is owner of the lock
        if (entry.owner == client) {
            if (entry.queue.isEmpty()) {
                // if the waiting queue is empty, just remove the lock table entry
                it.remove();
            } else {
                // remove first client from the waiting queue and make it
                // the new owner of the lock
                entry.owner = entry.queue.takeFirst();

                // activate the new owner by sending the reply
                emit lockGranted(entry.owner, it.key());
            }
        }
    }

    // forget about the client
    m_clients.remove(client);
}

#include "klockingserver.moc"
