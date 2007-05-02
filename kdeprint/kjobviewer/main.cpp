/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2001 Michael Goffioul <kdeprint@swing.be>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>

#include "kjobviewer.h"
#include <klocale.h>
#include <stdlib.h>

static KCmdLineOptions options[] = {
	{ "d <printer-name>", I18N_NOOP("The printer for which jobs are requested"), 0 },
	{ "noshow", I18N_NOOP("Show job viewer at startup"), 0},
	{ "all", I18N_NOOP("Show jobs for all printers"), 0},
        KCmdLineLastOption
};


extern "C" int KDE_EXPORT kdemain(int argc, char *argv[])
{
	KAboutData	aboutData("kjobviewer",I18N_NOOP("KJobViewer"),"0.1",I18N_NOOP("A print job viewer"),KAboutData::License_GPL,"(c) 2001, Michael Goffioul", 0, "http://printing.kde.org");
	aboutData.addAuthor("Michael Goffioul",0,"kdeprint@swing.be");
	KCmdLineArgs::init(argc,argv,&aboutData);
	KCmdLineArgs::addCmdLineOptions(options);
	KJobViewerApp::addCmdLineOptions();

	if (!KJobViewerApp::start())
		exit(0);

	KJobViewerApp	a;
	return a.exec();
}
