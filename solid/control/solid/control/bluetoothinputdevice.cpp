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
#include <QStringList>

#include <solid/control/ifaces/bluetoothinputdevice.h>

#include "frontendobject_p.h"

#include "../soliddefs_p.h"

#include "bluetoothinputdevice.h"

namespace Solid
{
namespace Control
{
    class BluetoothInputDevicePrivate : public FrontendObjectPrivate
    {
    public:
        BluetoothInputDevicePrivate(QObject *parent)
            : FrontendObjectPrivate(parent) { }

        void setBackendObject(QObject *object);
    };
}
}

Solid::Control::BluetoothInputDevice::BluetoothInputDevice(QObject *backendObject)
    : QObject(), d(new BluetoothInputDevicePrivate(this))
{
    d->setBackendObject(backendObject);
}

Solid::Control::BluetoothInputDevice::BluetoothInputDevice(const BluetoothInputDevice &device)
    : QObject(), d(new BluetoothInputDevicePrivate(this))
{
    d->setBackendObject(device.d->backendObject());
}

Solid::Control::BluetoothInputDevice::~BluetoothInputDevice()
{}

Solid::Control::BluetoothInputDevice &Solid::Control::BluetoothInputDevice::operator=(const Solid::Control::BluetoothInputDevice  & dev)
{
    d->setBackendObject(dev.d->backendObject());

    return *this;
}

QString Solid::Control::BluetoothInputDevice::ubi() const
{
    return_SOLID_CALL(Ifaces::BluetoothInputDevice *, d->backendObject(), QString(), ubi());
}

bool Solid::Control::BluetoothInputDevice::isConnected() const
{
    return_SOLID_CALL(Ifaces::BluetoothInputDevice *, d->backendObject(), false, isConnected());
}

QString Solid::Control::BluetoothInputDevice::name() const
{
    return_SOLID_CALL(Ifaces::BluetoothInputDevice *, d->backendObject(), QString(), name());
}

QString Solid::Control::BluetoothInputDevice::address() const
{
    return_SOLID_CALL(Ifaces::BluetoothInputDevice *, d->backendObject(), QString(), address());
}

QString Solid::Control::BluetoothInputDevice::productID() const
{
    return_SOLID_CALL(Ifaces::BluetoothInputDevice *, d->backendObject(), QString(), productID());
}

QString Solid::Control::BluetoothInputDevice::vendorID() const
{
    return_SOLID_CALL(Ifaces::BluetoothInputDevice *, d->backendObject(), QString(), vendorID());
}

void Solid::Control::BluetoothInputDevice::slotConnect()
{
    SOLID_CALL(Ifaces::BluetoothInputDevice *, d->backendObject(), slotConnect());
}

void Solid::Control::BluetoothInputDevice::slotDisconnect()
{
    SOLID_CALL(Ifaces::BluetoothInputDevice *, d->backendObject(), slotDisconnect());
}

void Solid::Control::BluetoothInputDevicePrivate::setBackendObject(QObject *object)
{
    FrontendObjectPrivate::setBackendObject(object);

    if (object) {
        QObject::connect(object, SIGNAL(connected()),
                         parent(), SIGNAL(connected()));
        QObject::connect(object, SIGNAL(disconnected()),
                         parent(), SIGNAL(disconnected()));
    }
}

#include "bluetoothinputdevice.moc"
