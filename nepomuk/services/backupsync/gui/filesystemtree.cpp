/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "filesystemtree.h"

#include <QtCore/QListIterator>

//
// FileSystemTree
//

FileSystemTree::FileSystemTree()
{
}


FileSystemTree::~FileSystemTree()
{
    foreach( FileSystemTreeItem * p, m_rootNodes ) {
        delete p;
    }
}


void FileSystemTree::add( FileSystemTreeItem* item )
{
    FileSystemTreeItem::insert( m_rootNodes, item, 0 );
}


void FileSystemTree::remove(FileSystemTreeItem* item)
{
    if( item->parent() == 0 ) {
        kDebug() << "Removing one of the root nodes";
        m_rootNodes.removeAll( item );
        
        foreach( FileSystemTreeItem * child, item->m_children )
            FileSystemTreeItem::insert( m_rootNodes, child, 0 );

        item->m_children.clear();
        delete item;
    }
    else {
        kDebug() << "Removing .. " << item->url();
        kDebug() << "Other ..";

        item->m_parent->m_children.removeAll( item );

        foreach( FileSystemTreeItem * child, item->m_children )
                item->m_parent->add( child );

        item->m_children.clear();
        kDebug() << "Deleting item";
        delete item;
    }
}

void FileSystemTree::remove(const QString& url)
{
    FileSystemTreeItem * t = find( url );
    if( t ) {
        remove( t );
    }
}


FileSystemTreeItem* FileSystemTree::find(const QString& url)
{
    return FileSystemTreeItem::find( m_rootNodes, url );
}


QList<QString> FileSystemTree::toList() const
{
    QList<QString> list;
    foreach( FileSystemTreeItem * item, m_rootNodes ) {
        kDebug() << item->url();
        list << item->toList();
    }
    
    return list;
}

bool FileSystemTree::isEmpty() const
{
    return m_rootNodes.isEmpty();
}

QList< FileSystemTreeItem* > FileSystemTree::rootNodes() const
{
    return m_rootNodes;
}

int FileSystemTree::size() const
{
    int s = 0;
    foreach( const FileSystemTreeItem * item, m_rootNodes )
        s += item->size();
    return s;
}


//
// Tree Item
//

FileSystemTreeItem::FileSystemTreeItem()
{
    m_parent = 0;
}


FileSystemTreeItem::FileSystemTreeItem(const QString& url)
{
    m_parent = 0;
    m_url = url;
    m_isFolder = m_url.endsWith('/');
}


FileSystemTreeItem::FileSystemTreeItem(const QString& url, bool folder)
{
    m_parent = 0;
    m_url = url;
    m_isFolder = folder;
}


FileSystemTreeItem::~FileSystemTreeItem()
{
    kDebug();
    foreach( FileSystemTreeItem * t, m_children ) {
        kDebug() << "Deleting child : " << t->url();
        delete t;
    }
}

bool FileSystemTreeItem::shouldBeParentOf(FileSystemTreeItem* item)
{
    bool r = item->depth() > depth() && item->m_url.contains( m_url );
    //kDebug() << url() << " should be parent of " << item->url() << " ? " << r;
    //kDebug() << url() << " depth : "<< depth();
    //kDebug() << item->url() << " depth : " << item->depth();
    return r;
}

bool FileSystemTreeItem::isFile() const
{
    return !isFolder();
}

namespace {
    bool withoutTrailingSlashCompare( const QString & a, const QString & b ) {
        if( a.endsWith('/') ) {
            if( b.endsWith('/') )
                return a < b;
            
            return a.mid( 0, a.length() -1 ) < b;
        }
        else if( b.endsWith('/') ) {
            return a < b.mid( 0, b.length() -1 );
        }
        else
            return a < b;
    }


    bool withoutTrailingSlashEquality( const QString & a, const QString & b ) {
        if( a.endsWith('/') ) {
            if( b.endsWith('/') )
                return a == b;

            return a.mid( 0, a.length() -1 ) == b;
        }
        else if( b.endsWith('/') ) {
            return a == b.mid( 0, b.length() -1 );
        }
        else
            return a == b;
    }
}

