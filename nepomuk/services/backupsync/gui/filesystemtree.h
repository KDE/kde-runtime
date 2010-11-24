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
    virtual void remove( FileSystemTreeItem * item );
    
    FileSystemTreeItem * find( const QString & url );
    
    QList<FileSystemTreeItem*> rootNodes() const;
    
    QList<QString> toList() const;

    bool isEmpty() const;
    int size() const;
    
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

    void add( FileSystemTreeItem * item );

    FileSystemTreeItem* find( const QString& url );
    
    QList<FileSystemTreeItem*> children() const;
    FileSystemTreeItem* child(int row) const;
    int numChildren() const;
    
    /**
     * Returns the row number of the current TreeItem in it's parents children list.
     * Returns -1 if it doesn't have a parent
     */
    int parentRowNum();
    
    FileSystemTreeItem * parent() const;
    QString url() const;
   
    QList< QString > toList() const;

    /// Returns the number of nodes in this tree
    int size() const;
    
    bool isNull() const;
    bool isFile() const;
    bool isFolder() const;
private :
    
    QList<FileSystemTreeItem *> m_children;
    FileSystemTreeItem * m_parent;
    bool m_isFolder;
    
    QString m_url;
    
    friend class FileSystemTree;

    int depth();
    bool shouldBeParentOf( FileSystemTreeItem * item );

    static void insert(QList<FileSystemTreeItem*>& list,
                       FileSystemTreeItem* item, FileSystemTreeItem * parent );
    static FileSystemTreeItem* find( QList<FileSystemTreeItem*>& list, const QString & url );
};




#endif // FILESYSTEMTREE_H
