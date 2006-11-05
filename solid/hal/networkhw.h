/*  This file is part of the KDE project
    Copyright (C) 2006 Kevin Ottens <ervin@kde.org>

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

#ifndef NETWORKHW_H
#define NETWORKHW_H

#include <solid/ifaces/networkhw.h>
#include "capability.h"

class HalDevice;

class NetworkHw : public Capability, virtual public Solid::Ifaces::NetworkHw
{
    Q_OBJECT
    Q_INTERFACES( Solid::Ifaces::NetworkHw )

public:
    NetworkHw( HalDevice *device );
    virtual ~NetworkHw();

    virtual QString ifaceName() const;
    virtual bool isWireless() const;
    virtual QString hwAddress() const;
    virtual qulonglong macAddress() const;
};

#endif
