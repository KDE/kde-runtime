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

#include "DeviceAutomounterKCM.h"

#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>
#include <QtCore/QList>

#include <KGenericFactory>
#include <KIcon>
#include <KConfigDialogManager>
#include <KAboutData>

#include "AutomounterSettings.h"

K_PLUGIN_FACTORY(DeviceAutomounterKCMFactory, registerPlugin<DeviceAutomounterKCM>();)
K_EXPORT_PLUGIN(DeviceAutomounterKCMFactory("device_automounter"))

DeviceAutomounterKCM::DeviceAutomounterKCM(QWidget *parent, const QVariantList &)
    : KCModule(DeviceAutomounterKCMFactory::componentData(), parent)
{
    KAboutData *about = new KAboutData("device_automounter",
                                       0,
                                       ki18n("Device Automounter"),
                                       "0.1",
                                       ki18n("Automatically mounts devices at login or when attached"),
                                       KAboutData::License_GPL_V2,
                                       ki18n("(c) 2009 Trever Fischer"));
    about->addAuthor(ki18n("Trever Fischer"));
    setAboutData(about);
    setupUi(this);
    connect(automountOnLogin, SIGNAL(stateChanged(int)), this, SLOT(emitChanged()));
    connect(automountOnPlugin, SIGNAL(stateChanged(int)), this, SLOT(emitChanged()));
}

void
DeviceAutomounterKCM::emitChanged()
{
    changed();
}

void
DeviceAutomounterKCM::load()
{
    if (AutomounterSettings::automountOnLogin())
        automountOnLogin->setCheckState(Qt::Checked);
    else
        automountOnLogin->setCheckState(Qt::Unchecked);
    
    if (AutomounterSettings::automountOnPlugin())
        automountOnPlugin->setCheckState(Qt::Checked);
    else
        automountOnPlugin->setCheckState(Qt::Unchecked);
}

void
DeviceAutomounterKCM::save()
{
    if (this->automountOnLogin->checkState())
        AutomounterSettings::setAutomountOnLogin(true);
    else
        AutomounterSettings::setAutomountOnLogin(false);
    
    if (this->automountOnPlugin->checkState())
        AutomounterSettings::setAutomountOnPlugin(true);
    else
        AutomounterSettings::setAutomountOnPlugin(false);
    
    AutomounterSettings::self()->writeConfig();
}

DeviceAutomounterKCM::~DeviceAutomounterKCM()
{
}

