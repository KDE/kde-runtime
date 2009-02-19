/* This file is part of the KDE Project
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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

#include <map>
#include <vector>

#include <strigi/strigiconfig.h>
#include <strigi/indexwriter.h>
#include <strigi/indexmanager.h>
#include <strigi/indexreader.h>
#include <strigi/analysisresult.h>
#include <strigi/fileinputstream.h>
#include <strigi/analyzerconfiguration.h>


// FIXME: remove all files from the datastore which are in folders not supposed to be indexed

class StoppableConfiguration : public Strigi::AnalyzerConfiguration {
public:
    StoppableConfiguration()
        : m_stop(false) {
#if defined(STRIGI_IS_VERSION)
#if STRIGI_IS_VERSION( 0, 6, 1 )
        setIndexArchiveContents( false );
#endif
#endif
    }

    bool indexMore() const {
        return !m_stop;
    }

    bool addMoreText() const {
        return !m_stop;
    }

    void setStop( bool s ) {
        m_stop = s;
    }

private:
    bool m_stop;
};


namespace {
    enum UpdateDirFlags {
        /**
         * No flags, only used to make code more readable
         */
        NoUpdateFlags = 0x0,

        /**
         * The folder should be updated recursive
         */
        UpdateRecursive = 0x1,

        /**
         * The folder has been scheduled to update by the
         * update system, not by a call to updateDir
         */
        AutoUpdateFolder = 0x2
    };
}


Nepomuk::IndexScheduler::IndexScheduler( Strigi::IndexManager* manager, QObject* parent )
    : QThread( parent ),
      m_suspended( false ),
      m_stopped( false ),
      m_indexing( false ),
      m_indexManager( manager )
{
    m_analyzerConfig = new StoppableConfiguration;

    connect( StrigiServiceConfig::self(), SIGNAL( configChanged() ),
             this, SLOT( readConfig() ) );
}


Nepomuk::IndexScheduler::~IndexScheduler()
{
    delete m_analyzerConfig;
}


void Nepomuk::IndexScheduler::suspend()
{
    if ( isRunning() ) {
        QMutexLocker locker( &m_resumeStopMutex );
        m_suspended = true;
    }
}


