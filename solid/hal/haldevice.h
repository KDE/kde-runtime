/*  This file is part of the KDE project
    Copyright (C) 2005,2006 Kevin Ottens <ervin@kde.org>

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

#ifndef HALDEVICE_H
#define HALDEVICE_H

#include <solid/ifaces/device.h>

class HalManager;
class HalDevicePrivate;

struct ChangeDescription
{
    QString key;
    bool added;
    bool removed;
};

class HalDevice : public Solid::Ifaces::Device
{
    Q_OBJECT

public:
    HalDevice(const QString &udi);
    virtual ~HalDevice();

    virtual QString udi() const;
    virtual QString parentUdi() const;

    virtual QString vendor() const;
    virtual QString product() const;

    virtual bool setProperty( const QString &key, const QVariant &value );
    virtual QVariant property( const QString &key ) const;

    virtual QMap<QString, QVariant> allProperties() const;

    virtual bool removeProperty( const QString &key );
    virtual bool propertyExists( const QString &key ) const;

    virtual bool addCapability( const Solid::Capability::Type &capability );
    virtual bool queryCapability( const Solid::Capability::Type &capability ) const;
    virtual QObject *createCapability( const Solid::Capability::Type &capability );

    virtual bool lock(const QString &reason);
    virtual bool unlock();
    virtual bool isLocked() const;
    virtual QString lockReason() const;

private Q_SLOTS:
    void slotPropertyModified( int count, const QList<ChangeDescription> &changes );
    void slotCondition( const QString &condition, const QString &reason );

private:
    HalDevicePrivate *d;
};

#endif
