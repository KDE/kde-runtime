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

#include "DeviceAutomounter.h"

#include "AutomounterSettings.h"

#include <KGenericFactory>

#include <Solid/Device>
#include <Solid/DeviceNotifier>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>

#include <KDebug>

K_PLUGIN_FACTORY(DeviceAutomounterFactory, registerPlugin<DeviceAutomounter>();)
K_EXPORT_PLUGIN(DeviceAutomounterFactory("kded_device_automounter"));

DeviceAutomounter::DeviceAutomounter(QObject *parent, const QVariantList &args)
    : KDEDModule(parent)
{
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString&)), this, SLOT(deviceAdded(const QString&)));
    if (AutomounterSettings::automountOnLogin()) {
        QList<Solid::Device> volumes = Solid::Device::listFromType(Solid::DeviceInterface::StorageVolume);
        foreach(const Solid::Device &volume, volumes) {
            const Solid::StorageAccess *sa = volume.as<Solid::StorageAccess>();
            connect(sa, SIGNAL(accessibilityChanged(bool, const QString)), this, SLOT(deviceMountChanged(bool, const QString)));
            automountDevice(volume);
        }
        AutomounterSettings::self()->writeConfig();
    }
}

DeviceAutomounter::~DeviceAutomounter()
{
}

void
DeviceAutomounter::deviceMountChanged(bool accessible, const QString &udi)
{
    AutomounterSettings::setDeviceLastSeenMounted(udi, accessible);
    AutomounterSettings::self()->writeConfig();
}

void
DeviceAutomounter::automountDevice(const Solid::Device &dev)
{
    if (dev.is<Solid::StorageVolume>() && dev.is<Solid::StorageAccess>()) {
        Solid::Device volumeDevice(dev.udi());
        Solid::StorageAccess *sa = volumeDevice.as<Solid::StorageAccess>();
        AutomounterSettings::setDeviceLastSeenMounted(dev.udi(), sa->isAccessible());
        if (AutomounterSettings::shouldAutomountDevice(dev)) {
            Solid::StorageVolume *sv = volumeDevice.as<Solid::StorageVolume>();
            if (!sv->isIgnored()) {
                kDebug() << "Mounting" << dev.udi();
                sa->setup();
            }
        }
    }
}

void
DeviceAutomounter::deviceAdded(const QString &udi)
{
    AutomounterSettings::self()->readConfig();
    if (AutomounterSettings::automountOnPlugin()) {
        Solid::Device dev(udi);
        automountDevice(dev);
        AutomounterSettings::self()->writeConfig();
    }
}
