/* This file is part of the KDE Project
   Copyright (c) 2007 Sebastian Trueg <trueg@kde.org>

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

#include "strigicontroller.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtCore/QTimer>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include <strigi/qtdbus/strigiclient.h>

#include <KDebug>
#include <KProcess>
#include <KStandardDirs>
#include <KMessageBox>
#include <KLocale>

#ifndef _WIN32

#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

namespace {
    pid_t strigidaemonPid() {
        int fd = open( QFile::encodeName( QString( "%1/.strigi/lock" ).arg( QDir::homePath() ) ), O_WRONLY );
        if ( fd == -1 ) {
            kDebug(300002) << "failed to open lock";
            return ( pid_t )-1;
        }
        struct flock lock;
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = 0;
        int r = fcntl( fd, F_GETLK, &lock );
        if ( r == -1 ) {
            kDebug(300002) << "failed to configure lock";
            close(fd);
            return ( pid_t )-1;
        }
        close( fd );
        kDebug(300002) << (lock.l_type == F_WRLCK);
        if ( lock.l_type == F_WRLCK ) {
            return lock.l_pid;
        }
        else {
            return ( pid_t )-1;
        }
    }

    // FIXME: what if the pid is reused during the time of this calling?
    bool waitForProcessToExit( pid_t pid, int timeout = 2 ) {
        time_t startTime = time(0);
        while ( time( 0 ) - startTime < timeout ) {
            if ( kill( pid, 0 ) < 0 ) {
                return true;
            }
        }
        return false;
    }
}
#endif // ifndef _WIN32


Nepomuk::StrigiController::StrigiController( QObject* parent )
    : QObject( parent ),
      m_strigiProcess( 0 ),
      m_running5Minutes( false ),
      m_state( Idle )
{
}


Nepomuk::StrigiController::~StrigiController()
{
    shutdown();
}


Nepomuk::StrigiController::State Nepomuk::StrigiController::state() const
{
    return m_state;
}


bool Nepomuk::StrigiController::start()
{
    kDebug(300002) << "(Nepomuk::StrigiController::start)";
    if ( !m_strigiProcess ) {
        m_strigiProcess = new KProcess( this );
        m_strigiProcess->setOutputChannelMode( KProcess::ForwardedChannels );
        connect( m_strigiProcess, SIGNAL( finished( int, QProcess::ExitStatus) ),
                 this, SLOT( slotProcessFinished( int, QProcess::ExitStatus) ) );
    }

    m_strigiProcess->clearProgram();
    *m_strigiProcess << KStandardDirs::findExe( "strigidaemon" );

    if ( m_strigiProcess->state() == QProcess::NotRunning ) {
        m_running5Minutes = false;
        m_state = StartingUp;
        m_strigiProcess->start();
        if ( m_strigiProcess->waitForStarted() ) {
            m_state = Running;
            QTimer::singleShot( 50000, this, SLOT( slotRunning5Minutes() ) );

            kDebug(300002) << "Strigi started successfully.";

            // Strigi might refuse to start properly for some reason (invalid config, lock file invalid, whatever.)
            // In that case the dbus client would hang. Thus, we wait for 5 seconds before starting the indexing
            // (Which is ugly anyway since Strigi should do that automatically)
            QTimer::singleShot( 5000, this, SLOT( slotStartStrigiIndexing() ) );

            return true;
        }
        else {
            kDebug(300002) << "Failed to start strigidaemon.";
            m_state = Idle;
            return false;
        }
    }
    else {
        kDebug(300002) << "strigidaemon already running.";
        return false;
    }
}


void Nepomuk::StrigiController::shutdown()
{
    kDebug(300002) << "(Nepomuk::StrigiController::shutdown)";
    if ( state() == Running ) {
        kDebug(300002) << "We started Strigi ourselves. Trying to shut it down gracefully.";
        m_state = ShuttingDown;
        m_strigiProcess->terminate();
        if ( !m_strigiProcess->waitForFinished() ) {
            kDebug(300002) << "strigidaemon does not terminate properly. Killing process...";
            m_strigiProcess->kill();
        }
        m_state = Idle;
    }
    else {
        // let's see if Strigi has been started by someone else
#ifndef _WIN32
        pid_t pid = strigidaemonPid();
        if ( pid != ( pid_t )-1 ) {
            kDebug(300002) << "Shutting down Strigi instance started by someone else.";

            m_state = ShuttingDown;

            // give Strigi the possibility to shutdown gracefully
            kill( pid, SIGQUIT );

            // wait for the Strigi process to exit
            if ( !waitForProcessToExit( pid ) ) {
                // kill it the hard way if it won't stop (Strigi tends to ignore the quit signal during indexing)
                kDebug(300002) << "strigidaemon does not terminate properly. Killing process...";
                kill( pid, SIGKILL );
            }

            m_state = Idle;
        }
#else
#warning FIXME: No Strigi status support on Windows
#endif
    }
}


void Nepomuk::StrigiController::slotProcessFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    if ( m_state == Running ) {
        kDebug(300002) << "strigidaemon shut down unexpectedly with exit code:" << exitCode;

        m_state = Idle;

        if ( exitStatus == QProcess::CrashExit ) {
            kDebug(300002) << "strigidaemon crashed.";
            if ( m_running5Minutes ) {
                kDebug(300002) << "restarting strigidaemon...";
                start();
            }
            else {
                kDebug(300002) << "looks like a recurring crash!";
                KMessageBox::error( 0,
                                    i18n( "Strigi (the desktop file indexer) crashed repeatedly. It will not be started again." ),
                                    i18n( "Strigi Desktop Search" ) );
            }
        }
    }
}


void Nepomuk::StrigiController::slotRunning5Minutes()
{
    m_running5Minutes = true;
}


bool Nepomuk::StrigiController::isRunning()
{
#ifndef _WIN32
    return(  strigidaemonPid() != ( pid_t )-1 );
#else
#warning FIXME: No Strigi status support on Windows
    // FIXME: this will actually start strigidaemon through DBus autostarting
//     return( QDBusConnection::sessionBus().call( QDBusMessage::createMethodCall( "vandenoever.strigi",
//                                                                                 "/search",
//                                                                                 "vandenoever.strigi",
//                                                                                 "getBackEnds" ) ).type()
//             != QDBusMessage::ErrorMessage );
    return false;
#endif /* _WIN32 */
}


void Nepomuk::StrigiController::slotStartStrigiIndexing()
{
    if ( isRunning() ) {
        StrigiClient strigiClient;
        strigiClient.startIndexing();
    }
}

#include "strigicontroller.moc"
