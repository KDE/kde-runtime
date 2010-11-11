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


#ifndef FILESYSTEMTREE_H
#define FILESYSTEMTREE_H

#include <QtCore/QString>
#include <QtCore/QUrl>
#include <KDebug>

template<typename Data>
class FileSystemTreeItem;

template<typename Data>
class FileSystemTree
{
public :
    FileSystemTree();
    virtual ~FileSystemTree();
    
    virtual void add( FileSystemTreeItem<Data> * item );
    virtual void remove( const QString& url );
    
    FileSystemTreeItem<Data> * root() const;
    FileSystemTreeItem<Data> * find( const QString & url );
    
    /**
     * Returns a list of all the files/folders arranged properly.
     */
    QList<QUrl> toList() const;
    
private :
    friend class FileSystemTreeItem<Data>;
    FileSystemTreeItem<Data> * m_root;
    
};


template<typename Data>
class FileSystemTreeItem {
    
public:
    FileSystemTreeItem();
    /**
     * Gets the resource uri, and the nie:url from the list of statements
     */
    FileSystemTreeItem( const QUrl & url, bool folder, const Data & data );
    
    /**
     * Normal destructor. Deletes all the children as well.
     */
    virtual ~FileSystemTreeItem();
    
    
    /**
     * Inserts \p child into the children of this, and sets
     * the child's parent to this
     */
    void insert( FileSystemTreeItem<Data>* child );
    
    /**
     * Return the TreeItem which has it's nie:url equal to \p nieUrl
     * It searches the entire tree
     */
    FileSystemTreeItem<Data>* find( const QString& url );
    
    /**
     * Remove this from it's parents children list, and resets it's parent
     * Clears all other data as well
     */
    void remove();
    
    /**
     * Returns the row number of the current TreeItem in it's parents children list.
     * Returns 0 if it doesn't have a parent
     */
    int parentRowNum();
    
    /**
     * Returns the number of children.
     */
    int numChildren() const;
    
    /**
     * Returns the child at row number \p row
     */
    FileSystemTreeItem<Data>* child(int row);
    
    /**
     * Returns the url
     */
    QUrl url() const;
    
    
    FileSystemTreeItem<Data> * parent() const;
    
    /**
     * Based on the nieurl it determines the correct position
     * and inserts item.
     * May cause major restructuring of the tree.
     * Should be called from the root node.
     */
    void add( FileSystemTreeItem<Data> * item );
    
    QList<QUrl> toList() const;
    
    QList<FileSystemTreeItem<Data>*> children() const;
    
    bool isEmpty() const;
    
    bool isFolder() const;

    const Data& data() const;
    Data & data();

    QString prettyString() const;
private :
    
    /**
     * Returns the depth of the nieUrl. Used to arrange the tree structure
     */
    int depth() const;
    
    // Tree Handling
    QList<FileSystemTreeItem<Data> *> m_children;
    FileSystemTreeItem<Data> * m_parent;
    bool m_isFolder;
    
    // Internal data
    QUrl m_url;
    Data m_data;
    
    /**
     * Return true if \p item should be a child based on filesystem ordering
     * Based on the TreeItem's nie:url
     */
    bool contains( FileSystemTreeItem<Data> * item );
    
    friend class FileSystemTree<Data>;
};



//
// FileSystemTree
//
template<typename Data>
FileSystemTree<Data>::FileSystemTree()
{
    //kDebug();
    m_root = new FileSystemTreeItem<Data>;
    //kDebug() << m_root;
}


template<typename Data>
FileSystemTree<Data>::~FileSystemTree()
{
    delete m_root;
}


