/*****************************************************************
 * drkonki - The KDE Crash Handler
 *
 * $Id$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include <kapp.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

#include "krashconf.h"
#include "toplevel.h"

static const char *version = "0.1";
static const char *description = I18N_NOOP( "KDE Crash Handler give the user feedback if a program crashed." );

static const KCmdLineOptions options[] =
{
  {"signal <number>", I18N_NOOP("The signal number we caught."), "11"},
  {"appname <name>",  I18N_NOOP("Name of the program."), 0},
  {"appversion <version>", I18N_NOOP("The version of the program."), 0},
  {"bugaddress <address>", I18N_NOOP("The bug address to use."), 0},
  {"programname <name>", I18N_NOOP("Translated name of the program."), 0},
  {"pid <pid>", I18N_NOOP("The PID of the program"), 0},
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

  // parse command line arguments
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  int signalnum = args->getOption( "signal" ).toInt();
  int pid = args->getOption( "pid" ).toInt();

  KAboutData oldabout(args->getOption("appname"),
		    args->getOption("programname"),
		    args->getOption("appversion"),
		    0,
		    0,
		    0,
		    0,
		    0,
		    args->getOption("bugaddress"));

  KrashConfig krashconf(&oldabout, signalnum, pid);

  Toplevel *w = new Toplevel(&krashconf);

  int i = w->exec();
  delete w;
  return i;
}

