/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>
    Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

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

#include <KUniqueApplication>
#include <KAboutData>
#include <KLocale>
#include <KCmdLineArgs>
#include <KDebug>

#include <QtDBus/QDBusConnection>

#include "backupwizard.h"
#include <QDBusConnectionInterface>

int main( int argc, char** argv )
{
    KAboutData aboutData("nepomukbackup",
                         "nepomukbackup",
                         ki18n("NepomukBackup"),
                         "1.0",
                         ki18n("Nepomuk Backup Tool"),
                         KAboutData::License_GPL,
                         ki18n("(c) 2010, Nepomuk-KDE Team"),
                         KLocalizedString(),
                         "http://nepomuk.kde.org");
    aboutData.addAuthor(ki18n("Vishesh Handa"), ki18n("Maintainer"), "handa.vish@gmail.com");
    aboutData.addAuthor(ki18n("Sebastian Trüg"),ki18n("Developer"), "trueg@kde.org");

    KCmdLineArgs::init( argc, argv, &aboutData );

    KCmdLineOptions options;
    options.add("backup", ki18n("Start the application in backup mode"));
    options.add("restore", ki18n("Start the application in restore mode"));
    KCmdLineArgs::addCmdLineOptions( options );
    KUniqueApplication::addCmdLineOptions();

    KUniqueApplication app;

    Nepomuk::BackupWizard wizard;

    if( QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.nepomuk.services.nepomukbackupsync") ) {
        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
        if( args->isSet( "backup" ) ) {
            wizard.startBackup();
        }
        else if( args->isSet("restore") ) {
            wizard.startRestore();
        }
    }
    else {
        wizard.showError(i18nc("@info", "The Nepomuk backup service does not seem to be running. Backups cannot be handled without it.") );
    }
    
    wizard.show();
    
    return app.exec();
}
