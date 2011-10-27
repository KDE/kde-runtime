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

#include "strigiserviceconfig.h"
#include "fileexcludefilters.h"

#include <QtCore/QStringList>
#include <QtCore/QDir>

#include <KDirWatch>
#include <KStandardDirs>
#include <KConfigGroup>
#include <KDebug>


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

    bool isDirHidden(const QString& path) {
        QDir dir(path);
        return isDirHidden(dir);
    }
}

Nepomuk::StrigiServiceConfig* Nepomuk::StrigiServiceConfig::s_self = 0;

Nepomuk::StrigiServiceConfig::StrigiServiceConfig(QObject* parent)
    : QObject(parent),
      m_config( "nepomukstrigirc" )
{
    if(!s_self) {
        s_self = this;
    }

    KDirWatch* dirWatch = KDirWatch::self();
    connect( dirWatch, SIGNAL( dirty( const QString& ) ),
             this, SLOT( slotConfigDirty() ) );
    connect( dirWatch, SIGNAL( created( const QString& ) ),
             this, SLOT( slotConfigDirty() ) );
    dirWatch->addFile( KStandardDirs::locateLocal( "config", m_config.name() ) );

    buildFolderCache();
    buildExcludeFilterRegExpCache();
}


Nepomuk::StrigiServiceConfig::~StrigiServiceConfig()
{
}


Nepomuk::StrigiServiceConfig* Nepomuk::StrigiServiceConfig::self()
{
    return s_self;
}


QList<QPair<QString, bool> > Nepomuk::StrigiServiceConfig::folders() const
{
    return m_folderCache;
}


QStringList Nepomuk::StrigiServiceConfig::includeFolders() const
{
    QStringList fl;
    for ( int i = 0; i < m_folderCache.count(); ++i ) {
        if ( m_folderCache[i].second )
            fl << m_folderCache[i].first;
    }
    return fl;
}


QStringList Nepomuk::StrigiServiceConfig::excludeFolders() const
{
    QStringList fl;
    for ( int i = 0; i < m_folderCache.count(); ++i ) {
        if ( !m_folderCache[i].second )
            fl << m_folderCache[i].first;
    }
    return fl;
}


QStringList Nepomuk::StrigiServiceConfig::excludeFilters() const
{
    KConfigGroup cfg = m_config.group( "General" );

    // read configured exclude filters
    QSet<QString> filters = cfg.readEntry( "exclude filters", defaultExcludeFilterList() ).toSet();

    // make sure we always keep the latest default exclude filters
    // TODO: there is one problem here. What if the user removed some of the default filters?
    if(cfg.readEntry("exclude filters version", 0) < defaultExcludeFilterListVersion()) {
        filters += defaultExcludeFilterList().toSet();

        // write the config directly since the KCM does not have support for the version yet
        // TODO: make this class public and use it in the KCM
        cfg.writeEntry("exclude filters", QStringList::fromSet(filters));
        cfg.writeEntry("exclude filters version", defaultExcludeFilterListVersion());
    }

    // remove duplicates
    return QStringList::fromSet(filters);
}


bool Nepomuk::StrigiServiceConfig::indexHiddenFilesAndFolders() const
{
    return m_config.group( "General" ).readEntry( "index hidden folders", false );
}


KIO::filesize_t Nepomuk::StrigiServiceConfig::minDiskSpace() const
{
    // default: 200 MB
    return m_config.group( "General" ).readEntry( "min disk space", KIO::filesize_t( 200*1024*1024 ) );
}


void Nepomuk::StrigiServiceConfig::slotConfigDirty()
{
    m_config.reparseConfiguration();
    buildFolderCache();
    buildExcludeFilterRegExpCache();
    emit configChanged();
}


bool Nepomuk::StrigiServiceConfig::isInitialRun() const
{
    return m_config.group( "General" ).readEntry( "first run", true );
}


bool Nepomuk::StrigiServiceConfig::shouldBeIndexed( const QString& path ) const
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


