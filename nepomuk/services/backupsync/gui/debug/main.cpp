/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <KDebug>

#include <KApplication>
#include <KAboutData>
#include <KLocale>
#include <KCmdLineArgs>

#include "controlwidget.h"

int main( int argc, char** argv )
{
    KAboutData data("NepomukBackupSync", 0,
                    ki18n("NepomukBackupSync"),
                    "1.0",
                    ki18n("Nepomuk : BackupSync App for GSoC"),
                    KAboutData::License_GPL,
                    ki18n("(c) 2010"),
                    ki18n("Optional Text"),
                    "",
                    "handa.vish@gmail.com");

	KCmdLineArgs::init(argc, argv, &data);
	KApplication app;

    Nepomuk::ControlWidget widget;
    widget.show();

	return app.exec();
}
