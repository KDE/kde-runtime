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

#include <qstringlist.h>
#include <qfile.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klibloader.h>
#include <kparts/componentfactory.h>

#include "config.h"
#include "global.h"
#include "modloader.h"

static KCModule *load(const ModuleInfo &mod, const QString &libname, KLibLoader *loader)
{
    // attempt to load modules with ComponentFactory, only if the symbol init_<lib> exists
    // (this is because some modules, e.g. kcmkio with multiple modules in the library,
    // cannot be ported to KGenericFactory)
    KLibrary *lib = loader->library(QFile::encodeName(libname.arg(mod.library())));
    if (lib) {
        QString initSym("init_");
        initSym += libname.arg(mod.library());

        if ( lib->hasSymbol(QFile::encodeName(initSym)) )
        {
            // Reuse "lib" instead of letting createInstanceFromLibrary recreate it
            //KCModule *module = KParts::ComponentFactory::createInstanceFromLibrary<KCModule>(QFile::encodeName(libname.arg(mod.library())));
            KLibFactory *factory = lib->factory();
            if ( factory )
            {
                KCModule *module = KParts::ComponentFactory::createInstanceFromFactory<KCModule>( factory );
                if (module)
                    return module;
            }
            // else do a fallback
            kdDebug() << "Unable to load module using ComponentFactory! Falling back to old loader." << endl;
        }
    
	// get the create_ function
	QString factory("create_%1");
	void *create = lib->symbol(QFile::encodeName(factory.arg(mod.handle())));
	
	if (create)
	    {
		// create the module
		KCModule* (*func)(QWidget *, const char *);
		func = (KCModule* (*)(QWidget *, const char *)) create;
		return  func(0, 0);
	    }

        lib->unload();
    }
    return 0;
}


KCModule *ModuleLoader::loadModule(const ModuleInfo &mod, bool withfallback)
{
  /*
   * Simple libraries as modules are the easiest case:
   *  We just have to load the library and get the module
   *  from the factory.
   */

  if (!mod.library().isEmpty())
  {
      // get the library loader instance

      KLibLoader *loader = KLibLoader::self();

       KCModule *module = load(mod, "kcm_%1", loader);
       if (!module)
	   module = load(mod, "libkcm_%1", loader);
       if (module)
	   return module;
    }
    else
      kdWarning() << "Module " << mod.fileName() << " doesn't specify a library!" << endl;
	
  /*
   * Ok, we could not load the library.
   * Try to run it as an executable.
   * This must not be done when calling from kcmshell, or you'll
   * have infinite recursion
   * (startService calls kcmshell which calls modloader which calls startService...)
   *
   */
  if(withfallback)
      KApplication::startServiceByDesktopPath(mod.fileName(), QString::null);
  return 0;
}

void ModuleLoader::unloadModule(const ModuleInfo &mod)
{
	// get the library loader instance
  KLibLoader *loader = KLibLoader::self();

  // try to unload the library
  QString libname("libkcm_%1");
  loader->unloadLibrary(QFile::encodeName(libname.arg(mod.library())));

  libname = "kcm_%1";
  loader->unloadLibrary(QFile::encodeName(libname.arg(mod.library())));
}
