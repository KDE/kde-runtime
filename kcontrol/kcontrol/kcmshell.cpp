/*
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
  Copyright (c) 2000 Matthias Elter <elter@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <iostream.h>
#include <stdlib.h>

#include <qfile.h>

#include <kapp.h>
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kstddirs.h>
#include <kcmdlineargs.h>
#include <kservice.h>
#include <kdesktopfile.h>
#include <qxembed.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <klibloader.h>

#include "kcdialog.h"
#include "kecdialog.h"
#include "moduleinfo.h"
#include "modloader.h"
#include "global.h"


static KCmdLineOptions options[] =
{
    { "list", I18N_NOOP("List all possible modules"), 0},
    { "+module", I18N_NOOP("Configuration module to open."), 0 },
    { "embed <id>", I18N_NOOP("Window ID to embed into."), 0 },
    KCmdLineLastOption
};

static QString locateModule(const QCString module)
{
    // locate the desktop file
    //QStringList files;
    if (module[0] == '/')
    {
        kdDebug() << "Full path given to kcmshell - not supported yet" << endl;
        // (because of KService::findServiceByDesktopPath)
        //files.append(args->arg(0));
    }

    QString path = KCGlobal::baseGroup();
    path += module;
    path += ".desktop";

    if (!KService::serviceByDesktopPath( path ))
    {
        // Path didn't work. Trying as a name
        KService::Ptr serv = KService::serviceByDesktopName( module );
        if ( serv )
            path = serv->entryPath();
        else
        {
            cerr << i18n("Module %1 not found!").arg(module).local8Bit() << endl;
            return QString::null;
        }
    }
    return path;
}

int main(int _argc, char *_argv[])
{
    KAboutData aboutData( "kcmshell", I18N_NOOP("KDE Control Module"),
                          "2.0",
                          I18N_NOOP("A tool to start single KDE control modules"),
                          KAboutData::License_GPL,
                          "(c) 1999-2000, The KDE Developers");
    aboutData.addAuthor("Matthias Hoelzer-Kluepfel",0, "hoelzer@kde.org");
    aboutData.addAuthor("Matthias Elter",0, "elter@kde.org");

    KCmdLineArgs::init(_argc, _argv, &aboutData);
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
    KLocale::setMainCatalogue("kcontrol");
    KApplication app;

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    KGlobal::iconLoader()->addAppDir( "kcontrol" );

    if (args->isSet("list")) {
        QStringList files;
        KGlobal::dirs()->findAllResources("apps",
                                          KCGlobal::baseGroup() + "*.desktop",
                                          true, true, files);
        QStringList modules;
        QStringList descriptions;
        uint maxwidth = 0;
        for (QStringList::ConstIterator it = files.begin(); it != files.end(); it++) {

            if (KDesktopFile::isDesktopFile(*it)) {
                KDesktopFile file(*it, true);
                QString module = *it;
                if (module.startsWith(KCGlobal::baseGroup()))
                    module = module.mid(KCGlobal::baseGroup().length());
                if (module.right(8) == ".desktop")
                    module.truncate(module.length() - 8);

                modules.append(module);
                if (module.length() > maxwidth)
                    maxwidth = module.length();
                descriptions.append(QString("%2 (%3)").arg(file.readName()).arg(file.readComment()));
            }
        }

        QByteArray vl;
        vl.fill(' ', 80);
        QString verylong = vl;

        for (uint i = 0; i < modules.count(); i++) {
            cout << (*modules.at(i)).local8Bit();
            cout << verylong.left(maxwidth - (*modules.at(i)).length()).local8Bit();
            cout << " - ";
            cout << (*descriptions.at(i)).local8Bit();
            cout << endl;
        }

        return 0;
    }

    if (args->count() < 1) {
        args->usage();
        return -1;
    }

    if (args->count() == 1) {

        QString path = locateModule(args->arg(0));

        // load the module
        ModuleInfo info(path);

        KCModule *module = ModuleLoader::loadModule(info, false);

        if (module) {
            // create the dialog
            KCDialog * dlg = new KCDialog(module, module->buttons(), info.docPath(), 0, 0, true);
            dlg->setCaption(info.name());

            // Needed for modules that use d'n'd (not really the right
            // solution for this though, I guess)
            dlg->setAcceptDrops(true);

            // if we are going to be embedded, embed
            QCString embed = args->getOption("embed");
            if (!embed.isEmpty())
            {
                bool ok;
                int id = embed.toInt(&ok);
                if (ok)
                    QXEmbed::embedClientIntoWindow(dlg, id);
            }

            // run the dialog
            int ret = dlg->exec();
            delete dlg;
            ModuleLoader::unloadModule(info);
            return ret;
        }

        KMessageBox::error(0,
                           i18n("There was an error loading the module.\nThe diagnostics is:\n%1")
                           .arg(KLibLoader::self()->lastErrorMessage()));
        return 0;
    }

    // multiple control modules
    QStringList modules;
    for (int i = 0; i < args->count(); i++) {
        QString path = locateModule(args->arg(i));
        if(!path.isEmpty())
            modules.append(path);
    }

    if (modules.count() < 1) return -1;

    // create the dialog
    KExtendedCDialog * dlg = new KExtendedCDialog(0, 0, true);

    // Needed for modules that use d'n'd (not really the right
    // solution for this though, I guess)
    dlg->setAcceptDrops(true);

    // add modules
    for (QStringList::ConstIterator it = modules.begin(); it != modules.end(); ++it)
        dlg->addModule(*it, false);

    // if we are going to be embedded, embed
    QCString embed = args->getOption("embed");
    if (!embed.isEmpty())
    {
        bool ok;
        int id = embed.toInt(&ok);
        if (ok)
            QXEmbed::embedClientIntoWindow(dlg, id);
    }

    // run the dialog
    int ret = dlg->exec();
    delete dlg;
    return ret;
}
