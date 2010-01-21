/**************************************************************************
*   Copyright (C) 2009-2010 Trever Fischer <tdfischer@fedoraproject.org>  *
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

#include <KGlobal>
#include <KLocale>

#include "AutomounterSettings.h"
#include "LayoutSettings.h"
#include "DeviceModel.h"

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

    m_devices = new DeviceModel(this);
    deviceView->setModel(m_devices);

    connect(automountOnLogin, SIGNAL(stateChanged(int)), this, SLOT(emitChanged()));
    connect(automountOnPlugin, SIGNAL(stateChanged(int)), this, SLOT(emitChanged()));
    connect(automountEnabled, SIGNAL(stateChanged(int)), this, SLOT(emitChanged()));
    connect(automountUnknownDevices, SIGNAL(stateChanged(int)), this, SLOT(emitChanged()));
    connect(m_devices, SIGNAL(dataChanged(const QModelIndex, const QModelIndex)), this, SLOT(emitChanged()));

    connect(automountEnabled, SIGNAL(stateChanged(int)), this, SLOT(enabledChanged()));

    connect(deviceView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection, const QItemSelection)), this, SLOT(updateForgetDeviceButton()));

    connect(forgetDevice, SIGNAL(clicked(bool)), this, SLOT(forgetSelectedDevices()));

    forgetDevice->setEnabled(false);
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
        m_devices->forgetDevice(idx.data(Qt::UserRole).toString());
    }
    changed();
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
        deviceView->setEnabled(false);
    } else {
        automountOnLogin->setEnabled(true);
        automountOnPlugin->setEnabled(true);
        automountUnknownDevices->setEnabled(true);
        deviceView->setEnabled(true);
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

    m_devices->reload();
    enabledChanged();
    loadLayout();
}

void
DeviceAutomounterKCM::save()
{
    saveLayout();
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

    QStringList validDevices;
    for(int i = 0;i < m_devices->rowCount();i++) {
        QModelIndex idx = m_devices->index(i, 0);
        for(int j = 0;j < m_devices->rowCount(idx);j++) {
            QModelIndex dev = m_devices->index(j, 1, idx);
            QString device = dev.data(Qt::UserRole).toString();
            validDevices << device;
            if (dev.data(Qt::CheckStateRole).toInt() == Qt::Checked)
                AutomounterSettings::deviceSettings(device).writeEntry("ForceLoginAutomount", true);
            else
                AutomounterSettings::deviceSettings(device).writeEntry("ForceLoginAutomount", false);
            dev = dev.sibling(j, 2);
            if (dev.data(Qt::CheckStateRole).toInt() == Qt::Checked)
                AutomounterSettings::deviceSettings(device).writeEntry("ForceAttachAutomount", true);
            else
                AutomounterSettings::deviceSettings(device).writeEntry("ForceAttachAutomount", false);
        }
    }

    foreach(const QString &possibleDevice, AutomounterSettings::knownDevices()) {
        if (!validDevices.contains(possibleDevice))
            AutomounterSettings::deviceSettings(possibleDevice).deleteGroup();
    }

    AutomounterSettings::self()->writeConfig();
}

DeviceAutomounterKCM::~DeviceAutomounterKCM()
{
    saveLayout();
}

void
DeviceAutomounterKCM::saveLayout()
{
    QList<int> widths;
    for(int i = 0;i<m_devices->columnCount();i++)
        widths << deviceView->columnWidth(i);
    LayoutSettings::setHeaderWidths(widths);
    //Check DeviceModel.cpp, thats where the magic row numbers come from.
    LayoutSettings::setAttachedExpanded(deviceView->isExpanded(m_devices->index(0,0)));
    LayoutSettings::setDetatchedExpanded(deviceView->isExpanded(m_devices->index(1,0)));
    LayoutSettings::self()->writeConfig();
}

void
DeviceAutomounterKCM::loadLayout()
{
    LayoutSettings::self()->readConfig();
    //Reset it first, just in case there isn't any layout saved for a particular column.
    for(int i = 0;i<m_devices->columnCount();i++)
        deviceView->resizeColumnToContents(i);

    QList<int> widths = LayoutSettings::headerWidths();
    for(int i = 0;i<m_devices->columnCount() && i<widths.size();i++) {
        deviceView->setColumnWidth(i, widths[i]);
    }

    deviceView->setExpanded(m_devices->index(0,0), LayoutSettings::attachedExpanded());
    deviceView->setExpanded(m_devices->index(1,0), LayoutSettings::detatchedExpanded());
}
