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

#include "DeviceAutomounterKCM.h"

#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>

#include <KGenericFactory>
#include <KAboutData>
#include <KConfigGroup>
#include <KInputDialog>
#include <Solid/DeviceNotifier>
#include <Solid/StorageVolume>

#include "AutomounterSettings.h"

K_PLUGIN_FACTORY(DeviceAutomounterKCMFactory, registerPlugin<DeviceAutomounterKCM>();)
K_EXPORT_PLUGIN(DeviceAutomounterKCMFactory("kcm_device_automounter"))

DeviceAutomounterKCM::DeviceAutomounterKCM(QWidget *parent, const QVariantList &)
    : KCModule(DeviceAutomounterKCMFactory::componentData(), parent)
{
    KAboutData *about = new KAboutData("kcm_device_automounter",
                                       0,
                                       ki18n("Device Automounter"),
                                       "0.1",
                                       ki18n("Automatically mounts devices at login or when attached"),
                                       KAboutData::License_GPL_V2,
                                       ki18n("(c) 2009 Trever Fischer"));
    about->addAuthor(ki18n("Trever Fischer"));

    setAboutData(about);
    setupUi(this);

    m_devices = new QStandardItemModel(this);
    deviceView->setModel(m_devices);

    connect(automountOnLogin, SIGNAL(stateChanged(int)), this, SLOT(emitChanged()));
    connect(automountOnPlugin, SIGNAL(stateChanged(int)), this, SLOT(emitChanged()));
    connect(automountEnabled, SIGNAL(stateChanged(int)), this, SLOT(emitChanged()));
    connect(automountUnknownDevices, SIGNAL(stateChanged(int)), this, SLOT(emitChanged()));
    connect(m_devices, SIGNAL(dataChanged(const QModelIndex, const QModelIndex)), this, SLOT(emitChanged()));
    connect(m_devices, SIGNAL(rowsInserted(const QModelIndex, int, int)), this, SLOT(emitChanged()));
    connect(m_devices, SIGNAL(rowsRemoved(const QModelIndex, int, int)), this, SLOT(emitChanged()));

    connect(automountEnabled, SIGNAL(stateChanged(int)), this, SLOT(enabledChanged()));

    connect(deviceView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection, const QItemSelection)), this, SLOT(updateForgetDeviceButton()));

    connect(forgetDevice, SIGNAL(clicked(bool)), this, SLOT(forgetSelectedDevices()));
    connect(addDevice, SIGNAL(clicked(bool)), this, SLOT(addNewDevice()));

    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString)), this, SLOT(deviceAttached(const QString)));
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(const QString)), this, SLOT(deviceRemoved(const QString)));

    forgetDevice->setEnabled(false);
}

void
DeviceAutomounterKCM::addNewDevice()
{
    const QString udi = KInputDialog::getText(i18n("Add Device"), i18n("Enter the device's system identifier"), 0, 0, this);
    if( !udi.isEmpty())
       addNewDevice(udi);
}

void
DeviceAutomounterKCM::addNewDevice(const QString &udi)
{
    Solid::Device dev(udi);
    bool valid = dev.isValid();
    
    KConfigGroup grp = AutomounterSettings::deviceSettings(udi);
    QList<QStandardItem*> row;
    QString displayName;
    if (valid)
        displayName = dev.description();
    else
        displayName = AutomounterSettings::getDeviceName(udi);
    if (displayName.isEmpty())
        displayName = "UDI: "+udi;
    QStandardItem *name = new QStandardItem(displayName);
    name->setData(udi, Qt::ToolTipRole);
    name->setData(udi);
    QStandardItem *shouldAutomount = new QStandardItem();
    if (grp.readEntry("ForceAutomount", false))
        shouldAutomount->setData(Qt::Checked, Qt::CheckStateRole);
    else
        shouldAutomount->setData(Qt::Unchecked, Qt::CheckStateRole);
    shouldAutomount->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
    
    QStandardItem *willLoginAutomount;
    QStandardItem *willAttachAutomount;
    if (AutomounterSettings::shouldAutomountDevice(udi, AutomounterSettings::Login))
        willLoginAutomount = new QStandardItem(i18n("Yes"));
    else
        willLoginAutomount = new QStandardItem(i18n("No"));
    if (AutomounterSettings::shouldAutomountDevice(udi, AutomounterSettings::Attach))
        willAttachAutomount = new QStandardItem(i18n("Yes"));
    else
        willAttachAutomount = new QStandardItem(i18n("No"));
    name->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    row << name << shouldAutomount << willLoginAutomount << willAttachAutomount;
        
    if (valid)
        m_attachedGroup->appendRow(row);
    else
        m_disconnectedGroup->appendRow(row);
    m_deviceMap[udi] = row[0]->index();
}


void
DeviceAutomounterKCM::deviceRemoved(const QString &udi) {
    Solid::Device dev(udi);
    if (AutomounterSettings::knownDevices().contains(udi) && dev.is<Solid::StorageVolume>()) {
        QModelIndex dev = m_deviceMap[udi];
        m_devices->itemFromIndex(dev.parent())->removeRow(dev.row());
        addNewDevice(udi);
    }
}

void
DeviceAutomounterKCM::deviceAttached(const QString &udi)
{
    Solid::Device dev(udi);
    if (!AutomounterSettings::knownDevices().contains(udi) && dev.is<Solid::StorageVolume>())
        addNewDevice(udi);
}

