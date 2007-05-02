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

static KCmdLineOptions options[] =
{
	{ "c",               I18N_NOOP("Make an internal copy of the files to print"), 0},
	{ "P", 0, 0 },
	{ "d <printer>",     I18N_NOOP("Printer/destination to print on"),     0},
	{ "J", 0, 0 },
	{ "t <title>",       I18N_NOOP("Title/Name for the print job" ),             0},
	{ "#", 0, 0 },
	{ "n <number>",      I18N_NOOP("Number of copies"), 0 },
	{ "o <option=value>", I18N_NOOP("Printer option" ),                      0},
	{ "j <mode>",        I18N_NOOP("Job output mode (gui, console, none)" ), "gui"},
	{ "system <printsys>",I18N_NOOP("Print system to use (lpd, cups)" ), 0},
	{ "stdin",           I18N_NOOP("Allow printing from STDIN" ),        0},
	{ "nodialog",        I18N_NOOP("Do not show the print dialog (print directly)"), 0},
	{ "+file(s)",	      I18N_NOOP("Files to load" ),                       0},
	KCmdLineLastOption
};

extern "C" int KDE_EXPORT kdemain(int argc, char *argv[])
{
	KCmdLineArgs::init(argc,argv,"kprinter",I18N_NOOP("KPrinter"),I18N_NOOP("A printer tool for KDE" ),"0.0.1");
	KCmdLineArgs::addCmdLineOptions(options);
	KApplication	app;

	PrintWrapper	wrap;
  wrap.show();

	QTimer::singleShot(10,&wrap,SLOT(slotPrint()));

	return app.exec();
}
