/***************************************************************************
*   Copyright (C) 2009 by Trever Fischer <wm161@wm161.net                 *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
***************************************************************************/

#include "AutomounterSettings.h"

#include <QtCore/QDateTime>

KConfigGroup
AutomounterSettings::deviceSettings(const QString &udi)
{
    return self()->config()->group("Devices").group(udi);
}

QStringList
AutomounterSettings::knownDevices()
{
    KConfigGroup deviceList = self()->config()->group("Devices");
    QStringList allDevices = deviceList.groupList();
    QStringList knownDevices;
    foreach(const QString &dev, allDevices) {
        if (deviceList.group(dev).readEntry("EverMounted", false))
            knownDevices << dev;
    }
    return knownDevices;
}

bool
AutomounterSettings::deviceIsKnown(const QString &udi)
{
    return self()->config()->group("Devices").group(udi).readEntry("EverMounted", false);
}

bool
AutomounterSettings::deviceIsKnown(const Solid::Device &dev)
{
    return deviceIsKnown(dev.udi());
}

bool
AutomounterSettings::deviceAutomountIsForced(const QString &udi)
{
    return deviceSettings(udi).readEntry("ForceAutomount", false);
}

bool
AutomounterSettings::shouldAutomountDevice(const QString &udi)
{
    /*
     * First, we check the device-specific AutomountEnabled-overriding Automount value.
     * If thats true, we then check AutomountEnabled. Assuming true, we check if we
     * should automount known devices and if it is a known device. If neither, we check if
     * we should bother restoring its 'mounted' propery.
     */
    bool known = deviceIsKnown(udi);
    bool enabled = automountEnabled();
    bool automountKnown = !automountUnknownDevices();
    bool deviceAutomount = deviceAutomountIsForced(udi);
    bool lastSeenMounted = deviceSettings(udi).readEntry("LastSeenMounted", false);
    bool shouldAutomount = deviceAutomount || (enabled && ((automountKnown && known) || lastSeenMounted));

    kDebug() << "Processing" << udi;
    kDebug() << "automountKnownDevices:" << automountKnown;
    kDebug() << "deviceIsKnown:" << known;
    kDebug() << "AutomountEnabled:" << enabled;
    kDebug() << "Automount:" << deviceAutomount;
    kDebug() << "LastSeenMounted:" << lastSeenMounted;
    kDebug() << "ShouldAutomount:" << shouldAutomount;
    
    return shouldAutomount;
}

bool
AutomounterSettings::shouldAutomountDevice(const Solid::Device &dev)
{
    return shouldAutomountDevice(dev.udi());
}

void
AutomounterSettings::setDeviceLastSeenMounted(const QString &udi, bool mounted)
{
    kDebug() << "Marking" << udi << "as lastSeenMounted:" << mounted;
    if (mounted)
        deviceSettings(udi).writeEntry("EverMounted", true);
    deviceSettings(udi).writeEntry("LastSeenMounted", mounted);
}
