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

#include <qfile.h>

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
  QCString signal = args->getOption( "signal" );
  int signalnum = signal.toInt();
  if (signalnum < 0) signalnum = 11;
  QString appname = QFile::decodeName(args->getOption( "appname" ));

  Toplevel *w = new Toplevel(signalnum, appname);

  return w->exec();
}

