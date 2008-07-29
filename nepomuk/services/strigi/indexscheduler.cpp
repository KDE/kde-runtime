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
#include "config.h"

#include <QtCore/QMutexLocker>
#include <QtCore/QList>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDirIterator>
#include <QtCore/QDateTime>

#include <KDebug>

#include <map>
#include <vector>

#include <strigi/indexwriter.h>
#include <strigi/indexmanager.h>
#include <strigi/indexreader.h>
#include <strigi/analysisresult.h>
#include <strigi/fileinputstream.h>
#include <strigi/analyzerconfiguration.h>


class StoppableConfiguration : public Strigi::AnalyzerConfiguration {
public:
    StoppableConfiguration()
        : m_stop(false) {
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


Nepomuk::IndexScheduler::IndexScheduler( Strigi::IndexManager* manager, QObject* parent )
    : QThread( parent ),
      m_suspended( false ),
      m_stopped( false ),
      m_indexing( false ),
      m_indexManager( manager )
{
    m_analyzerConfig = new StoppableConfiguration;

    connect( Config::self(), SIGNAL( configChanged() ),
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
    foreach( const QString& f, Config::self()->folders() )
        m_dirsToUpdate << qMakePair( f, Config::self()->recursive() );

    m_startedRecursive = Config::self()->recursive();

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
        QPair<QString, bool> dir = *m_dirsToUpdate.begin();
        m_dirsToUpdate.erase( m_dirsToUpdate.begin() );
        m_dirsToUpdateMutex.unlock();

        // update until stopped
        if ( !updateDir( dir.first, &analyzer, dir.second ) ) {
            break;
        }
        m_currentFolder.clear();
    }

    setIndexingStarted( false );
}


// this method should be thread-safe ("should" because of the indexreader and -writer)
bool Nepomuk::IndexScheduler::updateDir( const QString& dir, Strigi::StreamAnalyzer* analyzer, bool recursive )
{
//    kDebug() << dir << analyzer << recursive;

    m_currentFolder = dir;

    // get a map of all indexed files from the dir including their stored mtime
    std::map<std::string, time_t> filesInStore;
    m_indexManager->indexReader()->getChildren( QFile::encodeName( dir ).data(), filesInStore );
    std::map<std::string, time_t>::const_iterator filesInStoreEnd = filesInStore.end();

    QList<QFileInfo> filesToIndex;
    QList<QString> subFolders;
    std::vector<std::string> filesToDelete;

    // iterate over all files in the dir
    // and select the ones we need to add or delete from the store
    QDirIterator dirIt( dir, QDir::NoDotAndDotDot|QDir::Readable|QDir::Files|QDir::Dirs );
    while ( dirIt.hasNext() ) {
        QString path = dirIt.next();

        QFileInfo fileInfo = dirIt.fileInfo();

        // check if this file is new by looking it up in the store
        std::map<std::string, time_t>::iterator filesInStoreIt = filesInStore.find( QFile::encodeName( path ).data() );
        bool newFile = ( filesInStoreIt == filesInStoreEnd );

        // do we need to update? Did the file change?
        bool fileChanged = !newFile && fileInfo.lastModified().toTime_t() != filesInStoreIt->second;

        if ( newFile || fileChanged )
            filesToIndex << fileInfo;

        if ( !newFile && fileChanged )
            filesToDelete.push_back( filesInStoreIt->first );

        // cleanup a bit for faster lookups
        if ( !newFile )
            filesInStore.erase( filesInStoreIt );

        if ( recursive && fileInfo.isDir() && !fileInfo.isSymLink() )
            subFolders << path;
    }

    // all the files left in filesInStore are not in the current
    // directory and should be deleted
    for ( std::map<std::string, time_t>::const_iterator it = filesInStore.begin();
          it != filesInStoreEnd; ++it ) {
        filesToDelete.push_back( it->first );
    }

    // inform interested clients
    if ( !filesToIndex.isEmpty() || !filesToDelete.empty() )
        emit indexingFolder( dir );

    // remove all files that need updating or have been removed
    m_indexManager->indexWriter()->deleteEntries( filesToDelete );

    // analyse all files that are new or need updating
    foreach( const QFileInfo& file, filesToIndex ) {

        analyzeFile( file, analyzer );

        // wait if we are suspended or return if we are stopped
        if ( !waitForContinue() )
            return false;
    }

    // recurse into subdirs (we do this in a separate loop to always keep a proper state:
    // compare m_currentFolder)
    if ( recursive ) {
        foreach( const QString& folder, subFolders ) {
            if ( !updateDir( folder, analyzer, true ) )
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
    m_dirsToUpdate << qMakePair( path, false );
    m_dirsToUpdateWc.wakeAll();
}


void Nepomuk::IndexScheduler::updateAll()
{
    QMutexLocker lock( &m_dirsToUpdateMutex );
    foreach( const QString& f, Config::self()->folders() )
        m_dirsToUpdate << qMakePair( f, true );
    m_dirsToUpdateWc.wakeAll();
}


void Nepomuk::IndexScheduler::readConfig()
{
    // load Strigi configuration
    std::vector<std::pair<bool, std::string> > filters;
    QStringList excludeFilters = Config::self()->excludeFilters();
    QStringList includeFilters = Config::self()->includeFilters();
    foreach( const QString& filter, excludeFilters ) {
        filters.push_back( std::make_pair<bool, std::string>( false, filter.toUtf8().data() ) );
    }
    foreach( const QString& filter, includeFilters ) {
        filters.push_back( std::make_pair<bool, std::string>( true, filter.toUtf8().data() ) );
    }
    m_analyzerConfig->setFilters(filters);

    // if the recursion setting changed, update everything again
    // FIXME: this does not really help if the new value is false!
    if ( m_startedRecursive != Config::self()->recursive() ) {
        QMutexLocker lock( &m_dirsToUpdateMutex );
        foreach( const QString& f, Config::self()->folders() )
            m_dirsToUpdate << qMakePair( f, Config::self()->recursive() );
        m_dirsToUpdateWc.wakeAll();
    }
}

#include "indexscheduler.moc"
