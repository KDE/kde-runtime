/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-2011 Sebastian Trueg <trueg@kde.org>
   Copyright (C) 2012 Ivan Cukic <ivan.cukic@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "nepomukindexer.h"
#include "util.h"

#include <KUrl>
#include <KDebug>
#include <KProcess>
#include <KStandardDirs>

#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

KProcess * Nepomuk::Indexer::s_process = NULL;

Nepomuk::Indexer::Indexer(const QFileInfo& info, QObject* parent)
    : KJob(parent),
      m_url( info.absoluteFilePath() ),
      m_exitCode( -1 )
{
    // setup the timer used to kill the indexer process if it seems to get stuck
    m_processTimer = new QTimer(this);
    m_processTimer->setSingleShot(true);
    connect(m_processTimer, SIGNAL(timeout()),
            this, SLOT(slotProcessTimerTimeout()));
}

Nepomuk::Indexer::~Indexer()
{
    if( s_process ) {
        s_process->disconnect(this);
    }
}

void Nepomuk::Indexer::start()
{
    if( s_process && s_process->state() == QProcess::NotRunning ) {
        s_process->deleteLater();
        s_process = NULL;
    }

    if ( !s_process ) {
        // setup the external process which does the actual indexing
        const QString exe = KStandardDirs::findExe(QLatin1String("nepomukindexer"));

        s_process = new KProcess();
        s_process->setOutputChannelMode(KProcess::OnlyStdoutChannel);
        s_process->setProgram( exe, QStringList() << "--file-list-from-stdin" );

        // start the process
        kDebug() << "Running" << exe;

        s_process->start();
    }

    connect( s_process, SIGNAL(finished(int)), this, SLOT(slotProcessFinished(int)) );
    connect( s_process, SIGNAL(readyReadStandardOutput()), this, SLOT(slotProcessReadyRead()) );

    kDebug() << "Passing" << m_url.toLocalFile() << "to the indexer";

    QString fileName = m_url.toLocalFile() + '\n';
    s_process->write(fileName.toUtf8());
    s_process->waitForBytesWritten();

    // start the timer which will kill the process if it does not terminate after 5 minutes
    m_processTimer->start(5*60*1000);
}


void Nepomuk::Indexer::slotProcessFinished(int exitCode)
{
    m_exitCode = exitCode;

    slotFileIndexed();
}

void Nepomuk::Indexer::slotProcessReadyRead()
{
    QString result = QString::fromUtf8(s_process->readLine().trimmed());

    s_process->readAll(); // ignore

    kDebug() << "This is a result for" << m_url.toLocalFile() << result;

    m_exitCode = result.toInt();

    slotFileIndexed();
}

void Nepomuk::Indexer::slotFileIndexed()
{
    // stop the timer since there is no need to kill the process anymore
    m_processTimer->stop();

    kDebug() << "Indexing of " << m_url.toLocalFile() << "finished with exit code" << m_exitCode;

    emitResult();
}

void Nepomuk::Indexer::slotProcessTimerTimeout()
{
    kDebug() << "Killing the indexer process which seems stuck for" << m_url;

    s_process->kill();
    s_process->waitForFinished();

    emitResult();
}

#include "nepomukindexer.moc"
