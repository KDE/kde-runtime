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
#include "../filesystemtree.h"

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

    tree.add( new FileSystemTreeItem( "/a/b/c" ) );
    QVERIFY( tree.toList() == QList<QString>() << "/a/b/c" );
    
    tree.add( new FileSystemTreeItem( "/a/b/" ) );
    QVERIFY( tree.toList() == QList<QString>() << "/a/b/" << "/a/b/c" );
    
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
}


QTEST_KDEMAIN_CORE(FileSystemTreeTest)
