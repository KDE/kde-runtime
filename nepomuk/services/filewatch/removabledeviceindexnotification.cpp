/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "removabledeviceindexnotification.h"
#include "strigiserviceinterface.h"

#include <KLocale>
#include <KConfig>
#include <KConfigGroup>
#include <KIcon>
#include <KDebug>

#include <Solid/StorageAccess>

RemovableDeviceIndexNotification::RemovableDeviceIndexNotification(const Solid::Device& device,
                                                                   QObject *parent)
    : KNotification(QLatin1String("nepomuk_new_removable_device"),
          KNotification::Persistent,
          parent),
      m_device(device)
{
    setTitle(i18nc("@title", "New removable device detected"));
    setText(i18nc("@info", "Do you want files on removable device <resource>%1</resource> to be indexed for fast desktop searches?", device.description()));
    setPixmap(KIcon(QLatin1String("nepomuk")).pixmap(32, 32));

    setActions(QStringList()
               << i18nc("@action", "Index files on this device")
               << i18nc("@action", "Do not index files on this device")
               << i18nc("@action", "Always index files on removable devices")
               << i18nc("@action", "Never index files on removable media")
               << i18nc("@action", "Ask me later"));
    connect(this, SIGNAL(activated(uint)), this, SLOT(slotActionActivated(uint)));

    // as soon as the device is unmounted this notification becomes pointless
    if ( const Solid::StorageAccess* storage = m_device.as<Solid::StorageAccess>() ) {
        connect(storage, SIGNAL(accessibilityChanged(bool,QString)), SLOT(close()));
    }
}

void RemovableDeviceIndexNotification::slotActionDoIndexActivated()
{
    KConfig strigiConfig( "nepomukstrigirc" );
    strigiConfig.group("Devices").writeEntry(m_device.udi(), true);

    org::kde::nepomuk::Strigi strigi( "org.kde.nepomuk.services.nepomukstrigiservice", "/nepomukstrigiservice", QDBusConnection::sessionBus() );
    const Solid::StorageAccess* storage = m_device.as<Solid::StorageAccess>();
    if ( strigi.isValid() && storage ) {
        strigi.indexFolder( storage->filePath(), true /* recursive */, false /* no forced update */ );
    }

    close();
}

void RemovableDeviceIndexNotification::slotActionDoNotIndexActivated()
{
    KConfig strigiConfig( "nepomukstrigirc" );
    strigiConfig.group("Devices").writeEntry(m_device.udi(), false);

    close();
}

void RemovableDeviceIndexNotification::slotActionDoIndexAllActivated()
{
    KConfig strigiConfig( "nepomukstrigirc" );
    strigiConfig.group( "General" ).writeEntry( "index newly mounted", true );

    slotActionDoIndexActivated();
}

void RemovableDeviceIndexNotification::slotActionDoNotIndexAnyActivated()
{
    KConfig strigiConfig( "nepomukstrigirc" );
    strigiConfig.group( "General" ).writeEntry( "index newly mounted", false );

    close();
}

void RemovableDeviceIndexNotification::slotActionIgnoreActivated()
{
    close();
}

void RemovableDeviceIndexNotification::slotActionActivated(uint action)
{
    kDebug() << action;
    switch(action) {
    case 0:
        slotActionDoIndexActivated();
        break;
    case 1:
        slotActionDoNotIndexActivated();
        break;
    case 2:
        slotActionDoIndexAllActivated();
        break;
    case 3:
        slotActionDoNotIndexAnyActivated();
        break;
    case 4:
        slotActionIgnoreActivated();
        break;
    }
}

#include "removabledeviceindexnotification.moc"
