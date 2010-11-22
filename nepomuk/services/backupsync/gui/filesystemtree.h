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

class FileSystemTreeItem;

class FileSystemTree
{
public :
    FileSystemTree();
    virtual ~FileSystemTree();
    
    virtual void add( FileSystemTreeItem * item );
    virtual void remove( const QString& url );
    
    FileSystemTreeItem * find( const QString & url );

    QList<FileSystemTreeItem*> rootNodes() const;
    
    /**
     * Returns a list of all the files/folders arranged properly.
     */
    QList<QString> toList() const;

    bool isEmpty() const;
    
private :
    friend class FileSystemTreeItem;

    QList<FileSystemTreeItem *> m_rootNodes;
};


class FileSystemTreeItem {
    
public:
    FileSystemTreeItem();
    FileSystemTreeItem( const QString & url );
    FileSystemTreeItem( const QString & url, bool folder );

    virtual ~FileSystemTreeItem();

    /**
     * Inserts \p child into the children of this, and sets
     * the child's parent to this.
     */
    static void insert(QList<FileSystemTreeItem*>& list, FileSystemTreeItem* item, FileSystemTreeItem * parent );
    static FileSystemTreeItem* find( QList<FileSystemTreeItem*>& list, const QString & url );
    
    /**
     * Return the TreeItem which matches the given url
     */
    //FileSystemTreeItem* find( const QString& url );
    
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
    FileSystemTreeItem* child(int row);
    
    /**
     * Returns the url
     */
    QString url() const;
    
    
    FileSystemTreeItem * parent() const;
    
    /**
     * Based on the nieurl it determines the correct position
     * and inserts item.
     * May cause major restructuring of the tree.
     * Should be called from the root node.
     */
    //void add( FileSystemTreeItem * item );
    
    QList< QString > toList() const;
    
    QList<FileSystemTreeItem*> children() const;
    
    bool isNull() const;
    
    bool isFile() const;
    bool isFolder() const;

    int depth();
    
    QString prettyString() const;
private :
        
    // Tree Handling
    QList<FileSystemTreeItem *> m_children;
    FileSystemTreeItem * m_parent;
    bool m_isFolder;
    
    // Internal data
    QString m_url;
    
    friend class FileSystemTree;

    
    bool shouldBeParentOf( FileSystemTreeItem * item );
};




#endif // FILESYSTEMTREE_H
