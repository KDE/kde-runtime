/* This file is part of the KDE Project
   Copyright (c) 2007-2010 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "nepomukserver.h"
#include "nepomukserver_export.h"

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kcomponentdata.h>
#include <kdebug.h>

#include <QtCore/QCoreApplication>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

#include <signal.h>


namespace {
    Nepomuk::Server* s_server = 0;

#ifndef Q_OS_WIN
    void signalHandler( int signal )
    {
        switch( signal ) {
        case SIGHUP:
        case SIGQUIT:
        case SIGTERM:
        case SIGINT:
            if ( s_server ) {
                s_server->quit();
            }
        }
    }

    void installSignalHandler() {
        struct sigaction sa;
        ::memset( &sa, 0, sizeof( sa ) );
        sa.sa_handler = signalHandler;
        sigaction( SIGHUP, &sa, 0 );
        sigaction( SIGINT, &sa, 0 );
        sigaction( SIGQUIT, &sa, 0 );
        sigaction( SIGTERM, &sa, 0 );
    }
#endif
}


extern "C" NEPOMUK_SERVER_EXPORT int kdemain( int argc, char** argv )
{
    KAboutData aboutData( "NepomukServer", "nepomukserver",
                          ki18n("Nepomuk Server"),
                          "0.2",
                          ki18n("Nepomuk Server - Manages Nepomuk storage and services"),
                          KAboutData::License_GPL,
                          ki18n("(c) 2008-2011, Sebastian Trüg"),
                          KLocalizedString(),
                          "http://nepomuk.kde.org" );
    aboutData.addAuthor(ki18n("Sebastian Trüg"),ki18n("Maintainer"), "trueg@kde.org");

    KCmdLineArgs::init( argc, argv, &aboutData );
    KComponentData componentData( &aboutData );

    if ( QDBusConnection::sessionBus().interface()->isServiceRegistered(QLatin1String("org.kde.NepomukServer")) ) {
        fprintf( stderr, "Nepomuk server already running.\n" );
        return 0;
    }

#ifndef Q_OS_WIN
    installSignalHandler();
#endif

    QCoreApplication app(argc, argv);
    s_server = new Nepomuk::Server(&app);
    return app.exec();
}
