/* This file is part of the KDE project
   Copyright (C) 2004 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <kapplication.h>
#include <kio/netaccess.h>
#include <kio/job.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kdirnotify.h>
#include <kdebug.h>

int main(int argc, char *argv[])
{
    //KApplication::disableAutoDcopRegistration();
    KCmdLineArgs::init( argc, argv, "ktrash", "kio_trash",
                        ki18n( "ktrash" ),
                        KDE_VERSION_STRING ,
                        ki18n( "Helper program to handle the KDE trash can\n"
				   "Note: to move files to the trash, do not use ktrash, but \"kioclient move 'url' trash:/\"" ));

    KCmdLineOptions options;
    options.add("empty", ki18n( "Empty the contents of the trash" ));
    //{ "migrate", I18N_NOOP( "Migrate contents of old trash" ), 0 },
    options.add("restore <file>", ki18n( "Restore a trashed file to its original location" ));
    // This hack is for the servicemenu on trash.desktop which uses Exec=ktrash -empty. %f is implied...
    options.add("+[ignored]", ki18n( "Ignored" ));
    KCmdLineArgs::addCmdLineOptions( options );
    KApplication app;

    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    if ( args->isSet( "empty" ) ) {
        // We use a kio job instead of linking to TrashImpl, for a smaller binary
        // (and the possibility of a central service at some point)
        QByteArray packedArgs;
        QDataStream stream( &packedArgs, QIODevice::WriteOnly );
        stream << (int)1;
        KIO::Job* job = KIO::special( KUrl("trash:/"), packedArgs );
        (void)KIO::NetAccess::synchronousRun( job, 0 );

        // Update konq windows opened on trash:/
        org::kde::KDirNotify::emitFilesAdded(QString::fromLatin1("trash:/")); // yeah, files were removed, but we don't know which ones...
        return 0;
    }

#if 0
    // This is only for testing. KDesktop handles it automatically.
    if ( args->isSet( "migrate" ) ) {
        QByteArray packedArgs;
        QDataStream stream( packedArgs, QIODevice::WriteOnly );
        stream << (int)2;
        KIO::Job* job = KIO::special( "trash:/", packedArgs );
        (void)KIO::NetAccess::synchronousRun( job, 0 );
        return 0;
    }
#endif

    QString restoreArg = args->getOption( "restore" );
    if ( !restoreArg.isEmpty() ) {

        if (restoreArg.indexOf(QLatin1String("system:/trash"))==0) {
            restoreArg.remove(0, 13);
            restoreArg.prepend(QString::fromLatin1("trash:"));
        }

        KUrl trashURL( restoreArg );
        if ( !trashURL.isValid() || trashURL.protocol() != QLatin1String("trash") ) {
            kError() << "Invalid URL for restoring a trashed file:" << trashURL << endl;
            return 1;
        }

        QByteArray packedArgs;
        QDataStream stream( &packedArgs, QIODevice::WriteOnly );
        stream << (int)3 << trashURL;
        KIO::Job* job = KIO::special( trashURL, packedArgs );
        bool ok = KIO::NetAccess::synchronousRun( job, 0 );
        if ( !ok )
            kError() << KIO::NetAccess::lastErrorString() << endl;
        return 0;
    }

    return 0;
}
