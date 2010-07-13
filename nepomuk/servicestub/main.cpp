/* This file is part of the KDE Project
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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


#include <KComponentData>
#include <KCmdLineArgs>
#include <KAboutData>
#include <KService>
#include <KServiceTypeTrader>
#include <KDebug>
#include <KApplication>

#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

#include <signal.h>
#include <stdio.h>

#include "servicecontrol.h"

namespace {
#ifndef Q_OS_WIN
    void signalHandler( int signal )
    {
        switch( signal ) {
        case SIGHUP:
        case SIGQUIT:
        case SIGINT:
            QCoreApplication::exit( 0 );
        }
    }
#endif

    void installSignalHandler() {
#ifndef Q_OS_WIN
        struct sigaction sa;
        ::memset( &sa, 0, sizeof( sa ) );
        sa.sa_handler = signalHandler;
        sigaction( SIGHUP, &sa, 0 );
        sigaction( SIGINT, &sa, 0 );
        sigaction( SIGQUIT, &sa, 0 );
#endif
    }
}


int main( int argc, char** argv )
{
    KAboutData aboutData( "nepomukservicestub", "nepomuk",
                          ki18n("Nepomuk Service Stub"),
                          "0.2",
                          ki18n("Nepomuk Service Stub"),
                          KAboutData::License_GPL,
                          ki18n("(c) 2008, Sebastian Trüg"),
                          KLocalizedString(),
                          "http://nepomuk.kde.org" );
    aboutData.setProgramIconName( "nepomuk" );
    aboutData.addAuthor(ki18n("Sebastian Trüg"),ki18n("Maintainer"), "trueg@kde.org");

    KCmdLineOptions options;
    options.add("+servicename", ki18nc("@info:shell", "Service to start"));
    KCmdLineArgs::addCmdLineOptions( options );

    KCmdLineArgs::init( argc, argv, &aboutData );

    // FIXME: set the proper KConfig rc name using the service name

    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

    if( args->count() != 1 ) {
        // create dummy componentData for the locale
        KComponentData dummy( "___nepomukdummyservicename___" );
        KCmdLineArgs::usageError( i18n("No service name specified") );
    }

    QTextStream s( stderr );

    QString serviceName = args->arg(0);
    args->clear();

    aboutData.setAppName( serviceName.toLocal8Bit() );
    KApplication app( true /* The strigi service actually needs a GUI at the moment */ );
    app.disableSessionManagement();
    installSignalHandler();
    QApplication::setQuitOnLastWindowClosed( false );


    // check if NepomukServer is running
    // ====================================
//     if( !QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.kde.NepomukServer" ) ) {
//         s << "Nepomuk server not running." << endl;
//         return ErrorMissingDependency;
//     }


    // search the service
    // ====================================
    KService::List services = KServiceTypeTrader::self()->query( "NepomukService", "DesktopEntryName == '" + serviceName + '\'' );
    if( services.isEmpty() ) {
        s << i18n( "Unknown service name:") << " " <<  serviceName << endl;
        return Nepomuk::ServiceControl::ErrorUnknownServiceName;
    }
    KService::Ptr service = services.first();


    // Check if this service is already running
    // ====================================
    if( QDBusConnection::sessionBus().interface()->isServiceRegistered( Nepomuk::ServiceControl::dbusServiceName( serviceName ) ) ) {
        s << "Service " << serviceName << " already running." << endl;
        return Nepomuk::ServiceControl::ErrorServiceAlreadyRunning;
    }


    // Check the service dependencies
    // ====================================
    QStringList dependencies = service->property( "X-KDE-Nepomuk-dependencies", QVariant::StringList ).toStringList();
    foreach( const QString &dep, dependencies ) {
        if( !QDBusConnection::sessionBus().interface()->isServiceRegistered( Nepomuk::ServiceControl::dbusServiceName( dep ) ) ) {
            s << "Missing dependency " << dep << endl;
            return Nepomuk::ServiceControl::ErrorMissingDependency;
        }
    }


    // register the service control
    // ====================================
    Nepomuk::ServiceControl* control = new Nepomuk::ServiceControl( serviceName, service, &app );


    // start the service (queued since we need an event loop)
    // ====================================
    QTimer::singleShot( 0, control, SLOT( start() ) );

    return app.exec();
}
