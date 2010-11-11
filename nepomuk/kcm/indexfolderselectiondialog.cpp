/* This file is part of the KDE Project
   Copyright (c) 2010 Sebastian Trueg <trueg@kde.org>

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

#include "indexfolderselectiondialog.h"
#include "folderselectionmodel.h"

#include <QtGui/QTreeView>


namespace {
    void expandRecursively( const QModelIndex& index, QTreeView* view ) {
        if ( index.isValid() ) {
            view->expand( index );
            expandRecursively( index.parent(), view );
        }
    }

    bool isDirHidden( const QString& dir ) {
        QDir d( dir );
        while ( !d.isRoot() ) {
            if ( QFileInfo( d.path() ).isHidden() )
                return true;
            if ( !d.cdUp() )
                return false; // dir does not exist or is not readable
        }
        return false;
    }

    QStringList removeHiddenFolders( const QStringList& folders ) {
        QStringList newFolders( folders );
        for ( QStringList::iterator it = newFolders.begin(); it != newFolders.end(); /* do nothing here */ ) {
            if ( isDirHidden( *it ) ) {
                it = newFolders.erase( it );
            }
            else {
                ++it;
            }
        }
        return newFolders;
    }
}


Nepomuk::IndexFolderSelectionDialog::IndexFolderSelectionDialog( QWidget* parent )
    : KDialog( parent )
{
    setupUi( mainWidget() );
    setCaption( i18nc("@title:window Referring to the folders which will be searched for files to index for desktop search", "Customizing Index Folders"));

    m_editStrigiExcludeFilters->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_folderModel = new FolderSelectionModel( m_viewIndexFolders );
    m_viewIndexFolders->setModel( m_folderModel );
    m_viewIndexFolders->setHeaderHidden( true );
    m_viewIndexFolders->header()->setStretchLastSection( false );
    m_viewIndexFolders->header()->setResizeMode( QHeaderView::ResizeToContents );
    m_viewIndexFolders->setRootIsDecorated( true );
    m_viewIndexFolders->setAnimated( true );
    m_viewIndexFolders->setRootIndex( m_folderModel->setRootPath( QDir::rootPath() ) );

    connect( m_checkShowHiddenFolders, SIGNAL( toggled( bool ) ),
             m_folderModel, SLOT( setHiddenFoldersShown( bool ) ) );
}


Nepomuk::IndexFolderSelectionDialog::~IndexFolderSelectionDialog()
{
}


void Nepomuk::IndexFolderSelectionDialog::setFolders( const QStringList& includeDirs, const QStringList& exclude )
{
    m_folderModel->setFolders( includeDirs, exclude );

    // make sure we do not have a hidden folder to expand which would make QFileSystemModel crash
    // + it would be weird to have a hidden folder indexed but not shown
    if ( !m_checkShowHiddenFolders->isChecked() ) {
        foreach( const QString& dir, m_folderModel->includeFolders() + m_folderModel->excludeFolders() ) {
            if ( isDirHidden( dir ) ) {
                m_checkShowHiddenFolders->setChecked( true );
                break;
            }
        }
    }

    // make sure that the tree is expanded to show all selected items
    foreach( const QString& dir, m_folderModel->includeFolders() + m_folderModel->excludeFolders() ) {
        expandRecursively( m_folderModel->index( dir ), m_viewIndexFolders );
    }
}


void Nepomuk::IndexFolderSelectionDialog::setExcludeFilters( const QStringList& filters )
{
    m_editStrigiExcludeFilters->setItems( filters );
}


void Nepomuk::IndexFolderSelectionDialog::setIndexHiddenFolders( bool enable )
{
    m_checkShowHiddenFolders->setChecked( enable );
}


QStringList Nepomuk::IndexFolderSelectionDialog::includeFolders() const
{
    if ( !indexHiddenFolders() ) {
        return removeHiddenFolders( m_folderModel->includeFolders() );
    }
    else {
        return m_folderModel->includeFolders();
    }
}


QStringList Nepomuk::IndexFolderSelectionDialog::excludeFolders() const
{
    if ( !indexHiddenFolders() ) {
        return removeHiddenFolders( m_folderModel->excludeFolders() );
    }
    else {
        return m_folderModel->excludeFolders();
    }
}


QStringList Nepomuk::IndexFolderSelectionDialog::excludeFilters() const
{
    return m_editStrigiExcludeFilters->items();
}


bool Nepomuk::IndexFolderSelectionDialog::indexHiddenFolders() const
{
    return m_checkShowHiddenFolders->isChecked();
}

#include "indexfolderselectiondialog.moc"
