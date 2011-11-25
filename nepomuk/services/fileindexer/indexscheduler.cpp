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
#include "fileindexerconfig.h"
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

    QHash<QString, QDateTime> getChildren( const QString& dir )
    {
        QHash<QString, QDateTime> children;
        QString query = QString::fromLatin1( "select distinct ?url ?mtime where { "
                                             "?r %1 ?parent . ?parent %2 %3 . "
                                             "?r %4 ?mtime . "
                                             "?r %2 ?url . "
                                             "}" )
                .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::isPartOf() ),
                      Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                      Soprano::Node::resourceToN3( KUrl( dir ) ),
                      Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::lastModified() ) );
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



Nepomuk::IndexScheduler::FileQueue::FileQueue()
    : m_folderOffset(0)
{
}

void Nepomuk::IndexScheduler::FileQueue::enqueue(const QFileInfo &t)
{
    // we prepend the file to give preference to newly created and changed files over
    // the initial indexing. Sadly operator== cannot be relied on for QFileInfo. Thus
    // we need to do a dumb search
    const QString path = t.absoluteFilePath();
    QMutableListIterator<QFileInfo> it(*this);
    while(it.hasNext()) {
        if(it.next().absoluteFilePath() == path) {
            if(t.isDir()) {
                kDebug() << "Folder already queued:" << path;
                // we do not change the order of folders.
                // indexing folders is fast and we need to make sure that super-folders
                // are always indexed before their children.
                return;
            }
            else {
                kDebug() << "Already queued:" << path << "Moving to front of queue.";
                it.remove();
                break;
            }
        }
    }

//    kDebug() << "Queuing:" << path;

    if(t.isDir()) {
        insert(m_folderOffset++, t);
    }
    else {
        QQueue<QFileInfo>::enqueue(t);
    }
}

void Nepomuk::IndexScheduler::FileQueue::enqueue(const QList<QFileInfo> &infos)
{
    foreach(const QFileInfo& fi, infos) {
        enqueue(fi);
    }
}

QFileInfo Nepomuk::IndexScheduler::FileQueue::dequeue()
{
    QFileInfo info = QQueue<QFileInfo>::dequeue();
    if(info.isDir()) {
        --m_folderOffset;
    }
    return info;
}


Nepomuk::IndexScheduler::IndexScheduler( QObject* parent )
    : QObject( parent ),
      m_suspended( false ),
      m_indexing( false ),
      m_indexingDelay( 0 ),
      m_currentIndexerJob( 0 )
{
    m_cleaner = new IndexCleaner(this);
    connect( m_cleaner, SIGNAL(finished(KJob*)), this, SLOT(slotCleaningDone()) );
    m_cleaner->start();

    connect( FileIndexerConfig::self(), SIGNAL( configChanged() ),
             this, SLOT( slotConfigChanged() ) );
}


Nepomuk::IndexScheduler::~IndexScheduler()
{
}


void Nepomuk::IndexScheduler::suspend()
{
    QMutexLocker locker( &m_suspendMutex );
    if ( !m_suspended ) {
        m_suspended = true;
        if( m_cleaner ) {
            m_cleaner->suspend();
        }
        emit indexingSuspended( true );
    }
}


