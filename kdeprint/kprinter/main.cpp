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

#include "printwrapper.h"

#include <QTimer>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocale.h>

extern "C" int KDE_EXPORT kdemain(int argc, char *argv[])
{
	KCmdLineArgs::init(argc,argv,"kprinter", 0,ki18n("KPrinter"),"0.0.1",ki18n("A printer tool for KDE" ));

	KCmdLineOptions options;
	options.add("c", ki18n("Make an internal copy of the files to print"));
	options.add("P");
	options.add("d <printer>", ki18n("Printer/destination to print on"));
	options.add("J");
	options.add("t <title>", ki18n("Title/Name for the print job" ));
	options.add("#");
	options.add("n <number>", ki18n("Number of copies"));
	options.add("o <option=value>", ki18n("Printer option" ));
	options.add("j <mode>", ki18n("Job output mode (gui, console, none)" ), "gui");
	options.add("system <printsys>", ki18n("Print system to use (lpd, cups)" ));
	options.add("stdin", ki18n("Allow printing from STDIN" ));
	options.add("nodialog", ki18n("Do not show the print dialog (print directly)"));
	options.add("+file(s)", ki18n("Files to load" ));
	KCmdLineArgs::addCmdLineOptions(options);
	KApplication	app;

	PrintWrapper	wrap;

	QTimer::singleShot(10,&wrap,SLOT(slotPrint()));

	return app.exec();
}
