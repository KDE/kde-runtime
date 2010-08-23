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
#include "nie.h"

#include <QtCore/QList>
#include <QtCore/QFile>

#include <Soprano/Node>
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>

#include <Nepomuk/ResourceManager>

#include <KDebug>


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
    //
    // Since the removal of the graphs could intefere with the iterator and result
    // in jumping of rows (worst case) we cache all graphs to remove
    //
    QList<Soprano::Node> graphsToRemove;
    QString query = QString::fromLatin1( "select distinct ?g ?url where { "
                                         "?r %1 ?url. "
                                         "FILTER(regex(str(?url), 'file://')) . "
                                         "graph ?g { ?r ?p ?o. } }" )
                    .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ) );
    Soprano::QueryResultIterator it = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );

    while( it.next() && !m_stopped ) {
        QUrl url( it["url"].uri() );
        QString file = url.toLocalFile();

        if( !file.isEmpty() && !QFile::exists(file) ) {
            kDebug() << "REMOVING " << file;
            graphsToRemove << it["g"];
        }
    }

    Q_FOREACH( const Soprano::Node& g, graphsToRemove ) {
        if ( m_stopped )
            break;
        Nepomuk::ResourceManager::instance()->mainModel()->removeContext( g );
    }
    kDebug() << "Done searching for invalid local file entries";
}

#include "invalidfileresourcecleaner.moc"
