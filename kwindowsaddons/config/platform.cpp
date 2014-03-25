/***************************************************************************
 *   Copyright (C) 2009 Eduard Sukharev <kraplax@mail.ru>                  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "platform.h"

#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QtDebug>

#include <KAboutData>
#include <kpluginfactory.h>

#include "registryManager.h"
#include "shellEdit.h"

K_PLUGIN_FACTORY(PlatformFactory, registerPlugin<Platform>();)
K_EXPORT_PLUGIN(PlatformFactory("platform", "kcm_platform"))

Platform::Platform(QWidget *parent, const QVariantList &args)
        : KCModule(PlatformFactory::componentData(), parent, args), regMan(this)
{
    KAboutData *about = new KAboutData("kcm_platform", 0, ki18n("Platform"), "0.1");
    setAboutData(about);
    setButtons(Apply | Help);
    setupUi(this);

    currentShell = regMan.getCurShell();
    optCustom->setText(regMan.customShell.value("Name"));
    descrCustom->setText(regMan.customShell.value("Description"));

    if(currentShell == RegistryManager::Native){
        optNative->setChecked(true);
    } else if (currentShell == RegistryManager::Plasma) {
        optPlasma->setChecked(true);
    } else {
        optCustom->setChecked(true);
    }

    chkInstallCPl->setChecked(regMan.isCPlEntryInstalled());
    chkInstallCursors->setChecked(regMan.isCursorsInstalled());
    chkUseNativeDialogs->setChecked(regMan.isNativeDialogsUsed());
    chkDisableMenuGen->setChecked(menuMan.isMenuCreationEnabled());
    chkStartAtLogin->setChecked(regMan.isLoadedAtLogin());

    Qt::CheckState wpState = regMan.isWallpapersInstalled();
    if (wpState == Qt::PartiallyChecked){
        chkInstallWallpapers->setTristate(true);
        chkInstallWallpapers->setCheckState(Qt::PartiallyChecked);
    } else {
        chkInstallWallpapers->setTristate(false);
        chkInstallWallpapers->setCheckState(wpState);
    }

    connect(optNative, SIGNAL(toggled(bool)), this, SLOT(somethingChanged()));
    connect(optPlasma, SIGNAL(toggled(bool)), this, SLOT(somethingChanged()));
    connect(chkInstallCPl, SIGNAL(toggled(bool)), this, SLOT(somethingChanged()));
    connect(chkDisableMenuGen, SIGNAL(toggled(bool)), this, SLOT(somethingChanged()));
    connect(chkInstallCursors, SIGNAL(toggled(bool)), this, SLOT(somethingChanged()));
    connect(chkUseNativeDialogs, SIGNAL(toggled(bool)), this, SLOT(somethingChanged()));
    connect(chkInstallWallpapers, SIGNAL(stateChanged(int)), this, SLOT(somethingChanged()));
    connect(chkStartAtLogin, SIGNAL(stateChanged(int)), this, SLOT(somethingChanged()));
    connect(btnShellSetup, SIGNAL(pressed()), this, SLOT(showCustShellDlg()));

    emit changed(false);
}

void Platform::somethingChanged()
{
    emit changed(true);
}

Platform::~Platform()
{
}

void Platform::showCustShellDlg()
{
    ShellEdit shellEditDlg(regMan.customShell.value("Name"), regMan.customShell.value("Description"), regMan.customShell.value("Command"));

    if(shellEditDlg.exec()){
        optCustom->setText(shellEditDlg.shellNameEdit->text());
        descrCustom->setText(shellEditDlg.shellDescrEdit->text());

        regMan.customShell["Name"] = shellEditDlg.shellNameEdit->text();
        regMan.customShell["Description"] = shellEditDlg.shellDescrEdit->text();
        regMan.customShell["Command"] = shellEditDlg.shellCmdEdit->text();
    }

    emit changed(true);
}

void Platform::save()
{
    qDebug() << "Calling it save";

    if(optNative->isChecked()){
        regMan.setCurShell(RegistryManager::Native);
    } else if(optPlasma->isChecked()){
        regMan.setCurShell(RegistryManager::Plasma);
    } else if (optCustom->isChecked()){
        regMan.setCurShell(RegistryManager::Custom);
    }

    if(chkInstallCPl->isChecked()){
        regMan.installCPlEntry();
    } else {
        regMan.uninstallCPlEntry();
    }

    if(chkInstallCursors->isChecked()){
        regMan.installCursors();
    } else {
        regMan.uninstallCursors();
    }

    if(chkInstallWallpapers->checkState() == Qt::Checked){
        regMan.installWallpapers();
    } else if (chkInstallWallpapers->checkState() == Qt::Unchecked) {
        regMan.uninstallWallpapers();
    }

    regMan.setUseNativeDialogs(chkUseNativeDialogs->isChecked());
    regMan.setLoadAtLogin(chkUseNativeDialogs->isChecked());
    menuMan.setMenuCreationEnabled(chkDisableMenuGen->isChecked());
}

void Platform::load()
{
}