void
DeviceAutomounterKCM::updateForgetDeviceButton()
{
    forgetDevice->setEnabled(deviceView->selectionModel()->hasSelection());
}

void
DeviceAutomounterKCM::forgetSelectedDevices()
{
    const QModelIndexList selected = deviceView->selectionModel()->selectedRows();
    foreach(const QModelIndex &idx, selected) {
        kDebug() << "Deleting" << idx.row();
        m_devices->removeRow(idx.row());
    }
}

void
DeviceAutomounterKCM::emitChanged()
{
    changed();
}

void
DeviceAutomounterKCM::enabledChanged()
{
    if (automountEnabled->checkState() == Qt::Unchecked) {
        automountOnLogin->setEnabled(false);
        automountOnPlugin->setEnabled(false);
        automountUnknownDevices->setEnabled(false);
    } else {
        automountOnLogin->setEnabled(true);
        automountOnPlugin->setEnabled(true);
        automountUnknownDevices->setEnabled(true);
    }
}

void
DeviceAutomounterKCM::load()
{
    if (AutomounterSettings::automountEnabled())
        automountEnabled->setCheckState(Qt::Checked);
    else
        automountEnabled->setCheckState(Qt::Unchecked);

    if (AutomounterSettings::automountUnknownDevices())
        automountUnknownDevices->setCheckState(Qt::Unchecked);
    else
        automountUnknownDevices->setCheckState(Qt::Checked);

    if (AutomounterSettings::automountOnLogin())
        automountOnLogin->setCheckState(Qt::Checked);
    else
        automountOnLogin->setCheckState(Qt::Unchecked);

    if (AutomounterSettings::automountOnPlugin())
        automountOnPlugin->setCheckState(Qt::Checked);
    else
        automountOnPlugin->setCheckState(Qt::Unchecked);

    reloadDevices();

    enabledChanged();
}

void
DeviceAutomounterKCM::reloadDevices()
{
    AutomounterSettings::self()->readConfig();
    m_devices->clear();
    QStringList headers;
    headers << i18nc("@title:column The device's internal UDI if not attached, user-friendly name reported by Solid otherwise.", "Name" ) << i18n( "Always Automount" ) << i18n( "Automount on login" ) << i18n("Automount on attach");
    m_devices->setHorizontalHeaderLabels(headers);
    m_attachedGroup = new QStandardItem(i18nc("@title:group Group of devices currently attached to the computer", "Attached Devices"));
    m_disconnectedGroup = new QStandardItem(i18nc("@title:group Group of devices currently not attached to the computer", "Disconnected Devices"));
    m_devices->appendRow(m_attachedGroup);
    m_devices->appendRow(m_disconnectedGroup);
    foreach(const QString &dev, AutomounterSettings::knownDevices()) {
        addNewDevice(dev);
    }
    deviceView->resizeColumnToContents(0);
}

void
DeviceAutomounterKCM::save()
{
    if (this->automountEnabled->checkState() == Qt::Checked)
        AutomounterSettings::setAutomountEnabled(true);
    else
        AutomounterSettings::setAutomountEnabled(false);

    if (this->automountUnknownDevices->checkState() == Qt::Checked)
        AutomounterSettings::setAutomountUnknownDevices(false);
    else
        AutomounterSettings::setAutomountUnknownDevices(true);

    if (this->automountOnLogin->checkState() == Qt::Checked)
        AutomounterSettings::setAutomountOnLogin(true);
    else
        AutomounterSettings::setAutomountOnLogin(false);

    if (this->automountOnPlugin->checkState() == Qt::Checked)
        AutomounterSettings::setAutomountOnPlugin(true);
    else
        AutomounterSettings::setAutomountOnPlugin(false);

    int i;
    QStringList validDevices;
    for(i = 0;i < m_attachedGroup->rowCount();++i) {
        QStandardItem *udi = m_attachedGroup->child(i, 0);
        QStandardItem *automount = m_attachedGroup->child(i, 1);
        QString device = udi->data().toString();
        validDevices << device;
        if (automount->data(Qt::CheckStateRole).toInt() == Qt::Checked)
            AutomounterSettings::deviceSettings(device).writeEntry("ForceAutomount", true);
        else
            AutomounterSettings::deviceSettings(device).writeEntry("ForceAutomount", false);
    }
    
    for(i = 0;i < m_disconnectedGroup->rowCount();++i) {
        QStandardItem *udi = m_disconnectedGroup->child(i, 0);
        QStandardItem *automount = m_disconnectedGroup->child(i, 1);
        QString device = udi->data().toString();
        validDevices << device;
        if (automount->data(Qt::CheckStateRole).toInt() == Qt::Checked)
            AutomounterSettings::deviceSettings(device).writeEntry("ForceAutomount", true);
        else
            AutomounterSettings::deviceSettings(device).writeEntry("ForceAutomount", false);
    }
    
    foreach(const QString &possibleDevice, AutomounterSettings::knownDevices()) {
        if (!validDevices.contains(possibleDevice))
            AutomounterSettings::deviceSettings(possibleDevice).deleteGroup();
    }

    AutomounterSettings::self()->writeConfig();
}

DeviceAutomounterKCM::~DeviceAutomounterKCM()
{
}

