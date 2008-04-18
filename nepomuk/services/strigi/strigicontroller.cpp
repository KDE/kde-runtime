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

    StrigiClient strigiClient;

    m_state = ShuttingDown;

    if ( isRunning() ) {
        strigiClient.stopDaemon();
    }

    if ( state() == Running ) {
        kDebug(300002) << "We started Strigi ourselves. Trying to shut it down gracefully.";
        if ( !m_strigiProcess->waitForFinished(60000) ) {
            kDebug(300002) << "strigidaemon does not terminate properly. Killing process...";
            m_strigiProcess->kill();
        }
        m_state = Idle;
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
    return QDBusConnection::sessionBus().interface()->isServiceRegistered( "vandenoever.strigi" );
}


void Nepomuk::StrigiController::slotStartStrigiIndexing()
{
    if ( isRunning() ) {
        StrigiClient strigiClient;
        strigiClient.startIndexing();
    }
}

#include "strigicontroller.moc"
