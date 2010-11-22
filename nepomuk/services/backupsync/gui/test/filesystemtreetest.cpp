/*
   Copyright (C) 2010 by Vishesh Handa <handa.vish@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "filesystemtreetest.h"

#include <KTempDir>
#include <KRandom>
#include <KStandardDirs>
#include <qtest_kde.h>

#include <QtTest>
#include <QTextStream>
#include <QFile>
#include <QSignalSpy>
#include <QEventLoop>
#include <QTimer>
#include <QDir>

void FileSystemTreeTest::testAdditions()
{
    FileSystemTree tree;
    FileSystemTreeItem * item, *p;

    tree.add( new FileSystemTreeItem( "/a/b/c" ) );
    QVERIFY( tree.toList() == QList<QString>() << "/a/b/c" );
   
    tree.add( new FileSystemTreeItem( "/a/b/" ) );
    QVERIFY( tree.toList() == QList<QString>() << "/a/b/" << "/a/b/c" );
    item = tree.find( "/a/b/c" );
    QVERIFY( item != 0 );
    p = tree.find( "/a/b/" );
    QVERIFY( p != 0 );
    QVERIFY( item->parent() == p );
    
    tree.add( new FileSystemTreeItem( "/a/" ) );
    QVERIFY( tree.toList() == QList<QString>() << "/a/" << "/a/b/" << "/a/b/c" );
    
    tree.add( new FileSystemTreeItem( "/a/ba" ) );
    QVERIFY( tree.toList() == QList<QString>() << "/a/" << "/a/b/" << "/a/b/c" << "/a/ba" );
    
    tree.add( new FileSystemTreeItem( "/a/bb" ) );
    QVERIFY( tree.toList() == QList<QString>() << "/a/" << "/a/b/" << "/a/b/c" << "/a/ba" << "/a/bb" );
    
    tree.add( new FileSystemTreeItem( "/a/b/d" ) );
    QVERIFY( tree.toList() == QList<QString>() << "/a/" << "/a/b/" << "/a/b/c" << "/a/b/d" << "/a/ba" << "/a/bb" );

    int size = tree.size();
    tree.add( new FileSystemTreeItem("/a/b/d") );
    QVERIFY( tree.size() == size );

    
    tree.add( new FileSystemTreeItem( "/b" ) );
    QVERIFY( tree.toList() == QList<QString>() << "/a/" << "/a/b/" << "/a/b/c" << "/a/b/d" << "/a/ba" << "/a/bb" << "/b" );

    kDebug() << tree.toList();
    
    // Find tests
    item = tree.find("/a/b/d");
    QVERIFY( item != 0 );
    QVERIFY( item->url() == "/a/b/d" );
    kDebug() << "?";

    size = tree.size();
    tree.remove("/a/b");

    // Check all pointers
    checkPointers( tree );
    
    kDebug() << "Old : " << size << " New : "<< tree.size();
    QVERIFY( size == (tree.size() +1) );

    kDebug() << "To list..";
    kDebug() << "LIST :::: " <<  tree.toList();
    QVERIFY( tree.toList() == QList<QString>() << "/a/" << "/a/b/c" << "/a/b/d" << "/a/ba" << "/a/bb" << "/b" );

    item = tree.find("/a/b/c");
    QVERIFY( item != 0 );
    FileSystemTreeItem * parent = tree.find("/a/");
    QVERIFY( parent != 0 );
    QVERIFY( item->parent() == parent );

    size = tree.size();
    tree.remove( "/a" );
    QVERIFY( size == tree.size() + 1 );
    QVERIFY( tree.toList() == QList<QString>() << "/a/b/c" << "/a/b/d" << "/a/ba" << "/a/bb" << "/b" );
}

void FileSystemTreeTest::checkPointers(const FileSystemTree& tree)
{
    foreach( FileSystemTreeItem * p, tree.rootNodes() ) {
        checkAll( p, 0 );
    }
}

void FileSystemTreeTest::check( FileSystemTreeItem* child, FileSystemTreeItem* parent)
{
    kDebug() << "Checking with child ";
    kDebug() << child->url();
    kDebug() << " and parent ";
    if( parent )
        kDebug() << parent->url();
    else
        kDebug() << " NULL";
    
    if( child )
        QVERIFY( child->parent() == parent );
    if( parent )
        QVERIFY( parent->children().indexOf( child ) != -1 );
}

void FileSystemTreeTest::checkAll( FileSystemTreeItem* c, FileSystemTreeItem* p)
{
    check( c, p );
    foreach( FileSystemTreeItem * ch, c->children() ) {
        check( ch, c );
    }
}


QTEST_KDEMAIN_CORE(FileSystemTreeTest)
