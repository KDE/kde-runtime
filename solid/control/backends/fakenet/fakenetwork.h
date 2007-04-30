/*  This file is part of the KDE project
    Copyright (C) 2006 Will Stephenson <wstephenson@kde.org>

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

#ifndef FAKE_NETWORK_H
#define FAKE_NETWORK_H

#include <QStringList>
#include <QVariant>

#include <kdemacros.h>

#include <solid/control/ifaces/network.h>

using namespace Solid::Control::Ifaces;

class KDE_EXPORT FakeNetwork : public QObject, virtual public Solid::Control::Ifaces::Network
{
    Q_OBJECT
    Q_INTERFACES(Solid::Control::Ifaces::Network)
public:
    FakeNetwork(const QString  & uni, const QMap<QString, QVariant>  & propertyMap,
                 QObject * parent = 0);
    virtual ~FakeNetwork();

    QList<QNetworkAddressEntry> addressEntries() const;

    QString route() const;

    QList<QHostAddress> dnsServers() const;

    void setActivated(bool);
    bool isActive() const;

    QString uni() const;
Q_SIGNALS:
    void ipDetailsChanged();
    void activationStateChanged(bool);

protected:
    QList<QHostAddress> stringlistToKIpAddress( const QStringList  & addrStringList) const;
    QList<QNetworkAddressEntry> stringlistsToQNetworkAddressEntries(const QStringList &,const QStringList &,const QStringList  &) const;
    QMap<QString, QVariant> mPropertyMap;
};

#endif

