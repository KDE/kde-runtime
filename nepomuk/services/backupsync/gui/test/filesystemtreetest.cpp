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
    kDebug() << tree.toList();
    
    tree.add( new FileSystemTreeItem( "/a/b/" ) );
    kDebug() << tree.toList();
    
    tree.add( new FileSystemTreeItem( "/a/" ) );
    kDebug() << tree.toList();
    
    tree.add( new FileSystemTreeItem( "/a/ba" ) );
    kDebug() << tree.toList();
    
    tree.add( new FileSystemTreeItem( "/a/bb" ) );
    kDebug() << tree.toList();
    
    tree.add( new FileSystemTreeItem( "/a/b/d" ) );
    kDebug() << tree.toList();

    tree.add( new FileSystemTreeItem( "/b" ) );
    kDebug() << tree.toList();

    tree.add( new FileSystemTreeItem("/a/b/d") );
    kDebug() << tree.toList();
}


QTEST_KDEMAIN_CORE(FileSystemTreeTest)
