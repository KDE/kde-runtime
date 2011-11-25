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
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kcomponentdata.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>

//void listAllGroups(const QString &groupName=QString());

int main(int argc, char **argv)
{
    KAboutData about("kwinstartmenu", 0, ki18n("kwinstartmenu"), "1.4",
                     ki18n("An application to create/update or remove Windows Start Menu entries"),
                     KAboutData::License_GPL,
                     ki18n("(C) 2008-2011 Ralf Habacker"));
    KCmdLineArgs::init( argc, argv, &about);

    KCmdLineOptions options;
    options.add("remove",     ki18n("remove installed start menu entries"));
    options.add("install",    ki18n("install start menu entries"));
    options.add("update",     ki18n("update start menu entries"));
    options.add("cleanup",    ki18n("remove start menu entries from unused kde installation"));

    options.add("query-path", ki18n("query root path of start menu entries"));

    options.add("enable-categories", ki18n("use categories for start menu entries (default)"));
    options.add("disable-categories", ki18n("don't use categories for start menu entries"));
    options.add("categories", ki18n("query current value of categories in start menu"));
    
    options.add("set-custom-string <argument>", ki18n("set custom string for root start menu entry"));
    // @TODO unset-custom-string is required because args->isSet("set-root-custom-string") do not
    // detect --set-custom-string ""  as present set-root-custom-string option and
    options.add("unset-custom-string", ki18n("remove custom string from root start menu entry"));
    options.add("custom-string", ki18n("query current value of root start menu entry custom string"));

    options.add("set-name-string <argument>", ki18n("set custom name string for root start menu entry"));
    options.add("unset-name-string", ki18n("remove custom name string from root start menu entry"));
    options.add("name-string", ki18n("query current value of start menu entry custom name string"));

    options.add("set-version-string <argument>", ki18n("set custom version string for root start menu entry"));
    options.add("unset-version-string", ki18n("remove custom version string from root start menu entry"));
    options.add("version-string", ki18n("query current value of root start menu entry version string"));
    KCmdLineArgs::addCmdLineOptions( options ); // Add my own options.

    KComponentData a(&about);

    // Get application specific arguments
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KApplication app(false);

    // override global settings from command line 
    if (args->isSet("categories"))
        fprintf(stdout,"%s",settings.useCategories() ? "on" : "off");
    else if (args->isSet("enable-categories"))
    {
        settings.setUseCategories(true);
    }
    else if (args->isSet("disable-categories"))
    {
        settings.setUseCategories(false);
    }

    if (args->isSet("custom-string"))
        fprintf(stdout,"%s",qPrintable(settings.customString()));
    else if (args->isSet("unset-custom-string"))
    {
        settings.setCustomString("");
    }
    else if (args->isSet("set-custom-string"))
    {
        settings.setCustomString(args->getOption("set-custom-string"));
    }

    if (args->isSet("name-string"))
        fprintf(stdout,"%s",qPrintable(settings.nameString()));
    else if (args->isSet("unset-name-string"))
    {
        settings.setNameString("");
    }
    else if (args->isSet("set-name-string"))
    {
        settings.setNameString(args->getOption("set-name-string"));
    }

    if (args->isSet("version-string"))
        fprintf(stdout,"%s",qPrintable(settings.versionString()));
    else if (args->isSet("unset-version-string"))
    {
        settings.setVersionString("");
    }
    else if (args->isSet("set-version-string"))
    {
        settings.setVersionString(args->getOption("set-version-string"));
    }

    // determine initial values on fresh install or remove
    if (settings.nameString().isEmpty() && settings.versionString().isEmpty() &&  settings.customString().isEmpty())
    {
        if (args->isSet("install"))
        {
            QString version = KDE::versionString();
            QStringList versions = version.split(' ');
            settings.setVersionString(versions[0]);
            settings.setNameString("KDE");
            kWarning() << "no name, version or custom string set, using default values for install" << settings.nameString() << settings.versionString();
        }
        else if (args->isSet("remove") || args->isSet("update"))
        {
            kError() << "cannot remove/update start menu entries: no name, version or custom string set";
            return 1;
        }
    }
    /**
      @TODO how to solve the case if any --set-... option is used and the start menu entries are not removed before
      should we remove them before, to have a clean state or better something else ?
    */
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

// vim: ts=4 sw4 et
