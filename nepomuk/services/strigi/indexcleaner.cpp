/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-2011 Sebastian Trueg <trueg@kde.org>
   Copyright (C) 2010-2011 Vishesh Handa <handa.vish@gmail.com>

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

#include "indexcleaner.h"
#include "strigiserviceconfig.h"
#include "util.h"

#include <QtCore/QTimer>
#include <QtCore/QMutexLocker>
#include <KDebug>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/NodeIterator>
#include <Soprano/Node>

#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/Xesam>
#include <Soprano/Vocabulary/NAO>
#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NIE>

using namespace Nepomuk::Vocabulary;
using namespace Soprano::Vocabulary;


Nepomuk::IndexCleaner::IndexCleaner(QObject* parent)
    : KJob(parent),
      m_delay(0)
{
    setCapabilities( Suspendable );
}

namespace {
    /**
     * Creates one SPARQL filter expression that excludes the include folders.
     * This is necessary since constructFolderSubFilter will append a slash to
     * each folder to make sure it does not match something like
     * '/home/foobar' with '/home/foo'.
     */
    QString constructExcludeIncludeFoldersFilter()
    {
        QStringList filters;
        foreach( const QString& folder, Nepomuk::StrigiServiceConfig::self()->includeFolders() ) {
            filters << QString::fromLatin1( "(?url!=%1)" ).arg( Soprano::Node::resourceToN3( KUrl( folder ) ) );
        }
        return filters.join( QLatin1String( " && " ) );
    }

    QString constructFolderSubFilter( const QList<QPair<QString, bool> > folders, int& index )
    {
        QString path = folders[index].first;
        if ( !path.endsWith( '/' ) )
            path += '/';
        const bool include = folders[index].second;

        ++index;

        QStringList subFilters;
        while ( index < folders.count() &&
                folders[index].first.startsWith( path ) ) {
            subFilters << constructFolderSubFilter( folders, index );
        }

        QString thisFilter = QString::fromLatin1( "REGEX(STR(?url),'^%1')" ).arg( QString::fromAscii( KUrl( path ).toEncoded() ) );

        // we want all folders that should NOT be indexed
        if ( include ) {
            thisFilter.prepend( '!' );
        }
        subFilters.prepend( thisFilter );

        if ( subFilters.count() > 1 ) {
            return '(' + subFilters.join( include ? QLatin1String( " || " ) : QLatin1String( " && " ) ) + ')';
        }
        else {
            return subFilters.first();
        }
    }

    /**
     * Creates one SPARQL filter which matches all files and folders that should NOT be indexed.
     */
    QString constructFolderFilter()
    {
        QStringList subFilters( constructExcludeIncludeFoldersFilter() );

        // now add the actual filters
        QList<QPair<QString, bool> > folders = Nepomuk::StrigiServiceConfig::self()->folders();
        int index = 0;
        while ( index < folders.count() ) {
            subFilters << constructFolderSubFilter( folders, index );
        }
        QString filters = subFilters.join(" && ");
        if( !filters.isEmpty() )
            return QString::fromLatin1("FILTER(%1) .").arg(filters);

        return QString();
    }
}

