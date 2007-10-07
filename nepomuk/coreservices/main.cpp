/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <KUniqueApplication>
#include <KDebug>
#include <KLocale>
#include <KCmdLineArgs>
#include <KAboutData>

#include "coreservices.h"

#include <signal.h>

static const char* version = "0.91";
static const char* description = I18N_NOOP( "Nepomuk data Repository" );

namespace {
    void signalHandler( int signal )
    {
        switch( signal ) {
        case SIGHUP:
        case SIGINT:
        case SIGQUIT:
            QCoreApplication::exit( 1 );
        }
    }
}

int main( int argc, char** argv )
{
    KAboutData aboutData( "DataRepository",
                          0,
                          ki18n("Nepomuk Data Repository"),
                          version,
                          ki18n(description),
                          KAboutData::License_GPL,
                          ki18n("(C) 2007, Sebastian Trüg"));
    aboutData.addAuthor(ki18n("Sebastian Trüg"), KLocalizedString(), "trueg@kde.org");
    aboutData.addAuthor(ki18n("Daniele Galdi"), KLocalizedString(), "daniele.galdi@gmail.com" );

    aboutData.setOrganizationDomain( "nepomuk.kde.org" );

    KCmdLineArgs::init( argc, argv, &aboutData );

    KCmdLineOptions options;
    KCmdLineArgs::addCmdLineOptions( options );

    KUniqueApplication app( false ); // no need for a GUI
#ifndef Q_OS_WIN
    struct sigaction sa;
    ::memset( &sa, 0, sizeof( sa ) );
    sa.sa_handler = signalHandler;
    sigaction( SIGHUP, &sa, 0 );
    sigaction( SIGINT, &sa, 0 );
    sigaction( SIGQUIT, &sa, 0 );
    sigaction( SIGKILL, &sa, 0 );
#endif
    Nepomuk::CoreServices::DaemonImpl instance( &app );
    if( instance.registerServices() ) {
        return app.exec();
    }
    else {
        kDebug(300002) << "Failed to setup core services.";
        return -1;
    }
}
