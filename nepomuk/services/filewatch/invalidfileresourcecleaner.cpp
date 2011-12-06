/*
   Copyright (C) 2010 Vishesh Handa <handa.vish@gmail.com>
   Copyright (C) 2010 by Sebastian Trueg <trueg@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "invalidfileresourcecleaner.h"
#include "datamanagement.h"

#include <QtCore/QList>
#include <QtCore/QFile>
#include <QtCore/QTimer>

#include <Soprano/Node>
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Vocabulary/NIE>

#include <KDebug>
#include <KConfig>
#include <KConfigGroup>
#include <KUrl>
#include <KJob>


Nepomuk::InvalidFileResourceCleaner::InvalidFileResourceCleaner( QObject* parent )
    : QThread( parent ),
      m_stopped( false )
{
}


Nepomuk::InvalidFileResourceCleaner::~InvalidFileResourceCleaner()
{
    // gently terminate the thread
    m_stopped = true;
    wait();
}


void Nepomuk::InvalidFileResourceCleaner::run()
{
#ifndef NDEBUG
    kDebug() << "Searching for invalid local file entries";
    m_timer.start();
#endif

    //
    // Since the removal of the graphs could intefere with the iterator and result
    // in jumping of rows (worst case) we cache all resources to remove
    //
    QString basePathFilter;
    if(!m_basePath.isEmpty()) {
        basePathFilter = QString::fromLatin1("FILTER(REGEX(STR(?url), '^%1')) . ")
                .arg(KUrl(m_basePath).url(KUrl::AddTrailingSlash));
    }
    const QString query
            = QString::fromLatin1( "select distinct ?r ?url where { "
                                   "?r %1 ?url . %2}" )
            .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                  basePathFilter );
    Soprano::QueryResultIterator it
            = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );

    while( it.next() && !m_stopped ) {
        QUrl url( it["url"].uri() );
        QString file = url.toLocalFile();

        if( !file.isEmpty() && !QFile::exists(file) ) {
            kDebug() << "REMOVING " << file;
            m_resourcesToRemove << it["r"].uri();
        }
    }

    // call the removal method async - the slot will be called in the main thread since this QObject
    // lives in the main thread.
    QMetaObject::invokeMethod(this, "removeResources", Qt::QueuedConnection);
}

void Nepomuk::InvalidFileResourceCleaner::start(const QString &basePath)
{
    m_basePath = basePath;
    start();
}

void Nepomuk::InvalidFileResourceCleaner::removeResources()
{
    QList<QUrl> uris;
    for(int i = 0; i < 10 && !m_resourcesToRemove.isEmpty(); ++i) {
        uris << m_resourcesToRemove.takeFirst();
    }
    if(!m_stopped && !uris.isEmpty()) {
        kDebug() << uris;
        connect(Nepomuk::removeResources(uris), SIGNAL(result(KJob*)),
                this, SLOT(removeResources()));
    }
    else {
    #ifndef NDEBUG
        kDebug() << "Done searching for invalid local file entries";
        kDebug() << "Time elapsed: " << m_timer.elapsed()/1000.0 << "sec";
    #endif
        deleteLater();
        emit cleaningDone();
    }
}

#include "invalidfileresourcecleaner.moc"
