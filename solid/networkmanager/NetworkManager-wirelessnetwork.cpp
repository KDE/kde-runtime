/*  This file is part of the KDE project
    Copyright (C) 2007 Will Stephenson <wstephenson@kde.org>

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

// Copied from wireless.h
/* Modes of operation */
#define IW_MODE_AUTO    0   /* Let the driver decides */
#define IW_MODE_ADHOC   1   /* Single cell network */
#define IW_MODE_INFRA   2   /* Multi cell network, roaming, ... */
#define IW_MODE_MASTER  3   /* Synchronization master or Access Point */
#define IW_MODE_REPEAT  4   /* Wireless Repeater (forwarder) */
#define IW_MODE_SECOND  5   /* Secondary master/repeater (backup) */
#define IW_MODE_MONITOR 6   /* Passive monitor (listen only) */

#include <NetworkManager/NetworkManager.h>

#include <kdebug.h>
#include <solid/control/ifaces/authentication.h>

#include "NetworkManager-dbushelper.h"
#include "NetworkManager-wirelessnetwork.h"

void dump(const Solid::Control::WirelessNetwork::Capabilities  & cap)
{
    kDebug(1441) << "WEP      " << (cap  & Solid::Control::WirelessNetwork::Wep ? "X " : " O") << endl;
    kDebug(1441) << "WPA      " << (cap  & Solid::Control::WirelessNetwork::Wpa ? "X " : " O") << endl;
    kDebug(1441) << "WPA2     " << (cap  & Solid::Control::WirelessNetwork::Wpa2 ? "X " : " O") << endl;
    kDebug(1441) << "PSK      " << (cap  & Solid::Control::WirelessNetwork::Psk ? "X " : " O") << endl;
    kDebug(1441) << "Ieee8021x" << (cap  & Solid::Control::WirelessNetwork::Ieee8021x ? "X " : " O") << endl;
    kDebug(1441) << "Wep40    " << (cap  & Solid::Control::WirelessNetwork::Wep40 ? "X " : " O") << endl;
    kDebug(1441) << "Wep104   " << (cap  & Solid::Control::WirelessNetwork::Wep104 ? "X " : " O") << endl;
    kDebug(1441) << "Wep192   " << (cap  & Solid::Control::WirelessNetwork::Wep192 ? "X " : " O") << endl;
    kDebug(1441) << "Wep256   " << (cap  & Solid::Control::WirelessNetwork::Wep256 ? "X " : " O") << endl;
    kDebug(1441) << "WepOther " << (cap  & Solid::Control::WirelessNetwork::WepOther ? "X " : " O") << endl;
    kDebug(1441) << "TKIP     " << (cap  & Solid::Control::WirelessNetwork::Tkip ? "X " : " O") << endl;
    kDebug(1441) << "CCMP     " << (cap  & Solid::Control::WirelessNetwork::Ccmp ? "X " : " O") << endl;
}

void dump(const NMDBusWirelessNetworkProperties  & network)
{
    kDebug(1441) << "Object path: " << network.path.path() << "\nESSID: " << network.essid
        << "\nHardware address: " << network.hwAddr << "\nSignal strength: " << network.strength
        << "\nFrequency: " << network.frequency << "\nBit rate: " << network.rate
        << "\nMode: " << network.mode
        << "\nBroadcast: " << network.broadcast << "\nCapabilities: " << endl;
    dump(network.capabilities);
}

Solid::Control::WirelessNetwork::Capabilities getCapabilities(const int nm)
{
    Solid::Control::WirelessNetwork::Capabilities caps;
    if (nm  & NM_802_11_CAP_NONE)
        caps |= Solid::Control::WirelessNetwork::Unencrypted;
    if (nm  & NM_802_11_CAP_PROTO_WEP)
        caps |= Solid::Control::WirelessNetwork::Wep;
    if (nm  & NM_802_11_CAP_PROTO_WPA)
        caps |= Solid::Control::WirelessNetwork::Wpa;
    if (nm  & NM_802_11_CAP_PROTO_WPA2)
        caps |= Solid::Control::WirelessNetwork::Wpa2;
    if (nm  & NM_802_11_CAP_KEY_MGMT_PSK)
        caps |= Solid::Control::WirelessNetwork::Psk;
    if (nm  & NM_802_11_CAP_KEY_MGMT_802_1X)
        caps |= Solid::Control::WirelessNetwork::Ieee8021x;
    if (nm  & NM_802_11_CAP_CIPHER_WEP40)
        caps |= Solid::Control::WirelessNetwork::Wep40;
    if (nm  & NM_802_11_CAP_CIPHER_WEP104)
        caps |= Solid::Control::WirelessNetwork::Wep104;
    if (nm  & NM_802_11_CAP_CIPHER_TKIP)
        caps |= Solid::Control::WirelessNetwork::Tkip;
    if (nm  & NM_802_11_CAP_CIPHER_CCMP)
        caps |= Solid::Control::WirelessNetwork::Ccmp;
    return caps;
}

Solid::Control::WirelessNetwork::OperationMode getOperationMode(const int nm)
{
    Solid::Control::WirelessNetwork::OperationMode mode = Solid::Control::WirelessNetwork::Unassociated;
    switch (nm)
    {
    case IW_MODE_ADHOC:
        mode = Solid::Control::WirelessNetwork::Adhoc;
        break;
    case IW_MODE_INFRA:
    case IW_MODE_MASTER:
        mode = Solid::Control::WirelessNetwork::Managed;
        break;
    case IW_MODE_REPEAT:
        mode = Solid::Control::WirelessNetwork::Repeater;
        break;
    }
    return mode;
}

