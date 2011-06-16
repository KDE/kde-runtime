/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-11 Sebastian Trueg <trueg@kde.org>
   Copyright (C) 2010-11 Vishesh Handa <handa.vish@gmail.com>

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

#include <KDebug>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>
#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/ComparisonTerm>
#include <Nepomuk/Query/ResourceTerm>

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
    : QObject(parent),
      m_stopped( false )
{

}

Nepomuk::IndexCleaner::~IndexCleaner()
{

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
        return subFilters.join(" && ");
    }
}

void Nepomuk::IndexCleaner::removeOldAndUnwantedEntries()
{
    //
    // We now query all indexed files that are in folders that should not
    // be indexed at once.
    //
    QString folderFilter = constructFolderFilter();
    if( !folderFilter.isEmpty() )
        folderFilter = QString::fromLatin1("FILTER(%1) .").arg(folderFilter);

    //
    // Query all the resources which are maintained by nepomukindexer
    // and are not in one of the indexed folders
    //
    // vHanda: trying to remove more than 20 resources via removeDataByApp occasionally times out.
    const int limit = 20;
    QString query = QString::fromLatin1( "select distinct ?r where { "
                                         "graph ?g { ?r %1 ?url . } "
                                         "?g %2 ?app . "
                                         "?app %3 %4 . "
                                         " %5 } LIMIT %6" )
                    .arg( Soprano::Node::resourceToN3( NIE::url() ),
                          Soprano::Node::resourceToN3( NAO::maintainedBy() ),
                          Soprano::Node::resourceToN3( NAO::identifier() ),
                          Soprano::Node::literalToN3(QLatin1String("nepomukindexer")),
                          folderFilter,
                          QString::number( limit ) );

    while( true ) {
        QList<QUrl> resources;
        Soprano::QueryResultIterator it = ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );
        while( it.next() ) {
            resources << it[0].uri();
        }
        Nepomuk::blockingClearIndexedData(resources);

        // wait for resume or stop (or simply continue)
        kDebug() << "CHECKING";
        if ( m_stopped ) {
            kDebug() << "RETURNING";
            return;
        }

        if( resources.size() < limit )
            break;
    }

    //
    // We query all files that should not be in the store
    // This for example excludes all filex:/ URLs.
    //
    query = QString::fromLatin1( "select distinct ?g where { "
                                 "?r %1 ?url . "
                                 "?g <http://www.strigi.org/fields#indexGraphFor> ?r . "
                                 "FILTER(REGEX(STR(?url),'^file:/')) . "
                                 "%2 }" )
                    .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                          folderFilter );
    kDebug() << query;
    if ( !removeAllGraphsFromQuery( query ) )
        return;


    //
    // Build filter query for all exclude filters
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

    query = QString::fromLatin1( "select distinct ?g where { "
                                 "?r %1 ?url . "
                                 "?r %2 ?fn . "
                                 "?g <http://www.strigi.org/fields#indexGraphFor> ?r . "
                                 "FILTER(REGEX(STR(?url),\"^file:/\")) . "
                                 "%3 }" )
            .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                  Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NFO::fileName() ),
                  filters );
    kDebug() << query;
    if ( !removeAllGraphsFromQuery( query ) )
        return;


    //
    // Remove all old data from Xesam-times. While we leave out the data created by libnepomuk
    // there is no problem since libnepomuk still uses backwards compatible queries and we use
    // libnepomuk to determine URIs in the strigi backend.
    //
    query = QString::fromLatin1( "select distinct ?g where { "
                                 "?g <http://www.strigi.org/fields#indexGraphFor> ?x . "
                                 "{ graph ?g { ?r1 <http://strigi.sf.net/ontologies/0.9#parentUrl> ?p1 . } } "
                                 "UNION "
                                 "{ graph ?g { ?r2 %1 ?u2 . } } "
                                 "}" )
            .arg( Soprano::Node::resourceToN3( Soprano::Vocabulary::Xesam::url() ) );
    kDebug() << query;
    if ( !removeAllGraphsFromQuery( query ) )
        return;


    //
    // Remove data which is useless but still around from before. This could happen due to some buggy version of
    // the indexer or the filewatch service or even some application messing up the data.
    // We look for indexed files that do not have a nie:url defined and thus, will never be cached by any of the
    // other queries. In addition we check for an isPartOf relation since strigi produces EmbeddedFileDataObjects
    // for video and audio streams.
    //
    Query::Query q(
        Strigi::Ontology::indexGraphFor()
                == ( Soprano::Vocabulary::RDF::type()
                     == Query::ResourceTerm( Nepomuk::Resource::fromResourceUri(Nepomuk::Vocabulary::NFO::FileDataObject()) ) &&
                     !( Nepomuk::Vocabulary::NIE::url() == Query::Term() ) &&
                     !( Nepomuk::Vocabulary::NIE::isPartOf() == Query::Term() ) )
        );
    q.setQueryFlags(Query::Query::NoResultRestrictions);
    query = q.toSparqlQuery();
    kDebug() << query;
    removeAllGraphsFromQuery( query );
}



/**
 * Runs the query using a limit until all graphs have been deleted. This is not done
 * in one big loop to avoid the problems with messed up iterators when one of the iterated
 * item is deleted.
 */
bool Nepomuk::IndexCleaner::removeAllGraphsFromQuery( const QString& query )
{
    while ( 1 ) {
        // get the next batch of graphs
        QList<Soprano::Node> graphs
            = ResourceManager::instance()->mainModel()->executeQuery( query + QLatin1String( " LIMIT 200" ),
                                                                      Soprano::Query::QueryLanguageSparql ).iterateBindings( 0 ).allNodes();

        // remove all graphs in the batch
        Q_FOREACH( const Soprano::Node& graph, graphs ) {

            // wait for resume or stop (or simply continue)
            if ( m_stopped ) {
                return false;
            }

            ResourceManager::instance()->mainModel()->removeContext( graph );
        }

        // we are done when the last graphs are queried
        if ( graphs.count() < 200 ) {
            return true;
        }
    }

    // make gcc shut up
    return true;
}