// static
void FileSystemTreeItem::insert(QList< FileSystemTreeItem* >& list, FileSystemTreeItem* item, FileSystemTreeItem* parent)
{
    if( list.isEmpty() ) {
        kDebug() << "Empty list - appending";
        list.append( item );
        item->m_parent = parent;
        return;
    }
    
    // Step 1 : Check for duplicates
    foreach( FileSystemTreeItem *p, list ) {
        if( withoutTrailingSlashEquality( p->m_url, item->m_url ) ) {
            kDebug() << "Duplicate ..";
            return;
        }
    }
    
    // Step 2 : Check if it should be inserted in this list
    //          If so, call for that FileSystemTreeItem *
    foreach( FileSystemTreeItem * p, list ) {
        if( p->isFolder() && p->shouldBeParentOf( item ) ) {
            kDebug() << "Calling insert on " << p->url();
            insert( p->m_children, item, p );
            return;
        }
    }

    // Step 3 : Insert in the correct position

    kDebug() << "Inserting " << item->url() << " over here somewhere";
    QMutableListIterator<FileSystemTreeItem* > iter( list );
    while( iter.hasNext() ) {
        FileSystemTreeItem * p = iter.next();
        
        if( withoutTrailingSlashCompare( item->m_url, p->m_url ) ) {
            kDebug() << "Inserting before " << p->url();
            iter.previous();
            // Insert it and set it's parent
            iter.insert( item );
            item->m_parent = parent;
            kDebug() << "DONE!!";
            if( parent )
                kDebug() << parent->toList();
            break;
        }
    }

    // Insert in the end if proper position not found
    if( !iter.hasNext() ) {
        list.append( item );
        item->m_parent = parent;
        return;
    }
    
    if( item->isFile() ) {
        kDebug() << "Is file .. nothing more to do";
        return;
    }

    // Insert all the files which are in the list and should be under item
    kDebug() << "Checking remaining files/folders..";
    while( iter.hasNext() ) {
        FileSystemTreeItem * p = iter.next();
        kDebug() << "Testing " << p->url();
        if( /*p->isFile() &&*/ item->shouldBeParentOf( p ) ) {
            kDebug() << p->url() << " is now going to be child of " << item->url();
            iter.remove();
            insert( item->m_children, p, item );
        }
    }
}

FileSystemTreeItem* FileSystemTreeItem::find(QList< FileSystemTreeItem* >& list, const QString& url)
{
    foreach( FileSystemTreeItem * item, list ) {
        if( withoutTrailingSlashEquality( item->url(), url ) )
            return item;
    }

    foreach( FileSystemTreeItem * t, list ) {
        if( url.contains( t->m_url ) )
            return t->find( url );
    }
    
    return 0;
}


FileSystemTreeItem* FileSystemTreeItem::child(int row)
{
    return m_children.at( row );
}



int FileSystemTreeItem::numChildren() const
{
    return m_children.size();
}



FileSystemTreeItem* FileSystemTreeItem::parent() const
{
    return m_parent;
}



int FileSystemTreeItem::parentRowNum()
{
    if( m_parent ) {
        return m_parent->m_children.indexOf( this );
    }

    kDebug() << "RETURNING -1";
    return -1;
}


int FileSystemTreeItem::size() const
{
    if( m_children.isEmpty() )
        return 1;
    
    int s = 1; // For self
    foreach( const FileSystemTreeItem * child, m_children )
        s += child->size();
    return s;
}



QString FileSystemTreeItem::url() const
{
    return m_url;
}


void FileSystemTreeItem::add(FileSystemTreeItem* item)
{
    if( withoutTrailingSlashEquality( item->url(), url() ) )
        return;
    
    insert( m_children, item, this ); 
}


FileSystemTreeItem* FileSystemTreeItem::find(const QString& url)
{
    if( withoutTrailingSlashEquality( m_url, url ) )
        return this;

    return find( m_children, url );
}


QList<QString> FileSystemTreeItem::toList() const
{
    if( numChildren() == 0 && !m_url.isEmpty() ) {
        return QList<QString>() << m_url;
    }
    
    QList<QString> urls;
    if( !m_url.isEmpty() )
        urls << m_url;
    
    kDebug() << m_url << " children : ";
    foreach( FileSystemTreeItem * item, m_children )
        urls << item->toList();
    
    return urls;
}



bool FileSystemTreeItem::isFolder() const
{
    return m_isFolder;
}


QList<FileSystemTreeItem* > FileSystemTreeItem::children() const
{
    return m_children;
}


bool FileSystemTreeItem::isNull() const
{
    return m_url.isEmpty() && numChildren() == 0;
}

QString FileSystemTreeItem::prettyString() const
{
//     if( !m_parent )
//         return m_url.toLocalFile();
//     
//     QString parentString = m_parent->m_url.toLocalFile();
//     QString ownString = m_url.toLocalFile();
//     
//     ownString.replace( parentString, "" );
//     return ownString;
    return QString();
}

int FileSystemTreeItem::depth()
{
    int d = m_url.count('/');
    if( isFolder() )
        return d-1;
    else
        return d;
}
