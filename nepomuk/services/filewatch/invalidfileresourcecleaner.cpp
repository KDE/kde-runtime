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


Nepomuk::InvalidFileResourceCleaner::InvalidFileResourceCleaner( QObject* parent )
    : QThread( parent ),
      m_stopped( false )
{
    connect( this, SIGNAL(finished()), this, SLOT(deleteLater()) );
}


Nepomuk::InvalidFileResourceCleaner::~InvalidFileResourceCleaner()
{
    // gently terminate the thread
    m_stopped = true;
    wait();
}


void Nepomuk::InvalidFileResourceCleaner::run()
{
    kDebug() << "Searching for invalid local file entries";
#ifndef NDEBUG
    QTime timer;
    timer.start();
#endif
    //
    // Since the removal of the graphs could intefere with the iterator and result
    // in jumping of rows (worst case) we cache all resources to remove
    //
    QList<Soprano::Node> resourcesToRemove;
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
            resourcesToRemove << it["r"];
        }
    }

    Q_FOREACH( const Soprano::Node& r, resourcesToRemove ) {
        if ( m_stopped )
            break;
        // TODO: use DMS here instead of Soprano
        Nepomuk::ResourceManager::instance()->mainModel()->removeAllStatements( r, Soprano::Node(), Soprano::Node() );
        Nepomuk::ResourceManager::instance()->mainModel()->removeAllStatements( Soprano::Node(), Soprano::Node(), r );
    }
    kDebug() << "Done searching for invalid local file entries";
#ifndef NDEBUG
    kDebug() << "Time elapsed: " << timer.elapsed()/1000.0 << "sec";
#endif
}

void Nepomuk::InvalidFileResourceCleaner::start(const QString &basePath)
{
    m_basePath = basePath;
    start();
}

#include "invalidfileresourcecleaner.moc"
