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

#include <KDebug>
#include <KProcess>
#include <KStandardDirs>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


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


bool Nepomuk::StrigiController::start( bool withNepomuk )
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
    if ( withNepomuk ) {
        // use our soprano backend
        *m_strigiProcess << "-t" << "sopranobackend";
    }
    else {
        // use default backend
        *m_strigiProcess << "-t" << "clucene";
    }

    if ( m_strigiProcess->state() == QProcess::NotRunning ) {
        m_running5Minutes = false;
        m_state = StartingUp;
        m_strigiProcess->start();
        if ( m_strigiProcess->waitForStarted() ) {
            m_state = Running;
            QTimer::singleShot( 50000, this, SLOT( slotRunning5Minutes() ) );
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
        m_state = ShuttingDown;
        m_strigiProcess->terminate();
        if ( !m_strigiProcess->waitForFinished() ) {
            kDebug(300002) << "strigidaemon does not terminate properly. Killing process...";
            m_strigiProcess->kill();
        }
        m_state = Idle;
    }
}


void Nepomuk::StrigiController::slotProcessFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    if ( m_state != ShuttingDown ) {
        kDebug(300002) << "strigidaemon shut down unexpectedly.";
        if ( exitStatus == QProcess::CrashExit ) {
            kDebug(300002) << "strigidaemon crashed.";
            if ( m_running5Minutes ) {
                kDebug(300002) << "restarting strigidaemon...";
                start();
            }
            else {
                kDebug(300002) << "looks like a recurring crash!";
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
    kDebug(300002);

#ifndef _WIN32
    int fd = open( QFile::encodeName( QString( "%1/.strigi/lock" ).arg( QDir::homePath() ) ), O_WRONLY );
    if ( fd == -1 ) {
        kDebug(300002) << "failed to open lock";
        return false;
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
        return false;
    }
    close( fd );
    return lock.l_type == F_WRLCK;
#else
    // FIXME: this will actually start strigidaemon through DBus autostarting
//     return( QDBusConnection::sessionBus().call( QDBusMessage::createMethodCall( "vandenoever.strigi",
//                                                                                 "/search",
//                                                                                 "vandenoever.strigi",
//                                                                                 "getBackEnds" ) ).type()
//             != QDBusMessage::ErrorMessage );
    return false;
#endif /* _WIN32 */
}

#include "strigicontroller.moc"
