/*
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <qfile.h>

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kservice.h>
#include <klibloader.h>
#include <kdebug.h>
#include <dcopclient.h>


#include "global.h"

static KCmdLineOptions options[] =
{
    { "list", I18N_NOOP("List modules that are run at startup."), 0 },
    { "+module", I18N_NOOP("Configuration module to run."), 0 },
    KCmdLineLastOption
};

static bool runModule(const QString &libName, KLibLoader *loader, KService::Ptr service)
{
    KLibrary *lib = loader->library(QFile::encodeName(libName));
    if (lib) {
	// get the init_ function
	QString factory = QString("init_%1").arg(service->init());
	void *init = lib->symbol(factory.utf8());
	if (init) {
	    // initialize the module
	    kdDebug(1208) << "Initializing " << libName << ": " << factory << endl;
	    
	    void (*func)() = (void(*)())init;
	    func();
	    return true;
	}
	loader->unloadLibrary(QFile::encodeName(libName));
    }
    return false;
}

extern "C" int kdemain(int argc, char *argv[])
{
  KLocale::setMainCatalogue("kcontrol");
  KAboutData aboutData( "kcminit", I18N_NOOP("KCMInit"),
	"$Id$",
	I18N_NOOP("KCMInit - runs startups initialization for Control Modules."));

  KCmdLineArgs::init(argc, argv, &aboutData);
  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

  KApplication app;
  KLocale::setMainCatalogue(0);

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  QCString arg;
  if (args->count() == 1) {
    arg = args->arg(0);
  }

  // get the library loader instance
  KLibLoader *loader = KLibLoader::self();

  KService::List list;

  if (args->isSet("list"))
  {
    list = KService::allInitServices();
    
    for(KService::List::Iterator it = list.begin();
        it != list.end();
        ++it)
    {
      KService::Ptr service = (*it);
      if (service->library().isEmpty() || service->init().isEmpty())
	continue; // Skip
      printf("%s\n", QFile::encodeName(service->desktopEntryName()).data());
    }
    return 0;
  }

  if (!arg.isEmpty()) {

    QString module = QFile::decodeName(arg);
    if (!module.endsWith(".desktop"))
       module += ".desktop";

    KService::Ptr serv = KService::serviceByStorageId( module );
    if ( !serv || serv->library().isEmpty() ||
	 serv->init().isEmpty()) {
      kdError(1208) << i18n("Module %1 not found!").arg(module) << endl;
      return -1;
    } else
      list.append(serv);

  } else {

    // locate the desktop files
    list = KService::allInitServices();

  }

  QStrList alreadyInitialized;

  // look for X-KDE-Init=... entries
  for(KService::List::Iterator it = list.begin();
      it != list.end();
      ++it) {
      KService::Ptr service = (*it);
      if (service->library().isEmpty() || service->init().isEmpty())
	continue; // Skip

      QString libName = QString("kcm_%1").arg(service->library());

      // try to load the library
      if (! alreadyInitialized.contains( libName.ascii() )) {
	  if (!runModule(libName, loader, service)) {
	      libName = QString("libkcm_%1").arg(service->library());
	      if (! alreadyInitialized.contains( libName.ascii() )) {
		  runModule(libName, loader, service);
		  alreadyInitialized.append( libName.ascii() );
	      }
	  } else 
	      alreadyInitialized.append( libName.ascii() );
      }
  }

  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "ksplash", "", "upAndRunning(QString)",  QString("kcminit"));

  return 0;
}
