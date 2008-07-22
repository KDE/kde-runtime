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
#include <Nepomuk/Service>

#include <QtCore/QTextStream>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

#include <signal.h>
#include <stdio.h>

#include "servicecontrol.h"
#include "servicecontroladaptor.h"

namespace {
    QString dbusServiceName( const QString& serviceName ) {
        return QString("org.kde.nepomuk.services.%1").arg(serviceName);
    }

    enum Errors {
        ErrorUnknownServiceName = -9,
        ErrorServiceAlreadyRunning = -10,
        ErrorFailedToStart = -11,
        ErrorMissingDependency = -12
    };

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
                          "0.1",
                          ki18n("Nepomuk Service Stub"),
                          KAboutData::License_GPL,
                          ki18n("(c) 2008, Sebastian Trüg"),
                          KLocalizedString(),
                          "http://nepomuk.kde.org" );
    aboutData.addAuthor(ki18n("Sebastian Trüg"),ki18n("Maintainer"), "trueg@kde.org");

    KCmdLineOptions options;
    options.add("+servicename", ki18nc("@info:shell", "Service to start"));
    KCmdLineArgs::addCmdLineOptions( options );

    KCmdLineArgs::init( argc, argv, &aboutData );

    KApplication app;
    installSignalHandler();
    QApplication::setQuitOnLastWindowClosed( false );

    // FIXME: set the proper KConfig rc name using the service name

    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

    if( args->count() != 1 ) {
        KCmdLineArgs::usageError( i18n("No service name specified") );
    }

    QTextStream s( stderr );


    // check if NepomukServer is running
    // ====================================
//     if( !QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.kde.NepomukServer" ) ) {
//         s << "Nepomuk server not running." << endl;
//         return ErrorMissingDependency;
//     }


    // search the service
    // ====================================
    QString serviceName = args->arg(0);
    KService::List services = KServiceTypeTrader::self()->query( "NepomukService", "DesktopEntryName == '" + serviceName + "'" );
    if( services.isEmpty() ) {
        s << i18n( "Unknown service name:") << " " <<  serviceName << endl;
        return ErrorUnknownServiceName;
    }
    KService::Ptr service = services.first();


    // Check if this service is already running
    // ====================================
    if( QDBusConnection::sessionBus().interface()->isServiceRegistered( dbusServiceName( serviceName ) ) ) {
        s << "Service " << serviceName << " already running." << endl;
        return ErrorServiceAlreadyRunning;
    }


    // Check the service dependencies
    // ====================================
    QStringList dependencies = service->property( "X-KDE-Nepomuk-dependencies", QVariant::StringList ).toStringList();
    foreach( const QString &dep, dependencies ) {
        if( !QDBusConnection::sessionBus().interface()->isServiceRegistered( dbusServiceName( dep ) ) ) {
            s << "Missing dependency " << dep << endl;
            return ErrorMissingDependency;
        }
    }


    // register the service control
    // ====================================
    Nepomuk::ServiceControl* control = new Nepomuk::ServiceControl( &app );
    (void)new ServiceControlAdaptor( control );
    if( !QDBusConnection::sessionBus().registerObject( "/servicecontrol", control ) ) {
        s << "Failed to register dbus service " << dbusServiceName( serviceName ) << "." << endl;
        return ErrorFailedToStart;
    }

    // start the service
    // ====================================
    Nepomuk::Service* module = service->createInstance<Nepomuk::Service>( control );
    if( !module ) {
        s << "Failed to start service " << serviceName << "." << endl;
        return ErrorFailedToStart;
    }

    // register the service interface
    // ====================================
    if( !QDBusConnection::sessionBus().registerService( dbusServiceName( serviceName ) ) ) {
        s << "Failed to register dbus service " << dbusServiceName( serviceName ) << "." << endl;
        return ErrorFailedToStart;
    }

    QDBusConnection::sessionBus().registerObject( '/' + serviceName,
                                                  module,
                                                  QDBusConnection::ExportScriptableSlots |
                                                  QDBusConnection::ExportScriptableProperties |
                                                  QDBusConnection::ExportAdaptors);

    return app.exec();
}
