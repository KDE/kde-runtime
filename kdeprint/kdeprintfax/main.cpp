/*
 *   kdeprintfax - a interface to fax-packages
 *   Copyright (C) 2001  Michael Goffioul
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "kdeprintfax.h"

#include <QFile>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kapplication.h>

QString debugFlag;
int oneShotFlag = false;

static const char description[] =
	I18N_NOOP("A small fax utility to be used with kdeprint.");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE


int main(int argc, char *argv[])
{

  KAboutData aboutData( "kdeprintfax", 0, ki18n("KdeprintFax"),
    "1.0", ki18n(description), KAboutData::License_GPL,
    ki18n("(c), 2001 Michael Goffioul"), KLocalizedString(), "http://printing.kde.org");
  aboutData.addAuthor(ki18n("Michael Goffioul"),KLocalizedString(), "kdeprint@swing.be");
  KCmdLineArgs::init( argc, argv, &aboutData );

  KCmdLineOptions options;
  options.add("phone ", ki18n("Phone number to fax to"));
  options.add("immediate", ki18n("Send fax immediately"));
  options.add("batch", ki18n("Exit after sending"));
  options.add("+[file]", ki18n("File to fax (added to the file list)"));
  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
  KCmdLineArgs::addStdCmdLineOptions();

  KApplication a;
  KCmdLineArgs	*args = KCmdLineArgs::parsedArgs();

  KdeprintFax	*w = new KdeprintFax;
  w->show();
  for (int i=0;i<args->count();i++)
  	w->addURL(args->url(i));

	QString phone = args->getOption( "phone" );
	if( !phone.isEmpty() ) {
		w->setPhone( phone );
	}

	if( args->isSet( "immediate" ) ) {
		w->sendFax( args->isSet( "batch" ) );
	}

  args->clear();
  return a.exec();
}
