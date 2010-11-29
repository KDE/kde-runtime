/* This file is part of the KDE Project
   Copyright (c) 2008-2010 Sebastian Trueg <trueg@kde.org>
   Copyright (c) 2010 Vishesh Handa <handa.vish@gmail.com>

   Parts of this file are based on code from Strigi
   Copyright (C) 2006-2007 Jos van den Oever <jos@vandenoever.info>

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

#include "indexscheduler.h"
#include "strigiserviceconfig.h"
#include "nepomukindexer.h"
#include "util.h"

#include <QtCore/QMutexLocker>
#include <QtCore/QList>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDirIterator>
#include <QtCore/QDateTime>
#include <QtCore/QByteArray>
#include <QtCore/QUrl>

#include <KDebug>
#include <KTemporaryFile>
#include <KUrl>

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
#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NIE>

#include <map>
#include <vector>


namespace {
    const int s_reducedSpeedDelay = 500; // ms
    const int s_snailPaceDelay = 3000;   // ms


    bool isResourcePresent( const QString & dir ) {
        QString query = QString::fromLatin1(" ask { ?r %1 %2. } ")
                        .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                              Soprano::Node::resourceToN3( KUrl( dir ) ) );
        return Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql ).boolValue();
    }

    QHash<QString, QDateTime> getChildren( const QString& dir )
    {
        QHash<QString, QDateTime> children;
        QString query;

        if( !isResourcePresent( dir ) ) {
            query = QString::fromLatin1( "select distinct ?url ?mtime where { "
                                         "?r %1 ?url . "
                                         "FILTER( regex(str(?url), '^file://%2/([^/]*)$') ) . "
                                         "?r %3 ?mtime ."
                                         "}" )
                    .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                          dir,
                          Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::lastModified() ) );
        }
        else {
            query = QString::fromLatin1( "select distinct ?url ?mtime where { "
                                        "?r %1 ?parent . ?parent %2 %3 . "
                                        "?r %4 ?mtime . "
                                        "?r %2 ?url . "
                                        "}" )
                    .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::isPartOf() ),
                        Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                        Soprano::Node::resourceToN3( KUrl( dir ) ),
                        Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::lastModified() ) );
        }
        //kDebug() << "running getChildren query:" << query;

        Soprano::QueryResultIterator result = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );

        while ( result.next() ) {
            children.insert( result["url"].uri().toLocalFile(), result["mtime"].literal().toDateTime() );
        }

        return children;
    }
}


Nepomuk::IndexScheduler::IndexScheduler( QObject* parent )
    : QThread( parent ),
      m_suspended( false ),
      m_stopped( false ),
      m_indexing( false ),
      m_speed( FullSpeed )
{
    m_indexer = new Nepomuk::Indexer( this );
    connect( StrigiServiceConfig::self(), SIGNAL( configChanged() ),
             this, SLOT( slotConfigChanged() ) );
}


Nepomuk::IndexScheduler::~IndexScheduler()
{
}


void Nepomuk::IndexScheduler::suspend()
{
    if ( isRunning() && !m_suspended ) {
        QMutexLocker locker( &m_resumeStopMutex );
        m_suspended = true;
        emit indexingSuspended( true );
    }
}


void Nepomuk::IndexScheduler::resume()
{
    if ( isRunning() && m_suspended ) {
        QMutexLocker locker( &m_resumeStopMutex );
        m_suspended = false;
        m_resumeStopWc.wakeAll();
        emit indexingSuspended( false );
    }
}


void Nepomuk::IndexScheduler::setSuspended( bool suspended )
{
    if ( suspended )
        suspend();
    else
        resume();
}


void Nepomuk::IndexScheduler::stop()
{
    if ( isRunning() ) {
        QMutexLocker locker( &m_resumeStopMutex );
        m_stopped = true;
        m_suspended = false;
        m_indexer->stop();
        m_dirsToUpdateWc.wakeAll();
        m_resumeStopWc.wakeAll();
    }
}


void Nepomuk::IndexScheduler::restart()
{
    stop();
    wait();
    start();
}


void Nepomuk::IndexScheduler::setIndexingSpeed( IndexingSpeed speed )
{
    kDebug() << speed;
    m_speed = speed;
}


void Nepomuk::IndexScheduler::setReducedIndexingSpeed( bool reduced )
{
    if ( reduced )
        setIndexingSpeed( ReducedSpeed );
    else
        setIndexingSpeed( FullSpeed );
}


bool Nepomuk::IndexScheduler::isSuspended() const
{
    return isRunning() && m_suspended;
}


bool Nepomuk::IndexScheduler::isIndexing() const
{
    return m_indexing;
}


QString Nepomuk::IndexScheduler::currentFolder() const
{
    return m_currentFolder;
}


QString Nepomuk::IndexScheduler::currentFile() const
{
    return m_currentUrl.toLocalFile();
}


void Nepomuk::IndexScheduler::setIndexingStarted( bool started )
{
    if ( started != m_indexing ) {
        m_indexing = started;
        emit indexingStateChanged( m_indexing );
        if ( m_indexing )
            emit indexingStarted();
        else
            emit indexingStopped();
    }
}


void Nepomuk::IndexScheduler::run()
{
    // set lowest priority for this thread
    setPriority( QThread::IdlePriority );

    setIndexingStarted( true );

    // initialization
    queueAllFoldersForUpdate();

#ifndef NDEBUG
    QTime timer;
    timer.start();
#endif

    removeOldAndUnwantedEntries();

#ifndef NDEBUG
    kDebug() << "Removed old entries:" << timer.elapsed();
    timer.restart();
#endif

    while ( waitForContinue() ) {
        // wait for more dirs to analyze in case the initial
        // indexing is done
        if ( m_dirsToUpdate.isEmpty() ) {
            setIndexingStarted( false );

#ifndef NDEBUG
            kDebug() << "All folders updated:" << timer.elapsed();
#endif

            m_dirsToUpdateMutex.lock();
            m_dirsToUpdateWc.wait( &m_dirsToUpdateMutex );
            m_dirsToUpdateMutex.unlock();

#ifndef NDEBUG
            timer.restart();
#endif

            if ( !m_stopped )
                setIndexingStarted( true );
        }

        // wait for resume or stop (or simply continue)
        if ( !waitForContinue() ) {
            break;
        }

        // get the next folder
        m_dirsToUpdateMutex.lock();
        QPair<QString, UpdateDirFlags> dir = *m_dirsToUpdate.begin();
        m_dirsToUpdate.erase( m_dirsToUpdate.begin() );
        m_dirsToUpdateMutex.unlock();

        // update until stopped
        if ( !analyzeDir( dir.first, dir.second | UpdateRecursive ) ) {
            break;
        }
        m_currentFolder.clear();
    }

    setIndexingStarted( false );

    // reset state
    m_suspended = false;
    m_stopped = false;
    m_currentFolder.clear();
}


bool Nepomuk::IndexScheduler::analyzeDir( const QString& dir, UpdateDirFlags flags )
{
//    kDebug() << dir << analyzer << recursive;

    // inform interested clients
    emit indexingFolder( dir );

    m_currentFolder = dir;
    const bool recursive = flags&UpdateRecursive;
    const bool forceUpdate = flags&ForceUpdate;

    // we start by updating the folder itself
    QFileInfo dirInfo( dir );
    KUrl dirUrl( dir );
    Nepomuk::Resource dirRes( dirUrl );
    if ( !dirRes.exists() ||
         dirRes.property( Nepomuk::Vocabulary::NIE::lastModified() ).toDateTime() != dirInfo.lastModified() ) {
        m_indexer->indexFile( dirInfo );
    }

    // get a map of all indexed files from the dir including their stored mtime
    QHash<QString, QDateTime> filesInStore = getChildren( dir );
    QHash<QString, QDateTime>::iterator filesInStoreEnd = filesInStore.end();

    QList<QFileInfo> filesToIndex;
    QStringList subFolders;
    QStringList filesToDelete;

    // iterate over all files in the dir
    // and select the ones we need to add or delete from the store
    QDir::Filters dirFilter = QDir::NoDotAndDotDot|QDir::Readable|QDir::Files|QDir::Dirs;
    QDirIterator dirIt( dir, dirFilter );
    while ( dirIt.hasNext() ) {
        QString path = dirIt.next();

        // FIXME: we cannot use canonialFilePath here since that could lead into another folder. Thus, we probably
        // need to use another approach than the getChildren one.
        QFileInfo fileInfo = dirIt.fileInfo();//.canonialFilePath();

        bool indexFile = Nepomuk::StrigiServiceConfig::self()->shouldFileBeIndexed( fileInfo.fileName() );

        // check if this file is new by looking it up in the store
        QHash<QString, QDateTime>::iterator filesInStoreIt = filesInStore.find( path );
        bool newFile = ( filesInStoreIt == filesInStoreEnd );
        if ( newFile && indexFile )
            kDebug() << "NEW    :" << path;

        // do we need to update? Did the file change?
        bool fileChanged = !newFile && fileInfo.lastModified() != filesInStoreIt.value();
        if ( fileChanged )
            kDebug() << "CHANGED:" << path << fileInfo.lastModified() << filesInStoreIt.value();

        if ( indexFile && ( newFile || fileChanged || forceUpdate ) )
            filesToIndex << fileInfo;

        // we do not delete files to update here. We do that in the IndexWriter to make
        // sure we keep the resource URI
        else if ( !newFile && !indexFile )
            filesToDelete.append( filesInStoreIt.key() );

        // cleanup a bit for faster lookups
        if ( !newFile )
            filesInStore.erase( filesInStoreIt );

        if ( indexFile && recursive && fileInfo.isDir() && !fileInfo.isSymLink() )
            subFolders << path;
    }

    // all the files left in filesInStore are not in the current
    // directory and should be deleted
    filesToDelete += filesInStore.keys();

    // remove all files that have been removed recursively
    deleteEntries( filesToDelete );

    // analyze all files that are new or need updating
    foreach( const QFileInfo& file, filesToIndex ) {

        // wait if we are suspended or return if we are stopped
        if ( !waitForContinue() )
            return false;

        m_currentUrl = file.filePath();
        m_indexer->indexFile( file );
        m_currentUrl = KUrl();
    }

    // recurse into subdirs (we do this in a separate loop to always keep a proper state:
    // compare m_currentFolder)
    if ( recursive ) {
        foreach( const QString& folder, subFolders ) {
            if ( StrigiServiceConfig::self()->shouldFolderBeIndexed( folder ) &&
                 !analyzeDir( folder, flags ) )
                return false;
        }
    }

    return true;
}


bool Nepomuk::IndexScheduler::waitForContinue( bool disableDelay )
{
    QMutexLocker locker( &m_resumeStopMutex );
    if ( m_suspended ) {
        setIndexingStarted( false );
        m_resumeStopWc.wait( &m_resumeStopMutex );
        setIndexingStarted( true );
    }
    else if ( !disableDelay && m_speed != FullSpeed ) {
        msleep( m_speed == ReducedSpeed ? s_reducedSpeedDelay : s_snailPaceDelay );
    }

    return !m_stopped;
}


void Nepomuk::IndexScheduler::updateDir( const QString& path, bool forceUpdate )
{
    QMutexLocker lock( &m_dirsToUpdateMutex );
    m_dirsToUpdate << qMakePair( path, UpdateDirFlags( forceUpdate ? ForceUpdate : NoUpdateFlags ) );
    m_dirsToUpdateWc.wakeAll();
}


void Nepomuk::IndexScheduler::updateAll( bool forceUpdate )
{
    queueAllFoldersForUpdate( forceUpdate );
    m_dirsToUpdateWc.wakeAll();
}


void Nepomuk::IndexScheduler::queueAllFoldersForUpdate( bool forceUpdate )
{
    QMutexLocker lock( &m_dirsToUpdateMutex );

    // remove previously added folders to not index stuff we are not supposed to
    QSet<QPair<QString, UpdateDirFlags> >::iterator it = m_dirsToUpdate.begin();
    while ( it != m_dirsToUpdate.end() ) {
        if ( it->second & AutoUpdateFolder )
            it = m_dirsToUpdate.erase( it );
        else
            ++it;
    }

    UpdateDirFlags flags = UpdateRecursive|AutoUpdateFolder;
    if ( forceUpdate )
        flags |= ForceUpdate;

    // update everything again in case the folders changed
    foreach( const QString& f, StrigiServiceConfig::self()->includeFolders() )
        m_dirsToUpdate << qMakePair( f, flags );
}


void Nepomuk::IndexScheduler::slotConfigChanged()
{
    // restart to make sure we update all folders and removeOldAndUnwantedEntries
    if ( isRunning() )
        restart();
}


void Nepomuk::IndexScheduler::analyzeResource( const QUrl& uri, const QDateTime& modificationTime, QDataStream& data )
{
    Indexer indexer;
    indexer.indexResource( uri, modificationTime, data );
}


void Nepomuk::IndexScheduler::analyzeFile( const QString& path )
{
    Indexer indexer;
    indexer.indexFile( QFileInfo( path ) );
}


void Nepomuk::IndexScheduler::deleteEntries( const QStringList& entries )
{
    // recurse into subdirs
    // TODO: use a less mem intensive method
    for ( int i = 0; i < entries.count(); ++i ) {
        deleteEntries( getChildren( entries[i] ).keys() );
        m_indexer->clearIndexedData( KUrl( entries[i] ) );
    }
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

void Nepomuk::IndexScheduler::removeOldAndUnwantedEntries()
{
    //
    // We now query all indexed files that are in folders that should not
    // be indexed at once.
    //
    QString folderFilter = constructFolderFilter();
    if( !folderFilter.isEmpty() )
        folderFilter = QString::fromLatin1("FILTER(%1) .").arg(folderFilter);

    //
    // We query all files that should not be in the store
    // This for example excludes all filex:/ URLs.
    //
    QString query = QString::fromLatin1( "select distinct ?g where { "
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
    // We look for indexed files that do not have a nie:url defined and thus, will never be catched by any of the
    // other queries.
    //
    Query::Query q(
        Strigi::Ontology::indexGraphFor() == ( Soprano::Vocabulary::RDF::type() == Query::ResourceTerm( Nepomuk::Vocabulary::NFO::FileDataObject() ) &&
                                               !( Nepomuk::Vocabulary::NIE::url() == Query::Term() ) )
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
bool Nepomuk::IndexScheduler::removeAllGraphsFromQuery( const QString& query )
{
    while ( 1 ) {
        // get the next batch of graphs
        QList<Soprano::Node> graphs
            = ResourceManager::instance()->mainModel()->executeQuery( query + QLatin1String( " LIMIT 200" ),
                                                                      Soprano::Query::QueryLanguageSparql ).iterateBindings( 0 ).allNodes();

        // remove all graphs in the batch
        Q_FOREACH( const Soprano::Node& graph, graphs ) {

            // wait for resume or stop (or simply continue)
            if ( !waitForContinue() ) {
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


QDebug Nepomuk::operator<<( QDebug dbg, IndexScheduler::IndexingSpeed speed )
{
    dbg << ( int )speed;
    switch( speed ) {
    case IndexScheduler::FullSpeed:
        return dbg << "FullSpeed";
    case IndexScheduler::ReducedSpeed:
        return dbg << "ReducedSpeed";
    case IndexScheduler::SnailPace:
        return dbg << "SnailPace";
    }

    // make gcc shut up
    return dbg;
}

#include "indexscheduler.moc"
