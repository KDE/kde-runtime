/*****************************************************************
 * drkonki - The KDE Crash Handler
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#include <kapp.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

#include "toplevel.h"

static const char *version = "0.01";
static const char *description = I18N_NOOP( "A test implementation of the KDE Crash Handler." );

static const KCmdLineOptions options[] =
{
  {"signal <number>", I18N_NOOP("The signal number."), 0},
  {"appname <name>",  I18N_NOOP("Name of the application which crashed."), 0},
  { 0, 0, 0 }
};

int main( int argc, char* argv[] )
{
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
  QCString signal = args->getOption( "signal" );
  int signalnum = signal.toInt();
  QCString appname = args->getOption( "appname" );

  Toplevel *w = new Toplevel(signalnum, appname);

  return w->exec();
}

