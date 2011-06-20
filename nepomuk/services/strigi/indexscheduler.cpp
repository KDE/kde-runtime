/* This file is part of the KDE Project
   Copyright (c) 2008-2010 Sebastian Trueg <trueg@kde.org>
   Copyright (c) 2010-11 Vishesh Handa <handa.vish@gmail.com>

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
#include "datamanagement.h"

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
#include <Nepomuk/Query/ResourceTerm>

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/NodeIterator>
#include <Soprano/Node>

#include <Soprano/Vocabulary/RDF>
#include <Nepomuk/Vocabulary/NIE>

#include <map>
#include <vector>
#include "indexcleaner.h"

using namespace Soprano::Vocabulary;
using namespace Nepomuk::Vocabulary;

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

    QDateTime indexedMTimeForUrl(const KUrl& url)
    {
        Soprano::QueryResultIterator it
                = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(QString::fromLatin1("select ?mt where { ?r %1 %2 . ?r %3 ?mt . } LIMIT 1")
                                                                                  .arg(Soprano::Node::resourceToN3(NIE::url()),
                                                                                       Soprano::Node::resourceToN3(url),
                                                                                       Soprano::Node::resourceToN3(NIE::lastModified())),
                                                                                  Soprano::Query::QueryLanguageSparql);
        if(it.next()) {
            return it[0].literal().toDateTime();
        }
        else {
            return QDateTime();
        }
    }

    bool compareIndexedMTime(const KUrl& url, const QDateTime& mtime)
    {
        const QDateTime indexedMTime = indexedMTimeForUrl(url);
        if(indexedMTime.isNull())
            return false;
        else
            return indexedMTime == mtime;
    }
}


void Nepomuk::IndexScheduler::UpdateDirQueue::enqueueDir( const QString& dir, UpdateDirFlags flags )
{
    if( contains( qMakePair( dir, flags ) ) )
        return;

    if( !(flags & AutoUpdateFolder) ) {
        int i = 0;
        while( i < count() && !(at(i).second & AutoUpdateFolder) )
            ++i;
        insert( i, qMakePair( dir, flags ) );
    }
    else {
        enqueue( qMakePair( dir, flags ) );
    }
}


void Nepomuk::IndexScheduler::UpdateDirQueue::prependDir( const QString& dir, UpdateDirFlags flags )
{
    if( contains( qMakePair( dir, flags ) ) )
        return;

    if( flags & AutoUpdateFolder ) {
        int i = 0;
        while( i < count() && !(at(i).second & AutoUpdateFolder) )
            ++i;
        insert( i, qMakePair( dir, flags ) );
    }
    else {
        prepend( qMakePair( dir, flags ) );
    }
}


void Nepomuk::IndexScheduler::UpdateDirQueue::clearByFlags( UpdateDirFlags mask )
{
    QQueue<QPair<QString, UpdateDirFlags> >::iterator it = begin();
    while ( it != end() ) {
        if ( it->second & mask )
            it = erase( it );
        else
            ++it;
    }
}



Nepomuk::IndexScheduler::IndexScheduler( QObject* parent )
    : QObject( parent ),
      m_suspended( false ),
      m_indexing( false ),
      m_speed( FullSpeed )
{
    m_cleaner = new IndexCleaner();
    m_cleaner->start();

    connect( m_cleaner, SIGNAL(finished(KJob*)), this, SLOT(slotCleaningDone()) );

    connect( StrigiServiceConfig::self(), SIGNAL( configChanged() ),
             this, SLOT( slotConfigChanged() ) );
}


Nepomuk::IndexScheduler::~IndexScheduler()
{
}


void Nepomuk::IndexScheduler::suspend()
{
    if ( !m_suspended ) {
        QMutexLocker locker( &m_resumeStopMutex );
        m_suspended = true;
        if( m_cleaner ) {
            m_cleaner->suspend();
        }
        emit indexingSuspended( true );
    }
}


void Nepomuk::IndexScheduler::resume()
{
    if ( m_suspended ) {
        QMutexLocker locker( &m_resumeStopMutex );
        m_suspended = false;

        if( m_cleaner ) {
            m_cleaner->resume();
        }
        else {
            callDoIndexing();
        }
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
    return m_suspended;
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


void Nepomuk::IndexScheduler::slotCleaningDone()
{
    // initialization
    queueAllFoldersForUpdate();

    // reset state
    m_suspended = false;
    m_cleaner = 0;

    callDoIndexing();
}

void Nepomuk::IndexScheduler::doIndexing()
{
    setIndexingStarted( true );

    // get the next file
    if( !m_filesToUpdate.isEmpty() ) {
        kDebug() << "File";

        QFileInfo file = m_filesToUpdate.dequeue();

        m_currentUrl = file.filePath();
        m_currentFolder = m_currentUrl.directory();

        KJob * indexer = new Indexer( file );
        connect( indexer, SIGNAL(finished(KJob*)), this, SLOT(slotIndexingDone(KJob*)) );
        indexer->start();

        emit indexingFile( m_currentUrl.toLocalFile() );
    }

    // get the next folder
    else if( !m_dirsToUpdate.isEmpty() ) {
        kDebug() << "Directory";

        m_dirsToUpdateMutex.lock();
        QPair<QString, UpdateDirFlags> dir = m_dirsToUpdate.dequeue();
        m_dirsToUpdateMutex.unlock();

        // If a dir was not analyzed, then doIndexing must be called to
        // process the next file/directory in the queue
        if( !analyzeDir( dir.first, dir.second ) ) {
            callDoIndexing();
        }
    }

    else {
        setIndexingStarted( false );
    }
}

void Nepomuk::IndexScheduler::slotIndexingDone(KJob* job)
{
    kDebug() << job;
    Q_UNUSED( job );

    m_currentFolder.clear();
    m_currentUrl.clear();

    callDoIndexing();
}

bool Nepomuk::IndexScheduler::analyzeDir( const QString& dir_, Nepomuk::IndexScheduler::UpdateDirFlags flags )
{
//    kDebug() << dir << analyzer << recursive;

    // normalize the dir name, otherwise things might break below
    QString dir( dir_ );
    if( dir.endsWith(QLatin1String("/")) ) {
        dir.truncate( dir.length()-1 );
    }

    // inform interested clients
    emit indexingFolder( dir );

    m_currentFolder = dir;
    m_currentUrl = KUrl( dir );

    const bool recursive = flags&UpdateRecursive;
    const bool forceUpdate = flags&ForceUpdate;

    // we start by updating the folder itself
    QFileInfo dirInfo( dir );
    KUrl dirUrl( dir );
    bool shouldAnalyzerDir = !compareIndexedMTime(dirUrl, dirInfo.lastModified());
    if ( shouldAnalyzerDir ) {
        KJob * indexer = new Indexer( dirInfo );
        connect( indexer, SIGNAL(finished(KJob*)), this, SLOT(slotIndexingDone(KJob*)) );
        indexer->start();
    }

    // get a map of all indexed files from the dir including their stored mtime
    QHash<QString, QDateTime> filesInStore = getChildren( dir );
    QHash<QString, QDateTime>::iterator filesInStoreEnd = filesInStore.end();

    QList<QFileInfo> filesToIndex;
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
        else if( forceUpdate )
            kDebug() << "UPDATE FORCED:" << path;

        if ( indexFile && ( newFile || fileChanged || forceUpdate ) )
            filesToIndex << fileInfo;

        // we do not delete files to update here. We do that in the IndexWriter to make
        // sure we keep the resource URI
        else if ( !newFile && !indexFile )
            filesToDelete.append( filesInStoreIt.key() );

        // cleanup a bit for faster lookups
        if ( !newFile )
            filesInStore.erase( filesInStoreIt );

        // prepend sub folders to the dir queue
        if ( indexFile &&
                recursive &&
                fileInfo.isDir() &&
                !fileInfo.isSymLink() &&
                StrigiServiceConfig::self()->shouldFolderBeIndexed( path ) ) {
            QMutexLocker lock( &m_dirsToUpdateMutex );
            m_dirsToUpdate.prependDir( path, flags );
        }
    }

    // all the files left in filesInStore are not in the current
    // directory and should be deleted
    filesToDelete += filesInStore.keys();

    // remove all files that have been removed recursively
    deleteEntries( filesToDelete );

    // analyze all files that are new or need updating
    m_filesToUpdate.append( filesToIndex );

    return shouldAnalyzerDir;
}


void Nepomuk::IndexScheduler::callDoIndexing()
{
    if( !m_suspended ) {
        uint delay = 0;
        if ( m_speed != FullSpeed ) {
            delay = (m_speed == ReducedSpeed) ? s_reducedSpeedDelay : s_snailPaceDelay;
        }
        QTimer::singleShot( delay, this, SLOT(doIndexing()) );
    }
}


void Nepomuk::IndexScheduler::updateDir( const QString& path, UpdateDirFlags flags )
{
    QMutexLocker lock( &m_dirsToUpdateMutex );
    m_dirsToUpdate.prependDir( path, flags & ~AutoUpdateFolder );

    if( !m_indexing )
        callDoIndexing();
}


void Nepomuk::IndexScheduler::updateAll( bool forceUpdate )
{
    queueAllFoldersForUpdate( forceUpdate );

    if( !m_indexing )
        callDoIndexing();
}


void Nepomuk::IndexScheduler::queueAllFoldersForUpdate( bool forceUpdate )
{
    QMutexLocker lock( &m_dirsToUpdateMutex );

    // remove previously added folders to not index stuff we are not supposed to
    m_dirsToUpdate.clearByFlags( AutoUpdateFolder );

    UpdateDirFlags flags = UpdateRecursive|AutoUpdateFolder;
    if ( forceUpdate )
        flags |= ForceUpdate;

    // update everything again in case the folders changed
    foreach( const QString& f, StrigiServiceConfig::self()->includeFolders() ) {
        m_dirsToUpdate.enqueueDir( f, flags );
    }
}


void Nepomuk::IndexScheduler::slotConfigChanged()
{
    // restart to make sure we update all folders and removeOldAndUnwantedEntries
    if( m_cleaner ) {
        m_cleaner->kill();
        delete m_cleaner;
    }

    m_cleaner = new IndexCleaner( this );
    connect( m_cleaner, SIGNAL(finished(KJob*)), this, SLOT(slotCleaningDone()) );
    m_cleaner->start();
}


void Nepomuk::IndexScheduler::analyzeFile( const QString& path )
{
    KJob * indexer = new Indexer( KUrl(path) );
    indexer->start();
}


void Nepomuk::IndexScheduler::deleteEntries( const QStringList& entries )
{
    // recurse into subdirs
    // TODO: use a less mem intensive method
    for ( int i = 0; i < entries.count(); ++i ) {
        deleteEntries( getChildren( entries[i] ).keys() );
    }
    const KUrl::List urls(entries);
    Nepomuk::clearIndexedData(urls);
    Nepomuk::clearLegacyIndexedDataForUrls(urls);
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
