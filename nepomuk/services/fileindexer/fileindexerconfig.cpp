/* This file is part of the KDE Project
   Copyright (c) 2008-2010 Sebastian Trueg <trueg@kde.org>

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

#include "fileindexerconfig.h"
#include "fileexcludefilters.h"

#include <QtCore/QStringList>
#include <QtCore/QDir>

#include <KDirWatch>
#include <KStandardDirs>
#include <KConfigGroup>


Nepomuk::FileIndexerConfig::FileIndexerConfig()
    : QObject(),
      m_config( "nepomukstrigirc" )
{
    KDirWatch* dirWatch = KDirWatch::self();
    connect( dirWatch, SIGNAL( dirty( const QString& ) ),
             this, SLOT( slotConfigDirty() ) );
    connect( dirWatch, SIGNAL( created( const QString& ) ),
             this, SLOT( slotConfigDirty() ) );
    dirWatch->addFile( KStandardDirs::locateLocal( "config", m_config.name() ) );

    buildFolderCache();
    buildExcludeFilterRegExpCache();
}


Nepomuk::FileIndexerConfig::~FileIndexerConfig()
{
    m_config.group( "General" ).writeEntry( "first run", false );
}


Nepomuk::FileIndexerConfig* Nepomuk::FileIndexerConfig::self()
{
    K_GLOBAL_STATIC( FileIndexerConfig, _self );
    return _self;
}


QList<QPair<QString, bool> > Nepomuk::FileIndexerConfig::folders() const
{
    return m_folderCache;
}


QStringList Nepomuk::FileIndexerConfig::includeFolders() const
{
    QStringList fl;
    for ( int i = 0; i < m_folderCache.count(); ++i ) {
        if ( m_folderCache[i].second )
            fl << m_folderCache[i].first;
    }
    return fl;
}


QStringList Nepomuk::FileIndexerConfig::excludeFolders() const
{
    QStringList fl;
    for ( int i = 0; i < m_folderCache.count(); ++i ) {
        if ( !m_folderCache[i].second )
            fl << m_folderCache[i].first;
    }
    return fl;
}


QStringList Nepomuk::FileIndexerConfig::excludeFilters() const
{
    return m_config.group( "General" ).readEntry( "exclude filters", defaultExcludeFilterList() );
}


bool Nepomuk::FileIndexerConfig::indexHiddenFilesAndFolders() const
{
    return m_config.group( "General" ).readEntry( "index hidden folders", false );
}


KIO::filesize_t Nepomuk::FileIndexerConfig::minDiskSpace() const
{
    // default: 200 MB
    return m_config.group( "General" ).readEntry( "min disk space", KIO::filesize_t( 200*1024*1024 ) );
}


void Nepomuk::FileIndexerConfig::slotConfigDirty()
{
    m_config.reparseConfiguration();
    buildFolderCache();
    buildExcludeFilterRegExpCache();
    emit configChanged();
}


bool Nepomuk::FileIndexerConfig::isInitialRun() const
{
    return m_config.group( "General" ).readEntry( "first run", true );
}


bool Nepomuk::FileIndexerConfig::shouldBeIndexed( const QString& path ) const
{
    QFileInfo fi( path );
    if( fi.isDir() ) {
        return shouldFolderBeIndexed( path );
    }
    else {
        return( shouldFolderBeIndexed( fi.absolutePath() ) &&
                ( !fi.isHidden() || indexHiddenFilesAndFolders() ) &&
                shouldFileBeIndexed( fi.fileName() ) );
    }
}


namespace {
    /// recursively check if a folder is hidden
    bool isDirHidden( QDir& dir ) {
        if ( QFileInfo( dir.path() ).isHidden() )
            return true;
        else if ( dir.cdUp() )
            return isDirHidden( dir );
        else
            return false;
    }
}

bool Nepomuk::FileIndexerConfig::shouldFolderBeIndexed( const QString& path ) const
{
    bool exact = false;
    if ( folderInFolderList( path, exact ) ) {
        // we always index the folders in the list
        // ignoring the name filters
        if ( exact )
            return true;

        // check for hidden folders
        QDir dir( path );
        if ( !indexHiddenFilesAndFolders() && isDirHidden( dir ) )
            return false;

        // reset dir, cause isDirHidden modifies the QDir
        dir = path;

        // check the filters
        return shouldFileBeIndexed( dir.dirName() );
    }
    else {
        return false;
    }
}


bool Nepomuk::FileIndexerConfig::shouldFileBeIndexed( const QString& fileName ) const
{
    // check the filters
    QMutexLocker lock( &m_folderCacheMutex );
    return !m_excludeFilterRegExpCache.exactMatch( fileName );
}


bool Nepomuk::FileIndexerConfig::folderInFolderList( const QString& path, bool& exact ) const
{
    QMutexLocker lock( &m_folderCacheMutex );

    const QString p = KUrl( path ).path( KUrl::RemoveTrailingSlash );

    // we traverse the list backwards to catch all exclude folders
    int i = m_folderCache.count();
    while ( --i >= 0 ) {
        const QString& f = m_folderCache[i].first;
        const bool include = m_folderCache[i].second;
        if ( p.startsWith( f ) ) {
            exact = ( p == f );
            return include;
        }
    }
    // path is not in the list, thus it should not be included
    return false;
}


namespace {
    /**
     * Returns true if the specified folder f would already be included or excluded using the list
     * folders
     */
    bool alreadyInList( const QList<QPair<QString, bool> >& folders, const QString& f, bool include )
    {
        bool included = false;
        for ( int i = 0; i < folders.count(); ++i ) {
            if ( f != folders[i].first &&
                 f.startsWith( KUrl( folders[i].first ).path( KUrl::AddTrailingSlash ) ) )
                included = folders[i].second;
        }
        return included == include;
    }

    /**
     * Simple insertion sort
     */
    void insertSortFolders( const QStringList& folders, bool include, QList<QPair<QString, bool> >& result )
    {
        foreach( const QString& f, folders ) {
            int pos = 0;
            QString path = KUrl( f ).path( KUrl::RemoveTrailingSlash );
            while ( result.count() > pos &&
                    result[pos].first < path )
                ++pos;
            result.insert( pos, qMakePair( path, include ) );
        }
    }

    /**
     * Remove useless entries which would confuse the algo below.
     * This makes sure all top-level items are include folders.
     * This runs in O(n^2) and could be optimized but what for.
     */
    void cleanupList( QList<QPair<QString, bool> >& result )
    {
        int i = 0;
        while ( i < result.count() ) {
            if ( result[i].first.isEmpty() ||
                 alreadyInList( result, result[i].first, result[i].second ) )
                result.removeAt( i );
            else
                ++i;
        }
    }
}


void Nepomuk::FileIndexerConfig::buildFolderCache()
{
    QMutexLocker lock( &m_folderCacheMutex );

    QStringList includeFoldersPlain = m_config.group( "General" ).readPathEntry( "folders", QStringList() << QDir::homePath() );
    QStringList excludeFoldersPlain = m_config.group( "General" ).readPathEntry( "exclude folders", QStringList() );

    m_folderCache.clear();
    insertSortFolders( includeFoldersPlain, true, m_folderCache );
    insertSortFolders( excludeFoldersPlain, false, m_folderCache );
    cleanupList( m_folderCache );
}


void Nepomuk::FileIndexerConfig::buildExcludeFilterRegExpCache()
{
    QMutexLocker lock( &m_folderCacheMutex );
    m_excludeFilterRegExpCache.rebuildCacheFromFilterList( excludeFilters() );
}

bool Nepomuk::FileIndexerConfig::isDebugModeEnabled() const
{
    return m_config.group( "General" ).readEntry( "debug mode", false );
}

#include "fileindexerconfig.moc"
