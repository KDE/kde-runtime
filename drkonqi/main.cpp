/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * $Id$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#include <config.h>

#include <stdlib.h>

#include <kapp.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

#include "krashconf.h"
#include "toplevel.h"

static const char *version = "1.0";
static const char *description = I18N_NOOP( "KDE Crash Handler give the user feedback if a program crashed." );

static const KCmdLineOptions options[] =
{
  {"signal <number>", I18N_NOOP("The signal number we caught."), 0},
  {"appname <name>",  I18N_NOOP("Name of the program."), 0},
  {"appversion <version>", I18N_NOOP("The version of the program."), 0},
  {"bugaddress <address>", I18N_NOOP("The bug address to use."), 0},
  {"programname <name>", I18N_NOOP("Translated name of the program."), 0},
  {"pid <pid>", I18N_NOOP("The PID of the program"), 0},
  {"kdeinit", I18N_NOOP("The program was started by kdeinit"), 0},
  { 0, 0, 0 }
};

int main( int argc, char* argv[] )
{
  // Make sure that DrKonqi doesn't start DrKonqi when it crashes :-]
  setenv("KDE_DEBUG", "true", 1); 

  KAboutData aboutData( "drkonqi", 
			I18N_NOOP("The KDE Crash Handler"),
			version,
			description,
			KAboutData::License_BSD,
			"(C) 2000, Hans Petter Bieker");
  aboutData.addAuthor("Hans Petter Bieker", 0, "bieker@kde.org");

  KCmdLineArgs::init(argc, argv, &aboutData);
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication a;

  KrashConfig krashconf;

  Toplevel w(&krashconf);

  return w.exec();
}