bool Nepomuk::StrigiServiceConfig::shouldFolderBeIndexed( const QString& path ) const
{
    QString folder;
    if ( folderInFolderList( path, folder ) ) {
        // we always index the folders in the list
        // ignoring the name filters
        if ( folder == path )
            return true;

        // check for hidden folders
        QDir dir( path );
        if ( !indexHiddenFilesAndFolders() && isDirHidden( dir ) )
            return false;

        // reset dir, cause isDirHidden modifies the QDir
        dir = path;

        // check the exclude filters for all components of the path
        // after folder
        const QStringList pathComponents = path.mid(folder.count()).split('/', QString::SkipEmptyParts);
        foreach(const QString& c, pathComponents) {
            if(!shouldFileBeIndexed( c )) {
                return false;
            }
        }
        return true;
    }
    else {
        return false;
    }
}


bool Nepomuk::StrigiServiceConfig::shouldFileBeIndexed( const QString& fileName ) const
{
    // check the filters
    QMutexLocker lock( &m_folderCacheMutex );
    return !m_excludeFilterRegExpCache.exactMatch( fileName );
}


bool Nepomuk::StrigiServiceConfig::folderInFolderList( const QString& path, QString& folder ) const
{
    QMutexLocker lock( &m_folderCacheMutex );

    const QString p = KUrl( path ).path( KUrl::RemoveTrailingSlash );

    // we traverse the list backwards to catch all exclude folders
    int i = m_folderCache.count();
    while ( --i >= 0 ) {
        const QString& f = m_folderCache[i].first;
        const bool include = m_folderCache[i].second;
        if ( p.startsWith( f ) ) {
            folder = f;
            return include;
        }
    }
    // path is not in the list, thus it should not be included
    folder.clear();
    return false;
}


namespace {
    /**
     * Returns true if the specified folder f would already be excluded using the list
     * folders.
     */
    bool alreadyExcluded( const QList<QPair<QString, bool> >& folders, const QString& f )
    {
        bool included = false;
        for ( int i = 0; i < folders.count(); ++i ) {
            if ( f != folders[i].first &&
                 f.startsWith( KUrl( folders[i].first ).path( KUrl::AddTrailingSlash ) ) ) {
                included = folders[i].second;
            }
        }
        return !included;
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
     * Remove useless exclude entries which would confuse the folderInFolderList algo.
     * This makes sure all top-level items are include folders.
     * This runs in O(n^2) and could be optimized but what for.
     */
    void cleanupList( QList<QPair<QString, bool> >& result )
    {
        int i = 0;
        while ( i < result.count() ) {
            if ( result[i].first.isEmpty() ||
                 (!result[i].second &&
                  alreadyExcluded( result, result[i].first ) ))
                result.removeAt( i );
            else
                ++i;
        }
    }
}


void Nepomuk::StrigiServiceConfig::buildFolderCache()
{
    QMutexLocker lock( &m_folderCacheMutex );

    QStringList includeFoldersPlain = m_config.group( "General" ).readPathEntry( "folders", QStringList() << QDir::homePath() );
    QStringList excludeFoldersPlain = m_config.group( "General" ).readPathEntry( "exclude folders", QStringList() );

    m_folderCache.clear();
    insertSortFolders( includeFoldersPlain, true, m_folderCache );
    insertSortFolders( excludeFoldersPlain, false, m_folderCache );
    cleanupList( m_folderCache );
}


void Nepomuk::StrigiServiceConfig::buildExcludeFilterRegExpCache()
{
    QMutexLocker lock( &m_folderCacheMutex );
    m_excludeFilterRegExpCache.rebuildCacheFromFilterList( excludeFilters() );
}

void Nepomuk::StrigiServiceConfig::setInitialRun(bool isInitialRun)
{
    m_config.group( "General" ).writeEntry( "first run", isInitialRun );
}

bool Nepomuk::StrigiServiceConfig::initialUpdateDisabled() const
{
    return m_config.group( "General" ).readEntry( "disable initial update", false );
}

bool Nepomuk::StrigiServiceConfig::suspendOnPowerSaveDisabled() const
{
    return m_config.group( "General" ).readEntry( "disable suspend on powersave", false );
}

#include "strigiserviceconfig.moc"
