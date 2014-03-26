/***************************************************************************
 *   Copyright (C) 2009 Patrick Spendrin <ps_ml@gmx.de>                  *
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

#include "menuManager.h"

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <KStandardDirs>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>

class MenuManagerPrivate 
{
    public:
        MenuManagerPrivate() : config(new KConfig(KStandardDirs::installPath("config") + "kwinstartmenurc"))
        {
            kDebug() << "kwinstartmenu's configuration is writable:" 
                     << config->isConfigWritable(true);
        }

        KConfig *config;
};

MenuManager::MenuManager() : d( new MenuManagerPrivate )
{
}

MenuManager::~MenuManager()
{
    delete d->config;
    delete d;
}

bool MenuManager::isMenuCreationEnabled() const
{
    KConfigGroup group( d->config, "General" );

    if (group.readEntry("Enabled", true))
        return true;
    else
        return false;
}

void MenuManager::setMenuCreationEnabled(bool menu)
{
    KConfigGroup group( d->config, "General" );

    QDBusConnection bus = QDBusConnection::sessionBus();
    QDBusInterface *interface = new QDBusInterface("org.kde.kded", "/kded"); 
    
    interface->call("unloadModule", QString("winstartmenu"));
    if(!group.isImmutable())
        group.writeEntry("Enabled", menu);
    else
        kDebug() << "Group is immutable!!!";
    d->config->sync();
    interface->call("loadModule", QString("winstartmenu"));
    delete interface; 
}

