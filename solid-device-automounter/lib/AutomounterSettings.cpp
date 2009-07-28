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
    return self()->config()->group("Devices").groupList();
}

bool
AutomounterSettings::deviceIsKnown(const QString &udi)
{
    return self()->config()->group("Devices").hasGroup(udi);
}

bool
AutomounterSettings::deviceIsKnown(const Solid::Device &dev)
{
    return deviceIsKnown(dev.udi());
}

bool
AutomounterSettings::shouldAutomountDevice(const QString &udi)
{
    return automountUnknownDevices() || (deviceIsKnown(udi) && deviceSettings(udi).readEntry("Automount",automount()));
}

bool
AutomounterSettings::shouldAutomountDevice(const Solid::Device &dev)
{
    return shouldAutomountDevice(dev.udi());
}

void
AutomounterSettings::markDeviceSeen(const QString &udi)
{
    deviceSettings(udi).writeEntry("LastSeen", QDateTime::currentDateTime());
    kDebug() << "Marking" << udi << "as last seen on" << deviceSettings(udi).readEntry("LastSeen");
}

void
AutomounterSettings::markDeviceSeen(const Solid::Device &dev)
{
    markDeviceSeen(dev.udi());
}
