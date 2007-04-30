/*  This file is part of the KDE project
    Copyright (C) 2006 Will Stephenson <wstephenson@kde.org>
    Copyright (C) 2007 Kevin Ottens <ervin@kde.org>

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

#include "wirelessnetwork.h"

#include "network_p.h"
#include "../soliddefs_p.h"
#include <solid/control/ifaces/wirelessnetwork.h>

namespace Solid
{
namespace Control
{
    class WirelessNetworkPrivate : public NetworkPrivate
    {
    public:
        WirelessNetworkPrivate(QObject *parent)
            : NetworkPrivate(parent) { }

        void setBackendObject(QObject *object);
    };
}
}

Solid::Control::WirelessNetwork::WirelessNetwork(QObject *backendObject)
    : Network(*new WirelessNetworkPrivate(this), backendObject)
{
    Q_D(WirelessNetwork);
    d->setBackendObject(backendObject);
}

Solid::Control::WirelessNetwork::WirelessNetwork(const WirelessNetwork  & network)
    : Network(*new WirelessNetworkPrivate(this), network)
{
    Q_D(WirelessNetwork);
    d->setBackendObject(network.d_ptr->backendObject());
}

void Solid::Control::WirelessNetworkPrivate::setBackendObject(QObject *object)
{
    FrontendObjectPrivate::setBackendObject(object);

    if (object) {
        QObject::connect(object, SIGNAL(signalStrengthChanged(int)),
                         parent(), SIGNAL(signalStrengthChanged(int)));
        QObject::connect(object, SIGNAL(bitrateChanged(int)),
                         parent(), SIGNAL(bitrateChanged(int)));
        QObject::connect(object, SIGNAL(associationChanged(bool)),
                         parent(), SIGNAL(associationChanged(bool)));
        QObject::connect(object, SIGNAL(authenticationNeeded()),
                         parent(), SIGNAL(authenticationNeeded()));
    }
}

Solid::Control::WirelessNetwork::~WirelessNetwork()
{

}

bool Solid::Control::WirelessNetwork::isSameAs(const WirelessNetwork  & other) const
{
    if (other.essid() == essid())
    {
        QStringListIterator it(bssList());
        const QStringList otherBSSList = other.bssList();
        while (it.hasNext())
        {
            QString bss = it.next();
            if (other.bssList().contains(bss))
            {
                return true;
            }
        }
    }
    return false;
}

int Solid::Control::WirelessNetwork::signalStrength() const
{
    Q_D(const WirelessNetwork);
    return_SOLID_CALL(Ifaces::WirelessNetwork *, d->backendObject(), 0, signalStrength());
}

int Solid::Control::WirelessNetwork::bitrate() const
{
    Q_D(const WirelessNetwork);
    return_SOLID_CALL(Ifaces::WirelessNetwork *, d->backendObject(), 0, bitrate());
}

double Solid::Control::WirelessNetwork::frequency() const
{
    Q_D(const WirelessNetwork);
    return_SOLID_CALL(Ifaces::WirelessNetwork *, d->backendObject(), 0, frequency());
}

Solid::Control::MacAddressList Solid::Control::WirelessNetwork::bssList() const
{
    Q_D(const WirelessNetwork);
    return_SOLID_CALL(Ifaces::WirelessNetwork *, d->backendObject(), MacAddressList(), bssList());
}

Solid::Control::Authentication *Solid::Control::WirelessNetwork::authentication() const
{
    Q_D(const WirelessNetwork);
    return_SOLID_CALL(Ifaces::WirelessNetwork *, d->backendObject(), 0, authentication());
}

Solid::Control::WirelessNetwork::Capabilities Solid::Control::WirelessNetwork::capabilities() const
{
    Q_D(const WirelessNetwork);
    return_SOLID_CALL(Ifaces::WirelessNetwork *, d->backendObject(), Capabilities(), capabilities());
}

QString Solid::Control::WirelessNetwork::essid() const
{
    Q_D(const WirelessNetwork);
    return_SOLID_CALL(Ifaces::WirelessNetwork *, d->backendObject(), QString(), essid());
}

Solid::Control::WirelessNetwork::OperationMode Solid::Control::WirelessNetwork::mode() const
{
    Q_D(const WirelessNetwork);
    return_SOLID_CALL(Ifaces::WirelessNetwork *, d->backendObject(), Unassociated, mode());
}

bool Solid::Control::WirelessNetwork::isAssociated() const
{
    Q_D(const WirelessNetwork);
    return_SOLID_CALL(Ifaces::WirelessNetwork *, d->backendObject(), false, isAssociated());
}

bool Solid::Control::WirelessNetwork::isEncrypted() const
{
    Q_D(const WirelessNetwork);
    return_SOLID_CALL(Ifaces::WirelessNetwork *, d->backendObject(), false, isEncrypted());
}

bool Solid::Control::WirelessNetwork::isHidden() const
{
    Q_D(const WirelessNetwork);
    return_SOLID_CALL(Ifaces::WirelessNetwork *, d->backendObject(), false, isHidden());
}

void Solid::Control::WirelessNetwork::setAuthentication(Authentication *authentication)
{
    Q_D(WirelessNetwork);
    SOLID_CALL(Ifaces::WirelessNetwork *, d->backendObject(), setAuthentication(authentication));
}

#include "wirelessnetwork.moc"
