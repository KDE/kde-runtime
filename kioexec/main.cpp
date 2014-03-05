/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C)  2000-2005 David Faure <faure@kde.org>
   Copyright (C)       2001 Waldo Bastian <bastian@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "main.h"

#include <QtCore/QFile>
#include <QtCore/Q_PID>

#include <QtWidgets/QApplication>
#include <kdeversion.h>
//#include <kstandarddirs.h>
#include <qdebug.h>
#include <kmessagebox.h>
#include <kaboutdata.h>
#include <kio/job.h>
#include <krun.h>
#include <kglobal.h>
#include <kio/netaccess.h>
#include <kservice.h>
#include <klocale.h>
#include <KLocalizedString>
#include <KStandardDirs>
#include <kdeversion.h>
#include <kaboutdata.h>
#include <kstartupinfo.h>
#include <kshell.h>
#include <kde_file.h>
#include <qcommandlineparser.h>
#include <qcommandlineoption.h>
#include <QStandardPaths>

static const char description[] =
        I18N_NOOP("KIO Exec - Opens remote files, watches modifications, asks for upload");


KIOExec::KIOExec(const QStringList &args, bool tempFiles, const QString &suggestedFileName)
    : mExited(false)
{
    mTempFiles = tempFiles;
    mSuggestedFileName = suggestedFileName;
    expectedCounter = 0;
    jobCounter = 0;
    command = args.first();
    qDebug() << "command=" << command;

    for ( int i = 1; i < args.count(); i++ )
    {
        QUrl url(args.value(i));
	url = KIO::NetAccess::mostLocalUrl( url, 0 );

        //kDebug() << "url=" << url.url() << " filename=" << url.fileName();
        // A local file, not an URL ?
        // => It is not encoded and not shell escaped, too.
        if ( url.isLocalFile() )
        {
            FileInfo file;
            file.path = url.toLocalFile();
            file.url = url;
            fileList.append(file);
        }
        // It is an URL
        else
        {
            if ( !url.isValid() )
                KMessageBox::error( 0L, i18n( "The URL %1\nis malformed" ,  url.url() ) );
            else if ( mTempFiles )
                KMessageBox::error( 0L, i18n( "Remote URL %1\nnot allowed with --tempfiles switch" ,  url.url() ) );
            else
            // We must fetch the file
            {
                QString fileName = KIO::encodeFileName( url.fileName() );
                if ( !suggestedFileName.isEmpty() )
                    fileName = suggestedFileName;
                // Build the destination filename, in ~/.kde/cache-*/krun/
                // Unlike KDE-1.1, we put the filename at the end so that the extension is kept
                // (Some programs rely on it)
                QString tmp = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + '/' + "krun/"  +
                              QString("%1_%2_%3").arg(getpid()).arg(jobCounter++).arg(fileName);
                FileInfo file;
                file.path = tmp;
                file.url = url;
                fileList.append(file);

                expectedCounter++;
                QUrl dest;
                dest.setPath( tmp );
                qDebug() << "Copying " << url.path() << " to " << dest;
                KIO::Job *job = KIO::file_copy( url, dest );
                jobList.append( job );

                connect( job, SIGNAL( result( KJob * ) ), SLOT( slotResult( KJob * ) ) );
            }
        }
    }

    if ( mTempFiles )
    {
        slotRunApp();
        return;
    }

    counter = 0;
    if ( counter == expectedCounter )
        slotResult( 0L );
}

void KIOExec::slotResult( KJob * job )
{
    if (job && job->error())
    {
        // That error dialog would be queued, i.e. not immediate...
        //job->showErrorDialog();
        if ( (job->error() != KIO::ERR_USER_CANCELED) )
            KMessageBox::error( 0L, job->errorString() );

        QString path = static_cast<KIO::FileCopyJob*>(job)->destUrl().path();

        QList<FileInfo>::Iterator it = fileList.begin();
        for(;it != fileList.end(); ++it)
        {
           if ((*it).path == path)
              break;
        }

        if ( it != fileList.end() )
           fileList.erase( it );
        else
           qDebug() <<  path << " not found in list";
    }

    counter++;

    if ( counter < expectedCounter )
        return;

    qDebug() << "All files downloaded, will call slotRunApp shortly";
    // We know we can run the app now - but let's finish the job properly first.
    QTimer::singleShot( 0, this, SLOT( slotRunApp() ) );

    jobList.clear();
}