void Nepomuk::IndexScheduler::resume()
{
    QMutexLocker locker( &m_suspendMutex );
    if ( m_suspended ) {
        m_suspended = false;

        if( m_cleaner ) {
            m_cleaner->resume();
        }

        callDoIndexing();

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
    m_indexingDelay = 0;
    if ( speed != FullSpeed ) {
        m_indexingDelay = (speed == ReducedSpeed) ? s_reducedSpeedDelay : s_snailPaceDelay;
    }
    if( m_cleaner ) {
        m_cleaner->setDelay(m_indexingDelay);
    }
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
    QMutexLocker locker( &m_suspendMutex );
    return m_suspended;
}


bool Nepomuk::IndexScheduler::isIndexing() const
{
    QMutexLocker locker( &m_indexingMutex );
    return m_indexing;
}


QString Nepomuk::IndexScheduler::currentFolder() const
{
    QMutexLocker locker( &m_currentMutex );
    return m_currentUrl.directory();
}


QString Nepomuk::IndexScheduler::currentFile() const
{
    QMutexLocker locker( &m_currentMutex );
    return m_currentUrl.toLocalFile();
}


Nepomuk::IndexScheduler::UpdateDirFlags Nepomuk::IndexScheduler::currentFlags() const
{
    QMutexLocker locker( &m_currentMutex );
    return m_currentFlags;
}


void Nepomuk::IndexScheduler::setIndexingStarted( bool started )
{
    QMutexLocker locker( &m_indexingMutex );

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
    m_cleaner = 0;
}

void Nepomuk::IndexScheduler::doIndexing()
{
    setIndexingStarted( true );

    // lock file and dir queues as we check their status
    QMutexLocker fileLock( &m_filesToUpdateMutex );
    QMutexLocker dirLock( &m_dirsToUpdateMutex );

    // get the next file
    if( !m_filesToUpdate.isEmpty() ) {
        QFileInfo file = m_filesToUpdate.dequeue();

        m_currentMutex.lock();
        m_currentUrl = file.filePath();
        m_currentMutex.unlock();
        emit indexingFile( currentFile() );

        if(m_currentIndexerJob) {
            kError() << "Already running an indexer job on URL" << m_currentIndexerJob->url() << "This is a scheduling bug!";
        }
        else {
            m_currentIndexerJob = new Indexer( file );
            connect( m_currentIndexerJob, SIGNAL(finished(KJob*)), this, SLOT(slotIndexingDone(KJob*)), Qt::DirectConnection );
            m_currentIndexerJob->start();
        }
    }

    // get the next folder
    else if( !m_dirsToUpdate.isEmpty() ) {
        QPair<QString, UpdateDirFlags> dir = m_dirsToUpdate.dequeue();
        dirLock.unlock();
        fileLock.unlock();

        analyzeDir( dir.first, dir.second );
        callDoIndexing();
    }

    else {
        // reset status
        m_currentMutex.lock();
        m_currentUrl.clear();
        m_currentFlags = NoUpdateFlags;
        m_currentMutex.unlock();

        setIndexingStarted( false );

        emit indexingDone();
    }
}

void Nepomuk::IndexScheduler::slotIndexingDone(KJob* job)
{
    kDebug() << job;
    Q_UNUSED( job );

    m_currentIndexerJob = 0;

    m_currentMutex.lock();
    m_currentUrl.clear();
    m_currentFlags = NoUpdateFlags;
    m_currentMutex.unlock();

    callDoIndexing();
}

void Nepomuk::IndexScheduler::analyzeDir( const QString& dir_, Nepomuk::IndexScheduler::UpdateDirFlags flags )
{
    kDebug() << dir_;

    // normalize the dir name, otherwise things might break below
    QString dir( dir_ );
    if( dir.endsWith(QLatin1String("/")) ) {
        dir.truncate( dir.length()-1 );
    }

    // inform interested clients
    emit indexingFolder( dir );
    m_currentMutex.lock();
    m_currentUrl = KUrl( dir );
    m_currentFlags = flags;
    m_currentMutex.unlock();

    const bool recursive = flags&UpdateRecursive;
    const bool forceUpdate = flags&ForceUpdate;

    // we start by updating the folder itself
    QFileInfo dirInfo( dir );
    KUrl dirUrl( dir );
    if ( !compareIndexedMTime(dirUrl, dirInfo.lastModified()) ) {
        m_filesToUpdateMutex.lock();
        m_filesToUpdate.enqueue(dirInfo);
        m_filesToUpdateMutex.unlock();
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

        bool indexFile = Nepomuk::FileIndexerConfig::self()->shouldFileBeIndexed( fileInfo.fileName() );

        // check if this file is new by looking it up in the store
        QHash<QString, QDateTime>::iterator filesInStoreIt = filesInStore.find( path );
        bool newFile = ( filesInStoreIt == filesInStoreEnd );
        if ( newFile && indexFile )
            kDebug() << "NEW    :" << path;

        // do we need to update? Did the file change?
        bool fileChanged = !newFile && fileInfo.lastModified() != filesInStoreIt.value();
        //TODO: At some point make these "NEW", "CHANGED", and "FORCED" strings public
        //      so that they can be used to create a better status message.
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
                FileIndexerConfig::self()->shouldFolderBeIndexed( path ) ) {
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
    m_filesToUpdateMutex.lock();
    m_filesToUpdate.enqueue( filesToIndex );
    m_filesToUpdateMutex.unlock();

    // reset status
    m_currentMutex.lock();
    m_currentUrl.clear();
    m_currentFlags = NoUpdateFlags;
    m_currentMutex.unlock();
}


void Nepomuk::IndexScheduler::callDoIndexing(bool noDelay)
{
    if( !m_suspended ) {
        QTimer::singleShot( noDelay ? 0 : m_indexingDelay, this, SLOT(doIndexing()) );
    }
}


void Nepomuk::IndexScheduler::updateDir( const QString& path, UpdateDirFlags flags )
{
    QMutexLocker dirLock( &m_dirsToUpdateMutex );
    m_dirsToUpdate.prependDir( path, flags & ~AutoUpdateFolder );

    QMutexLocker statusLock( &m_indexingMutex );
    if( !m_indexing )
        callDoIndexing();
}


void Nepomuk::IndexScheduler::updateAll( bool forceUpdate )
{
    queueAllFoldersForUpdate( forceUpdate );

    QMutexLocker locker( &m_indexingMutex );
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
    foreach( const QString& f, FileIndexerConfig::self()->includeFolders() ) {
        m_dirsToUpdate.enqueueDir( f, flags );
    }
}


void Nepomuk::IndexScheduler::slotConfigChanged()
{
    // TODO: only update folders that were added in the config
    updateAll();

    if( m_cleaner ) {
        m_cleaner->kill();
        delete m_cleaner;
    }

    // TODO: only clean the folders that were removed from the config
    m_cleaner = new IndexCleaner( this );
    connect( m_cleaner, SIGNAL(finished(KJob*)), this, SLOT(slotCleaningDone()) );
    m_cleaner->start();
}


void Nepomuk::IndexScheduler::analyzeFile( const QString& path )
{
    kDebug() << path;
    QMutexLocker fileLock(&m_filesToUpdateMutex);
    m_filesToUpdate.enqueue(path);

    // continue indexing without any delay. We want changes reflected as soon as possible
    QMutexLocker statusLock(&m_indexingMutex);
    if( !m_indexing ) {
        callDoIndexing(true);
    }
}


void Nepomuk::IndexScheduler::deleteEntries( const QStringList& entries )
{
    // recurse into subdirs
    // TODO: use a less mem intensive method
    for ( int i = 0; i < entries.count(); ++i ) {
        deleteEntries( getChildren( entries[i] ).keys() );
    }
    Nepomuk::clearIndexedData(KUrl::List(entries));
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
