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
            mountDevice(volume.udi());
        }
    }
}

DeviceAutomounter::~DeviceAutomounter()
{
}

void
DeviceAutomounter::mountDevice(const QString &udi)
{
    Solid::Device dev(udi);
    if (dev.is<Solid::StorageVolume>()) {
        Solid::StorageVolume *sv = dev.as<Solid::StorageVolume>();
        if (!sv->isIgnored() && dev.is<Solid::StorageAccess>()) {
            Solid::StorageAccess *sa = dev.as<Solid::StorageAccess>();
            kDebug() << "Mounting" << udi;
            sa->setup();
        }
    }
}

void
DeviceAutomounter::deviceAdded(const QString &udi)
{
    AutomounterSettings::self()->readConfig();
    if (AutomounterSettings::automountOnPlugin()) {
        mountDevice(udi);
    }
}
