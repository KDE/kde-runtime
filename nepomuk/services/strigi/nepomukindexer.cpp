/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-2011 Sebastian Trueg <trueg@kde.org>

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

#include <Nepomuk/Resource>

#include <KUrl>
#include <KDebug>
#include <KProcess>
#include <KStandardDirs>

#include <QtCore/QFileInfo>
#include <QtCore/QTimer>


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

void Nepomuk::Indexer::start()
{
    // setup the external process which does the actual indexing
    const QString exe = KStandardDirs::findExe(QLatin1String("nepomukindexer"));
    m_process = new KProcess( this );
    m_process->setProgram( exe, QStringList() << m_url.toLocalFile() );

    // start the process
    kDebug() << "Running" << exe << m_url.toLocalFile();
    connect( m_process, SIGNAL(finished(int)), this, SLOT(slotIndexedFile(int)) );
    m_process->start();

    // start the timer which will kill the process if it does not terminate after 5 minutes
    m_processTimer->start(5*60*1000);
}


void Nepomuk::Indexer::slotIndexedFile(int exitCode)
{
    // stop the timer since there is no need to kill the process anymore
    m_processTimer->stop();

    kDebug() << "Indexing of " << m_url.toLocalFile() << "finished with exit code" << exitCode;
    m_exitCode = exitCode;

    emitResult();
}

void Nepomuk::Indexer::slotProcessTimerTimeout()
{
    kDebug() << "Killing the indexer process which seems stuck for" << m_url;
    m_process->disconnect(this);
    m_process->kill();
    m_process->waitForFinished();
    emitResult();
}

#include "nepomukindexer.moc"
