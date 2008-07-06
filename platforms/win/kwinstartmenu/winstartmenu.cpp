/* This file is part of the KDE project

   Copyright (C) 2008 Ralf Habacker <ralf.habacker@freenet.de>
   All rights reserved.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "winstartmenu.h"
#include "winstartmenu_adaptor.h"
#include "misc.h"

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kstandarddirs.h>
#include <ksycoca.h>

K_PLUGIN_FACTORY(WinStartMenuFactory,
                 registerPlugin<WinStartMenuModule>();
    )
K_EXPORT_PLUGIN(WinStartMenuFactory("WinStartMenu"))


struct WinStartMenuModulePrivate
{
    WinStartMenuModulePrivate()
     : config("kwinstartmenurc")
    {
    }
    virtual ~WinStartMenuModulePrivate() 
    { 
    }

    KConfig config;
};

WinStartMenuModule::WinStartMenuModule(QObject* parent, const QList<QVariant>&)
    : KDEDModule(parent)
    , d( new WinStartMenuModulePrivate )
{
    KConfigGroup group( &d->config, "General" );

    if (group.readEntry("Enabled", true))
      connect(KSycoca::self(), SIGNAL(databaseChanged()), this, SLOT(databaseChanged()));
    
    new WinStartMenuAdaptor( this );
}

WinStartMenuModule::~WinStartMenuModule()
{
    disconnect(KSycoca::self(), SIGNAL(databaseChanged()), this, SLOT(databaseChanged()));
    delete d;
}

void WinStartMenuModule::databaseChanged() 
{
    kDebug() << "database changed"; 
    createStartMenuEntries();
}

void WinStartMenuModule::removeStartMenuEntries()
{
    removeDirectory(getKDEStartMenuPath());
}

void WinStartMenuModule::createStartMenuEntries()
{
    updateStartMenuLinks();
}

#include "winstartmenu.moc"

// vim: ts=4 sw=4 et
