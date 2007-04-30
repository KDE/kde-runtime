/*  This file is part of the KDE project
    Copyright (C) 2006 Will Stephenson <wstephenson@kde.org>
    Copyright (C) 2007 Daniel Gollub <dgollub@suse.de>


    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include <QMap>
#include <QPair>
#include <QStringList>

#include <kdebug.h>

#include <solid/control/ifaces/bluetoothinterface.h>

#include "frontendobject_p.h"

#include "../soliddefs_p.h"
#include "bluetoothmanager.h"
#include "bluetoothinterface.h"
#include "bluetoothremotedevice.h"

namespace Solid
{
namespace Control
{
class BluetoothInterfacePrivate : public FrontendObjectPrivate
{
public:
    BluetoothInterfacePrivate(QObject *parent)
        : FrontendObjectPrivate(parent) { }

    void setBackendObject(QObject *object);

    QPair<BluetoothRemoteDevice *, Ifaces::BluetoothRemoteDevice *> findRegisteredBluetoothRemoteDevice(const QString &ubi) const;

    mutable QMap<QString, QPair<BluetoothRemoteDevice *, Ifaces::BluetoothRemoteDevice *> > remoteDeviceMap;
    mutable BluetoothRemoteDevice invalidDevice;
};
}
}

Solid::Control::BluetoothInterface::BluetoothInterface()
        : QObject(), d(new BluetoothInterfacePrivate(this))
{}

Solid::Control::BluetoothInterface::BluetoothInterface(const QString &ubi)
        : QObject(), d(new BluetoothInterfacePrivate(this))
{
    const BluetoothInterface &device = BluetoothManager::self().findBluetoothInterface(ubi);
    d->setBackendObject(device.d->backendObject());
}

Solid::Control::BluetoothInterface::BluetoothInterface(QObject *backendObject)
        : QObject(), d(new BluetoothInterfacePrivate(this))
{
    d->setBackendObject(backendObject);
}

Solid::Control::BluetoothInterface::BluetoothInterface(const BluetoothInterface &device)
        : QObject(), d(new BluetoothInterfacePrivate(this))
{
    d->setBackendObject(device.d->backendObject());
}

Solid::Control::BluetoothInterface::~BluetoothInterface()
{
    // Delete all the interfaces, they are now outdated
    typedef QPair<BluetoothRemoteDevice *, Ifaces::BluetoothRemoteDevice *> BluetoothRemoteDeviceIfacePair;

    // Delete all the devices, they are now outdated
    foreach (const BluetoothRemoteDeviceIfacePair &pair, d->remoteDeviceMap.values()) {
        delete pair.first;
        delete pair.second;
    }
}

Solid::Control::BluetoothInterface &Solid::Control::BluetoothInterface::operator=(const Solid::Control::BluetoothInterface  & dev)
{
    d->setBackendObject(dev.d->backendObject());

    return *this;
}

QString Solid::Control::BluetoothInterface::ubi() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), QString(), ubi());
}

const Solid::Control::BluetoothRemoteDevice &Solid::Control::BluetoothInterface::findBluetoothRemoteDevice(const QString &ubi) const
{
    QPair<BluetoothRemoteDevice *, Ifaces::BluetoothRemoteDevice *> pair = d->findRegisteredBluetoothRemoteDevice(ubi);

    if (pair.first != 0) {
        return *pair.first;
    } else {
        return d->invalidDevice;
    }
}

QString Solid::Control::BluetoothInterface::address() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), QString(), address());
}

QString Solid::Control::BluetoothInterface::version() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), QString(), version());
}

QString Solid::Control::BluetoothInterface::revision() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), QString(), revision());
}

QString Solid::Control::BluetoothInterface::manufacturer() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), QString(), manufacturer());
}

QString Solid::Control::BluetoothInterface::company() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), QString(), company());
}

Solid::Control::BluetoothInterface::Mode Solid::Control::BluetoothInterface::mode() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), Solid::Control::BluetoothInterface::Off, mode());
}

int Solid::Control::BluetoothInterface::discoverableTimeout() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), 0, discoverableTimeout());
}

bool Solid::Control::BluetoothInterface::isDiscoverable() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), false, isDiscoverable());
}

Solid::Control::BluetoothRemoteDeviceList Solid::Control::BluetoothInterface::listConnections() const
{
    BluetoothRemoteDeviceList list;
    Ifaces::BluetoothInterface *backend = qobject_cast<Ifaces::BluetoothInterface *>(d->backendObject());

    if (backend == 0) return list;

    QStringList ubis = backend->listConnections();

    foreach (const QString &ubi, ubis) {
        BluetoothRemoteDevice remoteDevice = findBluetoothRemoteDevice(ubi);
        list.append(remoteDevice);
    }

    return list;
}

QString Solid::Control::BluetoothInterface::majorClass() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), QString(), majorClass());
}

QStringList Solid::Control::BluetoothInterface::listAvailableMinorClasses() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), QStringList(), listAvailableMinorClasses());
}

QString Solid::Control::BluetoothInterface::minorClass() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), QString(), minorClass());
}

QStringList Solid::Control::BluetoothInterface::serviceClasses() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), QStringList(), serviceClasses());
}

QString Solid::Control::BluetoothInterface::name() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), QString(), name());
}

QStringList Solid::Control::BluetoothInterface::listBondings() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), QStringList(), listBondings());
}

bool Solid::Control::BluetoothInterface::isPeriodicDiscoveryActive() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), false, isPeriodicDiscoveryActive());
}

bool Solid::Control::BluetoothInterface::isPeriodicDiscoveryNameResolvingActive() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), false,
                      isPeriodicDiscoveryNameResolvingActive());
}

// TODO: QStringList or BluetoothRemoteDeviceList?
QStringList Solid::Control::BluetoothInterface::listRemoteDevices() const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), QStringList(),
                      listRemoteDevices());
}

// TODO: QStringList or BluetoothRemoteDeviceList?
QStringList Solid::Control::BluetoothInterface::listRecentRemoteDevices(const QDateTime &date) const
{
    return_SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), QStringList(),
                      listRecentRemoteDevices(date));
}

/***************************************************************/

