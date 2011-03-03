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
#include "hardwaredatabase.h"

#include <kconfiggroup.h>
#include <kdebug.h>
#include <klocale.h>

namespace PS
{

bool AudioDeviceKey::operator==(const AudioDeviceKey& rhs) const
{
    if (uniqueId.isNull() || rhs.uniqueId.isNull()) {
        return cardNumber == rhs.cardNumber && deviceNumber == rhs.deviceNumber;
    }

    return
        uniqueId == rhs.uniqueId &&
        cardNumber == rhs.cardNumber &&
        deviceNumber == rhs.deviceNumber;
}

AudioDevice::AudioDevice()
    : m_index(0), m_initialPreference(0), m_isAvailable(false), m_isAdvanced(true),
    m_dbNameOverrideFound(false)
{
}

AudioDevice::AudioDevice(const QString &cardName, const QString &icon,
        const AudioDeviceKey &key, int pref, bool adv) :
    m_cardName(cardName),
    m_icon(icon),
    m_key(key),
    m_index(0),
    m_initialPreference(pref),
    m_isAvailable(false),
    m_isAdvanced(adv),
    m_dbNameOverrideFound(false)
{
    applyHardwareDatabaseOverrides();
}

bool AudioDevice::operator<(const AudioDevice& rhs) const
{
    return m_initialPreference > rhs.m_initialPreference;
}

bool AudioDevice::operator==(const AudioDevice& rhs) const
{
    return m_key == rhs.m_key;
}

const QString& AudioDevice::name() const
{
    return m_cardName;
}

void AudioDevice::setPreferredName(const QString& name)
{
    if (!m_dbNameOverrideFound) {
        m_cardName = name;
    }
}

int AudioDevice::index() const
{
    return m_index;
}

const QString AudioDevice::description() const
{
    if (!m_isAvailable) {
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

const QString& AudioDevice::icon() const
{
    return m_icon;
}

bool AudioDevice::isAvailable() const
{
    return m_isAvailable;
}

bool AudioDevice::isAdvanced() const
{
    return m_isAdvanced;
}

int AudioDevice::initialPreference() const
{
    return m_initialPreference;
}

int AudioDevice::deviceNumber() const
{
    return m_key.deviceNumber;
}

const QList< AudioDeviceAccess >& AudioDevice::accessList() const
{
    return m_accessList;
}

const AudioDeviceKey& AudioDevice::key() const
{
    return m_key;
}

void AudioDevice::addAccess(const AudioDeviceAccess &access)
{
    m_isAvailable |= !access.deviceIds().isEmpty();

    m_accessList << access;
    qSort(m_accessList); // FIXME: do sorted insert
}

void AudioDevice::applyHardwareDatabaseOverrides()
{
    // now let's take a look at the hardware database whether we have to override something
    kDebug(601) << "looking for" << m_key.uniqueId;
    if (HardwareDatabase::contains(m_key.uniqueId)) {
        const HardwareDatabase::Entry &e = HardwareDatabase::entryFor(m_key.uniqueId);
        kDebug(601) << "  found it:" << e.name << e.iconName << e.initialPreference << e.isAdvanced;

        if (!e.name.isEmpty()) {
            m_dbNameOverrideFound = true;
            m_cardName = e.name;
        }

        if (!e.iconName.isEmpty()) {
            m_icon = e.iconName;
        }

        if (e.isAdvanced != 2) {
            m_isAdvanced = e.isAdvanced;
        }

        m_initialPreference = e.initialPreference;
    }
}

void AudioDevice::removeFromCache(const KSharedConfigPtr &config) const
{
    KConfigGroup cGroup(config, QLatin1String("AudioDevice_") + m_key.uniqueId);
    cGroup.writeEntry("deleted", true);
}

void AudioDevice::syncWithCache(const KSharedConfigPtr &config)
{
    KConfigGroup cGroup(config, QLatin1String("AudioDevice_") + m_key.uniqueId);
    if (cGroup.exists()) {
        m_index = cGroup.readEntry("index", 0);
    }

    if (m_index >= 0) {
        KConfigGroup globalGroup(config, "Globals");
        m_index = -globalGroup.readEntry("nextIndex", 1);
        globalGroup.writeEntry("nextIndex", 1 - m_index);
        Q_ASSERT(m_index < 0);

        cGroup.writeEntry("index", m_index);
    }

    cGroup.writeEntry("cardName", m_cardName);
    cGroup.writeEntry("iconName", m_icon);
    cGroup.writeEntry("initialPreference", m_initialPreference);
    cGroup.writeEntry("isAdvanced", m_isAdvanced);
    cGroup.writeEntry("deviceNumber", m_key.deviceNumber);
    cGroup.writeEntry("deleted", false);
    // hack: only internal soundcards should get the icon audio-card. All others, we assume, are
    // hotpluggable
    cGroup.writeEntry("hotpluggable", m_icon != QLatin1String("audio-card"));
}

} // namespace PS
