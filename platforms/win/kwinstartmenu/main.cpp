/* This file is part of the KDE Project

   Copyright (C) 2006-2011 Ralf Habacker <ralf.habacker@freenet.de>
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
#include <KConfig>
#include <KConfigGroup>
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
    KAboutData about("kwinstartmenu", "kdebase-runtime", ki18n("kwinstartmenu"), "1.2",
                     ki18n("An application to create/update or remove Windows Start Menu entries"),
                     KAboutData::License_GPL,
                     ki18n("(C) 2008-2010 Ralf Habacker"));
    KCmdLineArgs::init( argc, argv, &about);

    KCmdLineOptions options;
    options.add("remove",     ki18n("remove installed start menu entries"));
    options.add("install",    ki18n("install start menu entries"));
    options.add("update",     ki18n("update start menu entries"));
    options.add("cleanup",    ki18n("remove start menu entries from unused kde installation"));
    options.add("query-path", ki18n("query root path of start menu entries"));
    options.add("nocategories", ki18n("don't use categories for start menu entries"));
    options.add("set-root-custom-string <argument>", ki18n("set custom string for root start menu entry"));
    options.add("remove-root-custom-string", ki18n("remove custom string from root start menu entry"));
    options.add("root-custom-string", ki18n("query current value of root start menu entry custom string"));
    KCmdLineArgs::addCmdLineOptions( options ); // Add my own options.

    KComponentData a(&about);

    // Get application specific arguments
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KApplication app(false);
    
    // set global options from config file
    // @todo merge with related stuff in kwinstartmenu.cpp 
    KConfig config("kwinstartmenurc");
    KConfigGroup group( &config, "General" );
    globalOptions.useCategories = group.readEntry("useCategories", true);
    globalOptions.rootCustomString = group.readEntry("rootCustomString", "");
    
    // override global settings from command line 
    if (args->isSet("categories"))
    {
        globalOptions.useCategories = args->isSet("categories");
        group.writeEntry("useCategories",globalOptions.useCategories);
    }
    if (args->isSet("root-custom-string"))
        fprintf(stdout,"%s",qPrintable(globalOptions.rootCustomString));
    else if (args->isSet("remove-root-custom-string"))
    {
        globalOptions.rootCustomString = "";
        group.writeEntry("rootCustomString",globalOptions.rootCustomString);
    }
    else 
    {
        QString rootCustomString = args->getOption("set-root-custom-string");
        if (!rootCustomString.isEmpty())
        {
            globalOptions.rootCustomString = rootCustomString;
            group.writeEntry("rootCustomString",rootCustomString);
        }
    }
    
    if (args->isSet("query-path"))
        fprintf(stdout,"%s",qPrintable(QDir::toNativeSeparators(getKDEStartMenuPath())));
    else if (args->isSet("remove"))
        removeStartMenuLinks();
    else if (args->isSet("update"))
        updateStartMenuLinks();
    else if (args->isSet("install"))
        updateStartMenuLinks();
    else if (args->isSet("cleanup"))
        cleanupStartMenuLinks();

    return 0;
}
   
// vim: ts=4 sw=4 et
