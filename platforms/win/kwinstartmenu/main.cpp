/* This file is part of the KDE Project

   Copyright (C) 2006-2008 Ralf Habacker <ralf.habacker@freenet.de>
   All rights reserved.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "misc.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <KAboutData>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kcomponentdata.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstandarddirs.h>

//void listAllGroups(const QString &groupName=QString());

int main(int argc, char **argv)
{
    KAboutData about("winstartmenu", "kdebase-runtime", ki18n("winstartmenu"), "1.0",
                     ki18n("An application to create/update or remove windows start menu entries"),
                     KAboutData::License_GPL,
                     ki18n("(C) 2008 Ralf Habacker"));
    KCmdLineArgs::init( argc, argv, &about);

    KCmdLineOptions options;
    options.add("remove",     ki18n("remove installed start menu entries"));
    options.add("install",     ki18n("install start menu entries (this is the default also when this option is not set)"));
    KCmdLineArgs::addCmdLineOptions( options ); // Add my own options.

    KComponentData a(&about);
//  (void)KGlobal::dirs(); // trigger the creation
//  (void)KGlobal::config();

    // Get application specific arguments
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KApplication app(false);

    if (args->isSet("remove"))
        removeStartMenuLinks();
    else
        updateStartMenuLinks();
}

    
// vim: ts=4 sw=4 et