void deserialize(const QDBusMessage  & message, NMDBusWirelessNetworkProperties  & network)
{
    //Debug(1441) << "signature: " << message.signature() << endl;
    QList<QVariant> args = message.arguments();
    network.path.setPath(args.takeFirst().toString());
    network.essid = args.takeFirst().toString();
    network.hwAddr = args.takeFirst().toString();
    network.strength = args.takeFirst().toInt();
    network.frequency = args.takeFirst().toDouble();
    network.rate = args.takeFirst().toInt();
    network.mode = getOperationMode(args.takeFirst().toInt());
    network.capabilities = getCapabilities(args.takeFirst().toInt());
    network.broadcast = args.takeFirst().toBool();
}



typedef void Encryption;

class NMWirelessNetworkPrivate
{
public:
    NMWirelessNetworkPrivate(const QString  & netPath)
        : iface("org.freedesktop.NetworkManager",
                netPath,
                "org.freedesktop.NetworkManager.Devices",
                QDBusConnection::systemBus()),
        strength(0), frequency(0.0), rate(0), broadcast(true), authentication(0) { }
    QDBusInterface iface;
    QString essid;
    MacAddressList hwAddr; // MACs of the APs
    int strength;
    double frequency;
    int rate;
    Solid::Control::WirelessNetwork::OperationMode mode;
    Solid::Control::WirelessNetwork::Capabilities capabilities;
    bool broadcast;
    Solid::Control::Authentication * authentication;
};

NMWirelessNetwork::NMWirelessNetwork(const QString  & networkPath)
 : NMNetwork(networkPath), d(new NMWirelessNetworkPrivate(networkPath))
{
    //kDebug(1441) << "NMWirelessNetwork::NMWirelessNetwork() - " << networkPath << endl;
    QDBusMessage reply = d->iface.call("getProperties");
    NMDBusWirelessNetworkProperties wlan;
    deserialize(reply, wlan);
    //dump(wlan);
    setProperties(wlan);
}

NMWirelessNetwork::~NMWirelessNetwork()
{
    delete d;
}

void NMWirelessNetwork::setProperties(const NMDBusWirelessNetworkProperties  & props)
{
    d->essid = props.essid;
    d->hwAddr.append(props.hwAddr);
    d->strength = props.strength;
    d->frequency = props.frequency;
    d->rate = props.rate;
    d->mode = props.mode;
    d->capabilities = props.capabilities;
    d->broadcast = props.broadcast;
}

int NMWirelessNetwork::signalStrength() const
{
    return d->strength;
}

int NMWirelessNetwork::bitrate() const
{
    return d->rate;
}

double NMWirelessNetwork::frequency() const
{
    return d->frequency;
}

Solid::Control::WirelessNetwork::Capabilities NMWirelessNetwork::capabilities() const
{
    return d->capabilities;
}

QString NMWirelessNetwork::essid() const
{
    return d->essid;
}

Solid::Control::WirelessNetwork::OperationMode NMWirelessNetwork::mode() const
{
    return d->mode;
}

bool NMWirelessNetwork::isAssociated() const
{
#warning NMWirelessNetwork::isAssociated() is unimplemented
    kDebug(1441) << "Fixme: implement NMWirelessNetwork::isAssociated()" << endl;
    return true;
}

bool NMWirelessNetwork::isEncrypted() const
{
    return !(d->capabilities  & Solid::Control::WirelessNetwork::Unencrypted) ;
}

bool NMWirelessNetwork::isHidden() const
{
#warning NMWirelessNetwork::isHidden() is unimplemented
    kDebug(1441) << "Fixme: implement NMWirelessNetwork::isHidden()" << endl;
    return true;
}

MacAddressList NMWirelessNetwork::bssList() const
{
    return d->hwAddr;
}

Solid::Control::Authentication * NMWirelessNetwork::authentication() const
{
    return d->authentication;
}

void NMWirelessNetwork::setAuthentication(Solid::Control::Authentication * auth)
{
    d->authentication = auth;
}

void NMWirelessNetwork::setSignalStrength(int strength)
{
    d->strength = strength;
    emit signalStrengthChanged(strength);
}

void NMWirelessNetwork::setBitrate(int rate)
{
    d->rate = rate;
    emit bitrateChanged(rate);
}

void NMWirelessNetwork::setActivated(bool activated)
{
    QDBusInterface manager("org.freedesktop.NetworkManager",
            "/org/freedesktop/NetworkManager",
            "org.freedesktop.NetworkManager",
            QDBusConnection::systemBus());
    QString devicePath = uni().left(uni().indexOf("/Networks"));
    kDebug(1441) << k_funcinfo << devicePath << " - " << d->essid << endl;
    QDBusObjectPath op(devicePath);
#warning fixme hardcoded false fallback bool in setActiveDevice
    QList<QVariant> args;
    args << qVariantFromValue(op) << d->essid << false;
    bool error;
    args = NMDBusHelper::serialize(d->authentication, d->essid, args, &error);
    kDebug(1441) << " " << args << endl;
    if (error)
        kDebug(1411) << "Error whilst serializing authentication." << endl;
    else
        manager.callWithArgumentList(QDBus::Block, "setActiveDevice", args);
    if (manager.lastError().isValid())
        kDebug(1441) << "setActiveDevice returned error: " << manager.lastError().name() << ": " << manager.lastError().message() << endl;

    emit activationStateChanged(activated);
}

#include "NetworkManager-wirelessnetwork.moc"

