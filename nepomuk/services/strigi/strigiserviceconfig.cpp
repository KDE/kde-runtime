/* This file is part of the KDE Project
   Copyright (c) 2008-2009 Sebastian Trueg <trueg@kde.org>

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

#include <QtCore/QStringList>
#include <QtCore/QDir>

#include <kdirwatch.h>
#include <kstandarddirs.h>
#include <kconfiggroup.h>


Nepomuk::StrigiServiceConfig::StrigiServiceConfig()
    : QObject(),
      m_config( "nepomukstrigirc" )
{
    KDirWatch* dirWatch = KDirWatch::self();
    connect( dirWatch, SIGNAL( dirty( const QString& ) ),
             this, SLOT( slotConfigDirty() ) );
    connect( dirWatch, SIGNAL( created( const QString& ) ),
             this, SLOT( slotConfigDirty() ) );
    dirWatch->addFile( KStandardDirs::locateLocal( "config", m_config.name() ) );

    buildExcludeFilterRegExpCache();
}


Nepomuk::StrigiServiceConfig::~StrigiServiceConfig()
{
    m_config.group( "General" ).writeEntry( "first run", false );
}


Nepomuk::StrigiServiceConfig* Nepomuk::StrigiServiceConfig::self()
{
    K_GLOBAL_STATIC( StrigiServiceConfig, _self );
    return _self;
}


QStringList Nepomuk::StrigiServiceConfig::folders() const
{
    return m_config.group( "General" ).readPathEntry( "folders", QStringList() << QDir::homePath() );
}


QStringList Nepomuk::StrigiServiceConfig::excludeFolders() const
{
    return m_config.group( "General" ).readPathEntry( "exclude folders", QStringList() );
}


QStringList Nepomuk::StrigiServiceConfig::excludeFilters() const
{
    return m_config.group( "General" ).readEntry( "exclude filters", QStringList() << ".*/" << ".*" << "*~" << "*.part" );
}


QStringList Nepomuk::StrigiServiceConfig::includeFilters() const
{
    return m_config.group( "General" ).readEntry( "include filters", QStringList() );
}


bool Nepomuk::StrigiServiceConfig::indexHiddenFolders() const
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
    buildExcludeFilterRegExpCache();
    emit configChanged();
}


bool Nepomuk::StrigiServiceConfig::isInitialRun() const
{
    return m_config.group( "General" ).readEntry( "first run", true );
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

bool Nepomuk::StrigiServiceConfig::shouldFolderBeIndexed( const QString& path )
{
    if ( folderInFolderList( path, folders(), excludeFolders() ) ) {
        // check for hidden folders
        QDir dir( path );
        if ( !indexHiddenFolders() && isDirHidden( dir ) )
            return false;

        // reset dir
        dir = path;

        // check the filters
        const QString dirName = dir.dirName();
        foreach( const QRegExp& filter, m_excludeFilterRegExpCache ) {
            if ( filter.exactMatch( dirName ) ) {
                return false;
            }
        }

        return true;
    }
    else {
        return false;
    }
}


bool Nepomuk::StrigiServiceConfig::folderInFolderList( const QString& path, const QStringList& inDirs, const QStringList& exDirs ) const
{
    if( inDirs.contains( path ) ) {
        return true;
    }
    else if( exDirs.contains( path ) ) {
        return false;
    }
    else {
        QString parent = path.section( QDir::separator(), 0, -2, QString::SectionSkipEmpty|QString::SectionIncludeLeadingSep );
        if( parent.isEmpty() ) {
            return false;
        }
        else {
            return folderInFolderList( parent, inDirs, exDirs );
        }
    }
}


void Nepomuk::StrigiServiceConfig::buildExcludeFilterRegExpCache()
{
    m_excludeFilterRegExpCache.clear();
    foreach( const QString& filter, excludeFilters() ) {
        QString filterRxStr = QRegExp::escape( filter );
        filterRxStr.replace( "\\*", QLatin1String( ".*" ) );
        filterRxStr.replace( "\\?", QLatin1String( "." ) );
        m_excludeFilterRegExpCache << QRegExp( filterRxStr );
    }
}

#include "strigiserviceconfig.moc"
