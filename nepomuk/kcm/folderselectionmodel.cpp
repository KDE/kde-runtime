/* This file is part of the KDE Project
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

   Based on CollectionSetup.cpp from the Amarok project
   (C) 2003 Scott Wheeler <wheeler@kde.org>
   (C) 2004 Max Howell <max.howell@methylblue.com>
   (C) 2004 Mark Kretschmann <markey@web.de>
   (C) 2008 Seb Ruiz <ruiz@kde.org>

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

#include "folderselectionmodel.h"

#include <QtCore/QDir>


FolderSelectionModel::FolderSelectionModel( QObject* parent )
    : QFileSystemModel( parent ),
      m_recursive( true )
{
    setFilter( QDir::AllDirs | QDir::NoDotAndDotDot );
}


FolderSelectionModel::~FolderSelectionModel()
{
}


Qt::ItemFlags FolderSelectionModel::flags( const QModelIndex &index ) const
{
    Qt::ItemFlags flags = QFileSystemModel::flags( index );
    const QString path = filePath( index );
    if( ( recursive() && ancestorChecked( path ) ) || isForbiddenPath( path ) )
        flags ^= Qt::ItemIsEnabled; //disabled!

    flags |= Qt::ItemIsUserCheckable;

    return flags;
}


QVariant FolderSelectionModel::data( const QModelIndex& index, int role ) const
{
    if( index.isValid() && index.column() == 0 && role == Qt::CheckStateRole )
    {
        const QString path = filePath( index );
        if( recursive() && ancestorChecked( path ) )
            return Qt::Checked; // always set children of recursively checked parents to checked
        if( isForbiddenPath( path ) )
            return Qt::Unchecked; // forbidden paths can never be checked
        if ( !m_checked.contains( path ) && descendantChecked( path ) )
            return Qt::PartiallyChecked;
        return m_checked.contains( path ) ? Qt::Checked : Qt::Unchecked;
    }
    return QFileSystemModel::data( index, role );
}


bool FolderSelectionModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if( index.isValid() && index.column() == 0 && role == Qt::CheckStateRole )
    {
        QString path = filePath( index );
        // store checked paths, remove unchecked paths
        if( value.toInt() == Qt::Checked )
            m_checked.insert( path );
        else
            m_checked.remove( path );
        return true;
    }
    return QFileSystemModel::setData( index, value, role );
}


void FolderSelectionModel::setFolders( const QStringList &dirs )
{
    m_checked.clear();
    foreach( const QString& dir, dirs ) {
        m_checked.insert( dir );
    }
}


QStringList FolderSelectionModel::folders() const
{
    QStringList dirs = m_checked.toList();

    qSort( dirs.begin(), dirs.end() );

    // we need to remove any children of selected items as
    // they are redundant when recursive mode is chosen
    if( recursive() ) {
        foreach( const QString& dir, dirs ) {
            if( ancestorChecked( dir ) )
                dirs.removeAll( dir );
        }
    }

    return dirs;
}


inline bool FolderSelectionModel::isForbiddenPath( const QString& path ) const
{
    // we need the trailing slash otherwise we could forbid "/dev-music" for example
    QString _path = path.endsWith( "/" ) ? path : path + "/";
    return _path.startsWith( "/proc/" ) || _path.startsWith( "/dev/" ) || _path.startsWith( "/sys/" );
}


bool FolderSelectionModel::ancestorChecked( const QString& path ) const
{
    foreach( const QString& element, m_checked ) {
        if( path.startsWith( element ) && element != path )
            return true;
    }
    return false;
}


bool FolderSelectionModel::descendantChecked( const QString& path ) const
{
    foreach( const QString& element, m_checked ) {
        if ( element.startsWith( path ) && element != path )
            return true;
    }
    return false;
}


bool FolderSelectionModel::recursive() const
{
    return m_recursive;
}


void FolderSelectionModel::setRecursive( bool r )
{
    m_recursive = r;
}

#include "folderselectionmodel.moc"
