/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005-2007 Kevin Ottens <ervin@kde.org>

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

#ifndef DEVICEACTION_H
#define DEVICEACTION_H

#include <solid/device.h>

class DeviceAction
{
public:
    DeviceAction();
    virtual ~DeviceAction();

    virtual QString label() const;
    virtual QString iconName() const;
    virtual void setLabel(const QString &label);
    virtual void setIconName(const QString &icon);

    virtual QString id() const = 0;
    virtual void execute(Solid::Device &device) = 0;

private:
    QString m_label;
    QString m_iconName;
};

#endif
