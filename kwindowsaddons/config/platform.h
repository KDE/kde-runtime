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

#ifndef PLATFORM_H
#define PLATFORM_H

#include "registryManager.h"
#include "menuManager.h"
#include <kcmodule.h>
#include "ui_platform.h"

class Platform : public KCModule, Ui::PlatformThing
{
    Q_OBJECT

public:
    Platform(QWidget *parent, const QVariantList &args);
    ~Platform();
    void save();
    void load();

private slots:
    void somethingChanged();
    void showCustShellDlg();

private:
    QMap<QString, QString> customShell;
    RegistryManager::Shell currentShell;

    RegistryManager regMan;
    MenuManager menuMan;
};

#endif /* PLATFORM_H */