void Nepomuk::IndexScheduler::resume()
{
    if ( isRunning() ) {
        QMutexLocker locker( &m_resumeStopMutex );
        m_suspended = false;
        m_resumeStopWc.wakeAll();
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
        m_analyzerConfig->setStop( true );
        m_dirsToUpdateWc.wakeAll();
        m_resumeStopWc.wakeAll();
    }
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


void Nepomuk::IndexScheduler::setIndexingStarted( bool started )
{
    if ( started != m_indexing ) {
        m_indexing = started;
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

    // initialization
    m_suspended = false;
    m_stopped = false;
    m_analyzerConfig->setStop( false );
    readConfig();

    Strigi::StreamAnalyzer analyzer( *m_analyzerConfig );
    analyzer.setIndexWriter( *m_indexManager->indexWriter() );

    setIndexingStarted( true );

    // do the actual indexing
    m_dirsToUpdate.clear();
    foreach( const QString& f, StrigiServiceConfig::self()->folders() )
        m_dirsToUpdate << qMakePair( f, UpdateRecursive|AutoUpdateFolder );

    while ( 1 ) {
        // wait for more dirs to analyze in case the initial
        // indexing is done
        if ( m_dirsToUpdate.isEmpty() ) {
            setIndexingStarted( false );

            m_dirsToUpdateMutex.lock();
            m_dirsToUpdateWc.wait( &m_dirsToUpdateMutex );
            m_dirsToUpdateMutex.unlock();

            if ( !m_stopped )
                setIndexingStarted( true );
        }

        // wait for resume or stop (or simply continue)
        if ( !waitForContinue() ) {
            break;
        }

        // get the next folder
        m_dirsToUpdateMutex.lock();
        QPair<QString, int> dir = *m_dirsToUpdate.begin();
        m_dirsToUpdate.erase( m_dirsToUpdate.begin() );
        m_dirsToUpdateMutex.unlock();

        // update until stopped
        if ( !updateDir( dir.first, &analyzer, dir.second & UpdateRecursive ) ) {
            break;
        }
        m_currentFolder.clear();
    }

    setIndexingStarted( false );
}


// FIXME: the folders in StrigiServiceConfig::self()->folders() themselves are not indexed.

// this method should be thread-safe ("should" because of the unknown situation with indexreader and -writer)
bool Nepomuk::IndexScheduler::updateDir( const QString& dir, Strigi::StreamAnalyzer* analyzer, bool recursive )
{
//    kDebug() << dir << analyzer << recursive;

    // inform interested clients
    emit indexingFolder( dir );

    m_currentFolder = dir;

    // get a map of all indexed files from the dir including their stored mtime
    std::map<std::string, time_t> filesInStore;
    m_indexManager->indexReader()->getChildren( QFile::encodeName( dir ).data(), filesInStore );
    std::map<std::string, time_t>::const_iterator filesInStoreEnd = filesInStore.end();

    QList<QFileInfo> filesToIndex;
    QList<QString> subFolders;
    std::vector<std::string> filesToUpdate, filesToDelete;

    // iterate over all files in the dir
    // and select the ones we need to add or delete from the store
    QDir::Filters dirFilter = QDir::NoDotAndDotDot|QDir::Readable|QDir::Files|QDir::Dirs;
    QDirIterator dirIt( dir, dirFilter );
    while ( dirIt.hasNext() ) {
        QString path = dirIt.next();

        QFileInfo fileInfo = dirIt.fileInfo();

        bool indexFile = m_analyzerConfig->indexFile( QFile::encodeName( path ), QFile::encodeName( fileInfo.fileName() ) );

        // check if this file is new by looking it up in the store
        std::map<std::string, time_t>::iterator filesInStoreIt = filesInStore.find( QFile::encodeName( path ).data() );
        bool newFile = ( filesInStoreIt == filesInStoreEnd );

        // do we need to update? Did the file change?
        bool fileChanged = !newFile && fileInfo.lastModified().toTime_t() != filesInStoreIt->second;

        if ( indexFile && ( newFile || fileChanged ) )
            filesToIndex << fileInfo;

        if ( !newFile && fileChanged )
            filesToUpdate.push_back( filesInStoreIt->first );
        else if ( !newFile && !indexFile )
            filesToDelete.push_back( filesInStoreIt->first );

        // cleanup a bit for faster lookups
        if ( !newFile )
            filesInStore.erase( filesInStoreIt );

        if ( indexFile && recursive && fileInfo.isDir() && !fileInfo.isSymLink() )
            subFolders << path;
    }

    // all the files left in filesInStore are not in the current
    // directory and should be deleted
    for ( std::map<std::string, time_t>::const_iterator it = filesInStore.begin();
          it != filesInStoreEnd; ++it ) {
        filesToDelete.push_back( it->first );
    }

    // remove all files non-recursively that need updating
    m_indexManager->indexWriter()->deleteEntries( filesToUpdate );

    // remove all files that have been removed recursively
    deleteEntries( filesToDelete );

    // analyze all files that are new or need updating
    foreach( const QFileInfo& file, filesToIndex ) {

        // wait if we are suspended or return if we are stopped
        if ( !waitForContinue() )
            return false;

        analyzeFile( file, analyzer );
    }

    // recurse into subdirs (we do this in a separate loop to always keep a proper state:
    // compare m_currentFolder)
    if ( recursive ) {
        foreach( const QString& folder, subFolders ) {
            if ( !StrigiServiceConfig::self()->excludeFolders().contains( folder ) &&
                 !updateDir( folder, analyzer, true ) )
                return false;
        }
    }

    return true;
}


void Nepomuk::IndexScheduler::analyzeFile( const QFileInfo& file, Strigi::StreamAnalyzer* analyzer )
{
//    kDebug() << file.filePath();

    Strigi::AnalysisResult analysisresult( QFile::encodeName( file.filePath() ).data(),
                                           file.lastModified().toTime_t(),
                                           *m_indexManager->indexWriter(),
                                           *analyzer,
                                           QFile::encodeName( file.path() ).data() );
    if ( file.isFile() && !file.isSymLink() ) {
        Strigi::FileInputStream stream( QFile::encodeName( file.filePath() ) );
        analysisresult.index( &stream );
    }
    else {
        analysisresult.index(0);
    }
}


bool Nepomuk::IndexScheduler::waitForContinue()
{
    QMutexLocker locker( &m_resumeStopMutex );
    if ( m_suspended ) {
        setIndexingStarted( false );
        m_resumeStopWc.wait( &m_resumeStopMutex );
        setIndexingStarted( true );
    }

    return !m_stopped;
}


void Nepomuk::IndexScheduler::updateDir( const QString& path )
{
    QMutexLocker lock( &m_dirsToUpdateMutex );
    m_dirsToUpdate << qMakePair( path, ( int )NoUpdateFlags );
    m_dirsToUpdateWc.wakeAll();
}


void Nepomuk::IndexScheduler::updateAll()
{
    QMutexLocker lock( &m_dirsToUpdateMutex );

    // remove previously added folders to not index stuff we are not supposed to
    // (FIXME: this does not include currently being indexed folders)
    QSet<QPair<QString, int> >::iterator it = m_dirsToUpdate.begin();
    while ( it != m_dirsToUpdate.end() ) {
        if ( it->second & AutoUpdateFolder )
            it = m_dirsToUpdate.erase( it );
        else
            ++it;
    }

    // update everything again in case the folders changed
    foreach( const QString& f, StrigiServiceConfig::self()->folders() )
        m_dirsToUpdate << qMakePair( f, UpdateRecursive|AutoUpdateFolder );

    m_dirsToUpdateWc.wakeAll();
}


void Nepomuk::IndexScheduler::readConfig()
{
    // load Strigi configuration
    std::vector<std::pair<bool, std::string> > filters;
    QStringList excludeFilters = StrigiServiceConfig::self()->excludeFilters();
    QStringList includeFilters = StrigiServiceConfig::self()->includeFilters();
    foreach( const QString& filter, excludeFilters ) {
        filters.push_back( std::make_pair<bool, std::string>( false, filter.toUtf8().data() ) );
    }
    foreach( const QString& filter, includeFilters ) {
        filters.push_back( std::make_pair<bool, std::string>( true, filter.toUtf8().data() ) );
    }
    m_analyzerConfig->setFilters(filters);
    updateAll();
}


namespace {
    class QDataStreamStrigiBufferedStream : public Strigi::BufferedStream<char>
    {
    public:
        QDataStreamStrigiBufferedStream( QDataStream& stream )
            : m_stream( stream ) {
        }

        int32_t fillBuffer( char* start, int32_t space ) {
            int r = m_stream.readRawData( start, space );
            if ( r == 0 ) {
                // Strigi's API is so weird!
                return -1;
            }
            else if ( r < 0 ) {
                // Again: weird API. m_status is a protected member of StreamBaseBase (yes, 2x Base)
                m_status = Strigi::Error;
                return -1;
            }
            else {
                return r;
            }
        }

    private:
        QDataStream& m_stream;
    };
}


void Nepomuk::IndexScheduler::analyzeResource( const QUrl& uri, const QDateTime& modificationTime, QDataStream& data )
{
    QDateTime existingMTime = QDateTime::fromTime_t( m_indexManager->indexReader()->mTime( uri.toEncoded().data() ) );
    if ( existingMTime < modificationTime ) {
        // remove the old data
        std::vector<std::string> entries;
        entries.push_back( uri.toEncoded().data() );
        m_indexManager->indexWriter()->deleteEntries( entries );

        // create the new
        Strigi::StreamAnalyzer analyzer( *m_analyzerConfig );
        analyzer.setIndexWriter( *m_indexManager->indexWriter() );
        Strigi::AnalysisResult analysisresult( uri.toEncoded().data(),
                                               modificationTime.toTime_t(),
                                               *m_indexManager->indexWriter(),
                                               analyzer );
        QDataStreamStrigiBufferedStream stream( data );
        analysisresult.index( &stream );
    }
    else {
        kDebug() << uri << "up to date";
    }
}


void Nepomuk::IndexScheduler::deleteEntries( const std::vector<std::string>& entries )
{
    // recurse into subdirs
    for ( unsigned int i = 0; i < entries.size(); ++i ) {
        std::map<std::string, time_t> filesInStore;
        m_indexManager->indexReader()->getChildren( entries[i], filesInStore );
        std::vector<std::string> filesToDelete;
        for ( std::map<std::string, time_t>::const_iterator it = filesInStore.begin();
              it != filesInStore.end(); ++it ) {
            filesToDelete.push_back( it->first );
        }
        deleteEntries( filesToDelete );
    }
    m_indexManager->indexWriter()->deleteEntries( entries );
}

#include "indexscheduler.moc"