void Solid::Control::BluetoothInterface::setMode(const Solid::Control::BluetoothInterface::Mode mode)
{
    SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), setMode(mode));
}

void Solid::Control::BluetoothInterface::setDiscoverableTimeout(int timeout)
{
    SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), setDiscoverableTimeout(timeout));
}

void Solid::Control::BluetoothInterface::setMinorClass(const QString &minor)
{
    SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), setMinorClass(minor));
}

void Solid::Control::BluetoothInterface::setName(const QString &name)
{
    SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), setName(name));
}

void Solid::Control::BluetoothInterface::discoverDevices()
{
    kDebug() << k_funcinfo << endl;
    SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), discoverDevices());
}

void Solid::Control::BluetoothInterface::discoverDevicesWithoutNameResolving()
{
    SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), discoverDevicesWithoutNameResolving());
}

void Solid::Control::BluetoothInterface::cancelDiscovery()
{
    SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), cancelDiscovery());
}

void Solid::Control::BluetoothInterface::startPeriodicDiscovery()
{
    SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), startPeriodicDiscovery());
}

void Solid::Control::BluetoothInterface::stopPeriodicDiscovery()
{
    SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), stopPeriodicDiscovery());
}

void Solid::Control::BluetoothInterface::setPeriodicDiscoveryNameResolving(bool resolveNames)
{
    SOLID_CALL(Ifaces::BluetoothInterface *, d->backendObject(), setPeriodicDiscoveryNameResolving(resolveNames));
}

void Solid::Control::BluetoothInterfacePrivate::setBackendObject(QObject *object)
{
    FrontendObjectPrivate::setBackendObject(object);

    if (object) {
        QObject::connect(object, SIGNAL(modeChanged(const QString &)),
                         parent(), SIGNAL(modeChanged(const QString &)));
        QObject::connect(object, SIGNAL(discoverableTimeoutChanged(int)),
                         parent(), SIGNAL(discoverableTimeoutChanged(int)));
        QObject::connect(object, SIGNAL(minorClassChanged(const QString &)),
                         parent(), SIGNAL(minorClassChanged(const QString &)));
        QObject::connect(object, SIGNAL(nameChanged(const QString &)),
                         parent(), SIGNAL(nameChanged(const QString &)));
        QObject::connect(object, SIGNAL(discoveryStarted()),
                         parent(), SIGNAL(discoveryStarted()));
        QObject::connect(object, SIGNAL(discoveryCompleted()),
                         parent(), SIGNAL(discoveryCompleted()));
        QObject::connect(object, SIGNAL(remoteDeviceFound(const QString &, int, int)),
                         parent(), SIGNAL(remoteDeviceFound(const QString &, int, int)));
        QObject::connect(object, SIGNAL(remoteDeviceDisappeared(const QString &)),
                         parent(), SIGNAL(remoteDeviceDisappeared(const QString &)));
    }
}

QPair<Solid::Control::BluetoothRemoteDevice *, Solid::Control::Ifaces::BluetoothRemoteDevice *> Solid::Control::BluetoothInterfacePrivate::findRegisteredBluetoothRemoteDevice(const QString &ubi) const
{
    if (remoteDeviceMap.contains(ubi)) {
        return remoteDeviceMap[ubi];
    } else {
        Ifaces::BluetoothInterface *backend = qobject_cast<Ifaces::BluetoothInterface *>(backendObject());
        Ifaces::BluetoothRemoteDevice *iface = 0;

        if (backend != 0) {
            iface = qobject_cast<Ifaces::BluetoothRemoteDevice *>(backend->createBluetoothRemoteDevice(ubi));
        }

        if (iface != 0) {
            BluetoothRemoteDevice *device = new BluetoothRemoteDevice(iface);

            QPair<BluetoothRemoteDevice *, Ifaces::BluetoothRemoteDevice *> pair(device, iface);
            remoteDeviceMap[ubi] = pair;

            return pair;
        } else {
            return QPair<BluetoothRemoteDevice *, Ifaces::BluetoothRemoteDevice *>(0, 0);
        }

    }
}

#include "bluetoothinterface.moc"