void Nepomuk::IndexCleaner::start()
{
    const QString folderFilter = constructFolderFilter();

    const int limit = 20;

    //
    // Create all queries that return indexed data which should not be there anymore.
    //

    //
    // Query the nepomukindexer app resource in order to speed up the queries.
    //
    QUrl appRes;
    Soprano::QueryResultIterator appIt
            = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(QString::fromLatin1("select ?app where { ?app %1 %2 . } LIMIT 1")
                                                                              .arg(Soprano::Node::resourceToN3( NAO::identifier() ),
                                                                                   Soprano::Node::literalToN3(QLatin1String("nepomukindexer"))),
                                                                              Soprano::Query::QueryLanguageSparql);
    if(appIt.next()) {
        appRes = appIt[0].uri();
    }

    //
    // 1. Data that has been created in KDE >= 4.7 using the DMS
    //
    if(!appRes.isEmpty()) {
        m_removalQueries << QString::fromLatin1( "select distinct ?r where { "
                                                 "graph ?g { ?r %1 ?url . } . "
                                                 "?g %2 %3 . "
                                                 " %4 } LIMIT %5" )
                            .arg( Soprano::Node::resourceToN3( NIE::url() ),
                                  Soprano::Node::resourceToN3( NAO::maintainedBy() ),
                                  Soprano::Node::resourceToN3( appRes ),
                                  folderFilter,
                                  QString::number( limit ) );
    }


    //
    // 2. (legacy data) We query all files that should not be in the store.
    // This for example excludes all filex:/ URLs.
    //
    m_removalQueries << QString::fromLatin1( "select distinct ?r where { "
                                             "?r %1 ?url . "
                                             "?g <http://www.strigi.org/fields#indexGraphFor> ?r . "
                                             "FILTER(REGEX(STR(?url),'^file:/')) . "
                                             "%2 } LIMIT %3" )
                        .arg( Soprano::Node::resourceToN3( NIE::url() ),
                              folderFilter )
                        .arg(limit);


    //
    // 3. Build filter query for all exclude filters
    //
    QStringList fileFilters;
    foreach( const QString& filter, Nepomuk::StrigiServiceConfig::self()->excludeFilters() ) {
        QString filterRxStr = QRegExp::escape( filter );
        filterRxStr.replace( "\\*", QLatin1String( ".*" ) );
        filterRxStr.replace( "\\?", QLatin1String( "." ) );
        filterRxStr.replace( '\\',"\\\\" );
        fileFilters << QString::fromLatin1( "REGEX(STR(?fn),\"^%1$\")" ).arg( filterRxStr );
    }
    QString includeExcludeFilters = constructExcludeIncludeFoldersFilter();

    QString filters;
    if( !includeExcludeFilters.isEmpty() && !fileFilters.isEmpty() )
        filters = QString::fromLatin1("FILTER((%1) && (%2)) .").arg( includeExcludeFilters, fileFilters.join(" || ") );
    else if( !fileFilters.isEmpty() )
        filters = QString::fromLatin1("FILTER(%1) .").arg( fileFilters.join(" || ") );
    else if( !includeExcludeFilters.isEmpty() )
        filters = QString::fromLatin1("FILTER(%1) .").arg( includeExcludeFilters );

    // 3.1. Data for files which are excluded through filters
    if(!appRes.isEmpty()) {
        m_removalQueries << QString::fromLatin1( "select distinct ?r where { "
                                                 "graph ?g { ?r %1 ?url . } . "
                                                 "?r %2 ?fn . "
                                                 "?g %3 %4 . "
                                                 "FILTER(REGEX(STR(?url),\"^file:/\")) . "
                                                 "%6 } LIMIT %7" )
                            .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                                  Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NFO::fileName() ),
                                  Soprano::Node::resourceToN3( NAO::maintainedBy() ),
                                  Soprano::Node::resourceToN3( appRes ),
                                  filters )
                            .arg(limit);
    }

    // 3.2. (legacy data) Data for files which are excluded through filters
    m_removalQueries << QString::fromLatin1( "select distinct ?r where { "
                                             "?r %1 ?url . "
                                             "?r %2 ?fn . "
                                             "?g <http://www.strigi.org/fields#indexGraphFor> ?r . "
                                             "FILTER(REGEX(STR(?url),\"^file:/\")) . "
                                             "%3 } LIMIT %4" )
                        .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                              Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NFO::fileName() ),
                              filters )
                        .arg(limit);


    //
    // 4. (legacy data) Remove all old data from Xesam-times. While we leave out the data created by libnepomuk
    // there is no problem since libnepomuk still uses backwards compatible queries and we use
    // libnepomuk to determine URIs in the strigi backend.
    //
    m_removalQueries << QString::fromLatin1( "select distinct ?r where { "
                                             "?r <http://strigi.sf.net/ontologies/0.9#parentUrl> ?p1 . } LIMIT %1" )
                        .arg(limit);
    m_removalQueries << QString::fromLatin1( "select distinct ?r where { "
                                             "?r %1 ?u2 . } LIMIT %2" )
                        .arg( Soprano::Node::resourceToN3( Soprano::Vocabulary::Xesam::url() ) )
                        .arg(limit);


    //
    // 5. (legacy data) Remove data which is useless but still around from before. This could happen due to some buggy version of
    // the indexer or the filewatch service or even some application messing up the data.
    // We look for indexed files that do not have a nie:url defined and thus, will never be caught by any of the
    // other queries. In addition we check for an isPartOf relation since strigi produces EmbeddedFileDataObjects
    // for video and audio streams.
    //
    m_removalQueries << QString::fromLatin1("select ?r where { "
                                            "?g <http://www.strigi.org/fields#indexGraphFor> ?r . "
                                            "FILTER(!bif:exists((select (1) where { ?r %1 ?u . }))) . "
                                            "FILTER(!bif:exists((select (1) where { ?r %2 ?p . }))) . "
                                            "} LIMIT %3")
                        .arg(Soprano::Node::resourceToN3(NIE::url()),
                             Soprano::Node::resourceToN3(NIE::isPartOf()))
                        .arg(limit);

    //
    // Start the removal
    //
    m_query = m_removalQueries.dequeue();
    clearNextBatch();
}

void Nepomuk::IndexCleaner::slotRemoveResourcesDone(KJob* job)
{
    if( job->error() ) {
        kDebug() << job->errorString();
    }

    QMutexLocker lock(&m_stateMutex);
    if( !m_suspended ) {
        QTimer::singleShot(m_delay, this, SLOT(clearNextBatch()));
    }
}

void Nepomuk::IndexCleaner::clearNextBatch()
{
    QList<QUrl> resources;
    Soprano::QueryResultIterator it
            = ResourceManager::instance()->mainModel()->executeQuery( m_query, Soprano::Query::QueryLanguageSparql );
    while( it.next() ) {
        resources << it[0].uri();
    }

    if( !resources.isEmpty() ) {
        KJob* job = Nepomuk::clearIndexedData(resources);
        connect( job, SIGNAL(finished(KJob*)), this, SLOT(slotRemoveResourcesDone(KJob*)) );
    }

    else if( !m_removalQueries.isEmpty() ) {
        m_query = m_removalQueries.dequeue();
        clearNextBatch();
    }

    else {
        emitResult();
    }
}

bool Nepomuk::IndexCleaner::doSuspend()
{
    QMutexLocker locker(&m_stateMutex);
    m_suspended = true;
    return true;
}

bool Nepomuk::IndexCleaner::doResume()
{
    QMutexLocker locker(&m_stateMutex);
    if(m_suspended) {
        m_suspended = false;
        QTimer::singleShot( 0, this, SLOT(clearNextBatch()) );
    }
    return true;
}

void Nepomuk::IndexCleaner::setDelay(int msecs)
{
    m_delay = msecs;
}

#include "indexcleaner.moc"