void KIOExec::slotRunApp()
{
    if ( fileList.isEmpty() ) {
        qDebug() << "No files downloaded -> exiting";
        mExited = true;
        QApplication::exit(1);
        return;
    }

    KService service("dummy", command, QString());

    QList<QUrl> list;
    // Store modification times
    QList<FileInfo>::Iterator it = fileList.begin();
    for ( ; it != fileList.end() ; ++it )
    {
        KDE_struct_stat buff;
        (*it).time = KDE_stat( QFile::encodeName((*it).path), &buff ) ? 0 : buff.st_mtime;
        QUrl url;
        url.setPath((*it).path);
        list << url;
    }

    QStringList params = KRun::processDesktopExec(service, list);

    qDebug() << "EXEC " << KShell::joinArgs( params );

#ifdef Q_WS_X11
    // propagate the startup identification to the started process
    KStartupInfoId id;
    id.initId( kapp->startupId());
    id.setupStartupEnv();
#endif

    QString exe( params.takeFirst() );
    const int exit_code = QProcess::execute( exe, params );

#ifdef Q_WS_X11
    KStartupInfo::resetStartupEnv();
#endif

    qDebug() << "EXEC done";

    // Test whether one of the files changed
    it = fileList.begin();
    for( ;it != fileList.end(); ++it )
    {
        KDE_struct_stat buff;
        QString src = (*it).path;
        QUrl dest = (*it).url;
        if ( (KDE::stat( src, &buff ) == 0) &&
             ((*it).time != buff.st_mtime) )
        {
            if ( mTempFiles )
            {
                if ( KMessageBox::questionYesNo( 0L,
                                                 i18n( "The supposedly temporary file\n%1\nhas been modified.\nDo you still want to delete it?", dest.toDisplayString(QUrl::PreferLocalFile)),
                                                 i18n( "File Changed" ), KStandardGuiItem::del(), KGuiItem(i18n("Do Not Delete")) ) != KMessageBox::Yes )
                    continue; // don't delete the temp file
            }
            else if ( ! dest.isLocalFile() )  // no upload when it's already a local file
            {
                if ( KMessageBox::questionYesNo( 0L,
                                                 i18n( "The file\n%1\nhas been modified.\nDo you want to upload the changes?" , dest.toDisplayString()),
                                                 i18n( "File Changed" ), KGuiItem(i18n("Upload")), KGuiItem(i18n("Do Not Upload")) ) == KMessageBox::Yes )
                {
                    qDebug() << "src='" << src << "'  dest='" << dest << "'";
                    // Do it the synchronous way.
                    if ( !KIO::NetAccess::upload( src, dest, 0 ) )
                    {
                        KMessageBox::error( 0L, KIO::NetAccess::lastErrorString() );
                        continue; // don't delete the temp file
                    }
                }
            }
        }

        if ((!dest.isLocalFile() || mTempFiles) && exit_code == 0) {
            // Wait for a reasonable time so that even if the application forks on startup (like OOo or amarok)
            // it will have time to start up and read the file before it gets deleted. #130709.
            qDebug() << "sleeping...";
            sleep(180); // 3 mn
            qDebug() << "about to delete " << src;
            unlink( QFile::encodeName(src) );
        }
    }

    mExited = true;
    QApplication::exit(exit_code);
}

int main( int argc, char **argv )
{
    QApplication app( argc, argv);
    KAboutData aboutData( "kioexec", "kioexec", i18n("KIOExec"),
         "111.111", i18n(description), KAboutData::License_GPL,
         i18n("(c) 1998-2000,2003 The KFM/Konqueror Developers"));
    aboutData.addAuthor(i18n("David Faure"),QString(), "faure@kde.org");
    aboutData.addAuthor(i18n("Stephan Kulow"),QString(), "coolo@kde.org");
    aboutData.addAuthor(i18n("Bernhard Rosenkraenzer"),QString(), "bero@arklinux.org");
    aboutData.addAuthor(i18n("Waldo Bastian"),QString(), "bastian@kde.org");
    aboutData.addAuthor(i18n("Oswald Buddenhagen"),QString(), "ossi@kde.org");
    aboutData.setProgramIconName("kde");
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringList() << "tempfiles" , i18n("Treat URLs as local files and delete them afterwards")));
    parser.addOption(QCommandLineOption(QStringList() << "suggestedfilename <file name>" , i18n("Suggested file name for the downloaded file")));
    parser.addPositionalArgument("command", i18n("Command to execute"));
    parser.addPositionalArgument("urls", i18n("URL(s) or local file(s) used for 'command'"));

    app.setQuitOnLastWindowClosed(false);

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    if (parser.positionalArguments().count() < 1) {
        parser.showHelp(-1);
        return -1;
    }

    KIOExec exec(parser.positionalArguments(), parser.isSet(QStringLiteral("tempfiles")), QLatin1String("suggestedfilename"));

    // Don't go into the event loop if we already want to exit (#172197)
    if (exec.exited())
        return 0;

    return app.exec();
}

#include "main.moc"
