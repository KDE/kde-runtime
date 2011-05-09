/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "systray.h"

#include <KUniqueApplication>
#include <KCmdLineArgs>
#include <KAboutData>

int main( int argc, char *argv[] )
{
    KAboutData aboutData( "nepomukcontroller",
                          "nepomukcontroller",
                          ki18n("Nepomuk Controller"),
                          "0.2",
                          ki18n("A small tool to monitor and control Nepomuk file indexing"),
                          KAboutData::License_GPL,
                          ki18n("(c) 2008-2011, Sebastian Trüg"),
                          KLocalizedString(),
                          "http://nepomuk.kde.org" );
    aboutData.addAuthor(ki18n("Sebastian Trüg"),ki18n("Maintainer"), "trueg@kde.org");
    aboutData.setProgramIconName( "nepomuk" );

    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineOptions options;
    KCmdLineArgs::addCmdLineOptions( options );

    if( KUniqueApplication::start() ) {
        KUniqueApplication app;
        // trueg: Hack: we need to pass a 0 parent since otherwise the app would crash on exit due to some double deletion in K/QMenu
        (void)new Nepomuk::SystemTray( 0 /* &app */ );
        return app.exec();
    }
}