template<typename Data>
void FileSystemTree<Data>::add( FileSystemTreeItem<Data>* item )
{
    // If root is empty
    if( m_root->url().isEmpty() ) {
        
        if( m_root->numChildren() == 0 ) {
            m_root = item;
            return;
        }
        
        // If all the children have a depth >= item's depth
        bool shouldBecomeRoot = true;
        foreach( FileSystemTreeItem<Data> * child, m_root->m_children ) {
            if( child->depth() <= item->depth() /*&& item->contains( child )*/ ) {
                shouldBecomeRoot = false;
                break;
            }
        }
        
        if( shouldBecomeRoot ) {
            m_root->m_url = item->m_url;
            m_root->m_data = item->m_data;
            delete item;
            return;
        }
        
        // Become root's child
        m_root->add( item );
    }
    else {
        // item equal to root
        if( m_root->url() == item->url() )
            return;
        
        // item should be the new root
        if( item->depth() < m_root->depth() /*&& item->contains( m_root )*/ ) {
            FileSystemTreeItem<Data> * oldRoot = m_root;
            m_root = item;
            m_root->insert( oldRoot );
            return;
        }

        // New blank root required
        if( item->depth() == m_root->depth() ) {
            FileSystemTreeItem<Data> * oldRoot = m_root;
            m_root = new FileSystemTreeItem<Data>();
            m_root->insert( oldRoot );
            m_root->insert( item );
            return;
        }

        // Become root's child
        //kDebug() << "Become root's child ..";
        m_root->add( item );
    }
}


template<typename Data>
void FileSystemTree<Data>::remove(const QString& url)
{
    FileSystemTreeItem<Data> * t = m_root->find( url );
    if( t ) {
        kDebug() << "Found : " << t->url();
        t->remove();
        if( t != m_root )
            delete t;
    }
}

template<typename Data>
FileSystemTreeItem< Data >* FileSystemTree<Data>::find(const QString& url)
{
    return m_root->find( url );
}


template<typename Data>
FileSystemTreeItem<Data>* FileSystemTree<Data>::root() const
{
    return m_root;
}

template<typename Data>
QList<QUrl> FileSystemTree<Data>::toList() const
{
    kDebug() << "Root: " << m_root->url();
    foreach( FileSystemTreeItem<Data> * child, m_root->m_children )
        kDebug() << child->url();
    
    return m_root->toList();
}

//
// Tree Item
//

template<typename Data>
FileSystemTreeItem<Data>::FileSystemTreeItem()
{
    m_parent = 0;
}

template<typename Data>
FileSystemTreeItem<Data>::FileSystemTreeItem(const QUrl& url, bool folder, const Data& data)
{
    m_parent = 0;
    m_url = url;
    m_data = data;
    m_isFolder = folder;

    // Make sure al the urls end with a trailing '/'
    if( !m_url.toString().endsWith('/') )
        m_url = QUrl( m_url.toString() + '/' );
    if( m_url.scheme().isEmpty() )
        m_url.setScheme("file");
}

template<typename Data>
FileSystemTreeItem<Data>::~FileSystemTreeItem()
{
    foreach( FileSystemTreeItem<Data> * t, m_children ) {
        delete t;
    }
}


template<typename Data>
FileSystemTreeItem<Data>* FileSystemTreeItem<Data>::child(int row)
{
    return m_children.at( row );
}


template<typename Data>
int FileSystemTreeItem<Data>::numChildren() const
{
    return m_children.size();
}


template<typename Data>
FileSystemTreeItem<Data>* FileSystemTreeItem<Data>::parent() const
{
    return m_parent;
}


template<typename Data>
int FileSystemTreeItem<Data>::parentRowNum()
{
    if( m_parent ) {
        return m_parent->m_children.indexOf( this );
    }
    return -1;
}


template<typename Data>
QUrl FileSystemTreeItem<Data>::url() const
{
    return m_url;
}


template<typename Data>
int FileSystemTreeItem<Data>::depth() const
{
    return m_url.toString().count( '/' );
}


template<typename Data>
bool FileSystemTreeItem<Data>::contains(FileSystemTreeItem<Data>* t)
{
    return m_url.toString().contains( t->m_url.toString() );
}

template<typename Data>
void FileSystemTreeItem<Data>::add(FileSystemTreeItem<Data>* item)
{
    if( !isFolder() ) {
        return;
    }
    
    //
    // Check if it already exists
    //
    if( m_url == item->m_url )
        return;
    
    foreach( FileSystemTreeItem<Data> * t, m_children )
        if( t->m_url == item->m_url )
            return;
        
    //
    // Check if it should be inserted in the current node
    //
    bool shouldBeHere = true;
    foreach( FileSystemTreeItem<Data> * t, m_children ) {
        if( t->depth() < item->depth() ) {
            shouldBeHere = false;
            break;
        }
    }

    // If this is the correct position
    if( shouldBeHere ) {
        // Otherwise find all the TreeItem in m_children who should be item's children
        // and relocate them
        QMutableListIterator<FileSystemTreeItem<Data> *> it( m_children );
        while( it.hasNext() ) {
            FileSystemTreeItem<Data> * c = it.next();

            if( c->contains( item ) ) {
                // Relocate c
                it.remove();
                item->insert( c );
            }
        }
        insert( item );
        return;
    }

    // Find where item should be, and add it
    foreach( FileSystemTreeItem<Data> * t, m_children ) {
        if( item->contains( t ) ) {
            t->add( item );
            return;
        }
    }

    // Nothing needs to be done, just add it
    insert( item );
}

