/***************************************************************************
*   Copyright (C) 2009 by Trever Fischer <wm161@wm161.net>                *
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

#include <KPluginFactory>

#include <Solid/Device>
#include <Solid/DeviceNotifier>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>

#include <QTimer>
#include <KGlobal>
#include <KLocale>

#include <KDebug>

K_PLUGIN_FACTORY(DeviceAutomounterFactory, registerPlugin<DeviceAutomounter>();)
K_EXPORT_PLUGIN(DeviceAutomounterFactory("kded_device_automounter"))

DeviceAutomounter::DeviceAutomounter(QObject *parent, const QVariantList &args)
    : KDEDModule(parent)
{
    Q_UNUSED(args);
    QTimer::singleShot(0, this, SLOT(init()));
}

DeviceAutomounter::~DeviceAutomounter()
{
}

void
DeviceAutomounter::init()
{
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString&)), this, SLOT(deviceAdded(const QString&)));
    QList<Solid::Device> volumes = Solid::Device::listFromType(Solid::DeviceInterface::StorageVolume);
    foreach(Solid::Device volume, volumes) {
        // sa can be 0 (e.g. for the swap partition)
        if (Solid::StorageAccess *sa = volume.as<Solid::StorageAccess>())
            connect(sa, SIGNAL(accessibilityChanged(bool, const QString)),
                    this, SLOT(deviceMountChanged(bool, const QString)));
        automountDevice(volume, AutomounterSettings::Login);
    }
    AutomounterSettings::self()->writeConfig();
}

void
DeviceAutomounter::deviceMountChanged(bool accessible, const QString &udi)
{
    AutomounterSettings::setDeviceLastSeenMounted(udi, accessible);
    AutomounterSettings::self()->writeConfig();
}

void
DeviceAutomounter::automountDevice(Solid::Device &dev, AutomounterSettings::AutomountType type)
{
    if (dev.is<Solid::StorageVolume>() && dev.is<Solid::StorageAccess>()) {
        Solid::StorageAccess *sa = dev.as<Solid::StorageAccess>();
        AutomounterSettings::setDeviceLastSeenMounted(dev.udi(), sa->isAccessible());
        AutomounterSettings::saveDevice(dev);
        kDebug() << "Saving as" << dev.description();
        if (AutomounterSettings::shouldAutomountDevice(dev.udi(), type)) {
            Solid::StorageVolume *sv = dev.as<Solid::StorageVolume>();
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
    Solid::Device dev(udi);
    automountDevice(dev, AutomounterSettings::Attach);
    AutomounterSettings::self()->writeConfig();
    if (dev.is<Solid::StorageAccess>()) {
        Solid::StorageAccess *sa = dev.as<Solid::StorageAccess>();
        connect(sa, SIGNAL(accessibilityChanged(bool, const QString)), this, SLOT(deviceMountChanged(bool, const QString)));
    }
}
