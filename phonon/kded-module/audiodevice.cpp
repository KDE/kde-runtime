/*  This file is part of the KDE project
    Copyright (C) 2008 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) version 3.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "audiodevice.h"
#include <klocale.h>

PS::AudioDevice::AudioDevice()
    : m_initialPreference(0), m_available(false), m_isAdvanced(true)
{
}

PS::AudioDevice::AudioDevice(const QString &cardName, const QString &icon,
        const AudioDeviceKey &key, int pref, bool adv)
    : m_cardName(cardName),
    m_icon(icon),
    m_key(key),
    m_initialPreference(pref),
    m_available(false),
    m_isAdvanced(adv)
{
}

void PS::AudioDevice::addAccess(const AudioDeviceAccess &access)
{
    m_available |= !access.deviceIds().isEmpty();

    m_accessList << access;
    qSort(m_accessList); // FIXME: do sorted insert
}

QString PS::AudioDevice::description() const
{
    if (!m_available) {
        return i18n("<html>This device is currently not available (either it is unplugged or the "
                "driver is not loaded).</html>");
    }
    QString list;
    foreach (const AudioDeviceAccess &a, m_accessList) {
        foreach (const QString &id, a.deviceIds()) {
            list += i18nc("The first argument is name of the driver/sound subsystem. "
                    "The second argument is the device identifier", "<li>%1: %2</li>",
                    a.driverName(), id);
        }
    }
    return i18n("<html>This will try the following devices and use the first that works: "
            "<ol>%1</ol></html>", list);
}