template<typename Data>
void FileSystemTreeItem<Data>::insert(FileSystemTreeItem<Data>* child)
{
    //kDebug() << "INSERTING " << child->nieUrl() << " with Parent " << nieUrl();
    child->m_parent = this;
    
    // Insert at the correct position so that the list is sorted
    if( m_children.empty() ) {
        m_children.append( child );
        return;
    }
    
    for( int i = m_children.size()-1; i >= 0; i-- ) {
        FileSystemTreeItem<Data> * t = m_children[ i ];
        if( t->m_url.toString() < child->m_url.toString() ) {
            m_children.insert( i+1, child );
            return;
        }
    }
    
    //kDebug() << "Needs to go to the front ..";
    m_children.push_front( child );
}

template<typename Data>
void FileSystemTreeItem<Data>::remove()
{
    // If you are NOT the root node
    if( m_parent ) {
        // Give your children to your parent
        foreach( FileSystemTreeItem<Data> * child, m_children )
            m_parent->insert( child );
        
        // Get disowned
        m_parent->m_children.removeAll( this );
        m_parent = 0;
        m_children.clear();
    }
    
    // Clear the interal data
    m_url.clear();
    m_data = Data();
}

template<typename Data>
FileSystemTreeItem<Data>* FileSystemTreeItem<Data>::find(const QString & urlString)
{
    QString stringWithSlash = urlString;
    if( !urlString.endsWith('/') )
        stringWithSlash += '/';
    
    QUrl url( stringWithSlash );
    if( url.scheme().isEmpty() )
        url.setScheme("file");
    
    //If it's you
    if( m_url == url )
        return this;
    
    //kDebug() << m_url << " ......." << nieUrl;
    // If it's your one of your children
    foreach( FileSystemTreeItem<Data> * t, m_children ) {
        if( t->m_url == url ) {
            //kDebug() <<"Found!";
            return t;
        }
    }

    // Whose child is it?
    //kDebug() << "Whose child is it?";
    foreach( FileSystemTreeItem<Data> * t, m_children ) {
        //kDebug() << t->nieUrl();
        if( url.toString().contains( t->m_url.toString() ) )
            return t->find( urlString );
    }

    // Not found
    return 0;
}


template<typename Data>
QList<QUrl> FileSystemTreeItem<Data>::toList() const
{
    if( numChildren() == 0 && !m_url.isEmpty() ) {
        return QList<QUrl>() << m_url;
    }
    
    QList<QUrl> urls;
    if( !m_url.isEmpty() )
        urls << m_url;

    kDebug() << m_url << " children : ";
    foreach( FileSystemTreeItem<Data> * item, m_children )
        urls << item->toList();
    
    return urls;
}


template<typename Data>
bool FileSystemTreeItem<Data>::isFolder() const
{
    return m_url.isEmpty() || m_isFolder;
}

template<typename Data>
QList<FileSystemTreeItem<Data>* > FileSystemTreeItem<Data>::children() const
{
    return m_children;
}

template<typename Data>
bool FileSystemTreeItem<Data>::isEmpty() const
{
    return m_url.isEmpty() && numChildren() == 0;
}

template<typename Data>
const Data & FileSystemTreeItem<Data>::data() const
{
    return m_data;
}

template<typename Data>
Data& FileSystemTreeItem<Data>::data()
{
    return m_data;
}

template<typename Data>
QString FileSystemTreeItem<Data>::prettyString() const
{
    if( !m_parent )
        return m_url.toLocalFile();

    QString parentString = m_parent->m_url.toLocalFile();
    QString ownString = m_url.toLocalFile();

    ownString.replace( parentString, "" );
    return ownString;
}


#endif // FILESYSTEMTREE_H
