/* This file is part of the KDE Project
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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

#include "config.h"

#include <QtCore/QStringList>
#include <QtCore/QDir>

#include <kdirwatch.h>
#include <kstandarddirs.h>
#include <kconfiggroup.h>


Nepomuk::Config::Config()
    : QObject(),
      m_config( "nepomukstrigirc" )
{
    KDirWatch* dirWatch = KDirWatch::self();
    connect( dirWatch, SIGNAL( dirty( const QString& ) ),
             this, SLOT( slotConfigDirty() ) );
    connect( dirWatch, SIGNAL( created( const QString& ) ),
             this, SLOT( slotConfigDirty() ) );
    dirWatch->addFile( KStandardDirs::locateLocal( "config", m_config.name() ) );
}


Nepomuk::Config::~Config()
{
    m_config.group( "General" ).writeEntry( "first run", false );
}


Nepomuk::Config* Nepomuk::Config::self()
{
    K_GLOBAL_STATIC( Config, _self );
    return _self;
}


QStringList Nepomuk::Config::folders() const
{
    return m_config.group( "General" ).readPathEntry( "folders", QStringList() << QDir::homePath() );
}


bool Nepomuk::Config::recursive() const
{
    return m_config.group( "General" ).readEntry( "recursive", true );
}


QStringList Nepomuk::Config::excludeFilters() const
{
    return m_config.group( "General" ).readEntry( "exclude filters", QStringList() << ".*/" << ".*" << "*~" << "*.part" );
}


QStringList Nepomuk::Config::includeFilters() const
{
    return m_config.group( "General" ).readEntry( "include filters", QStringList() );
}


KIO::filesize_t Nepomuk::Config::minDiskSpace() const
{
    // default: 200 MB
    return m_config.group( "General" ).readEntry( "min disk space", KIO::filesize_t( 200*1024*1024 ) );
}


void Nepomuk::Config::slotConfigDirty()
{
    m_config.reparseConfiguration();
    emit configChanged();
}


bool Nepomuk::Config::showGui() const
{
    return m_config.group( "General" ).readEntry( "show gui", true );
}


void Nepomuk::Config::setShowGui( bool showGui )
{
    m_config.group( "General" ).writeEntry( "show gui", showGui );
}


bool Nepomuk::Config::isInitialRun() const
{
    return m_config.group( "General" ).readEntry( "first run", true );
}

#include "config.moc"
