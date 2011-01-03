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

namespace {

    /* BUG: This is a workaround to "Removable Storage" or "Network Storage" bug, where the metadata of all
     * the files in the NAS would be deleted if the NAS was unmounted and Nepomuk was restarted.
     *
     * FIXME: This is NOT a permanent fix and should be removed in 4.7
     */ 
    QString deletionBlacklistFilter() {
        KConfig config("nepomukserverrc");
        QStringList directories = config.group("Service-nepomukfilewatch").readPathEntry("Deletion Blacklist", QStringList());
        
        QString filter;
        Q_FOREACH( const QString & dir, directories ) {
            filter += QString::fromLatin1( " && !regex(str(?url), '^%1') " )
                      .arg( KUrl(dir).url( KUrl::AddTrailingSlash ) );
        }
        return filter;
    }
}

void Nepomuk::InvalidFileResourceCleaner::run()
{
    kDebug() << "Searching for invalid local file entries";
    //
    // Since the removal of the graphs could intefere with the iterator and result
    // in jumping of rows (worst case) we cache all graphs to remove
    //
    QList<Soprano::Node> resourcesToRemove;
    QString query = QString::fromLatin1( "select distinct ?r ?url where { "
                                         "?r %1 ?url. "
                                         "FILTER(regex(str(?url), 'file://') %2 ). }" )
                    .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                          deletionBlacklistFilter() );
    Soprano::QueryResultIterator it = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );

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
        Nepomuk::ResourceManager::instance()->mainModel()->removeAllStatements( r, Soprano::Node(), Soprano::Node() );
        Nepomuk::ResourceManager::instance()->mainModel()->removeAllStatements( Soprano::Node(), Soprano::Node(), r );
    }
    kDebug() << "Done searching for invalid local file entries";
}

#include "invalidfileresourcecleaner.moc"
