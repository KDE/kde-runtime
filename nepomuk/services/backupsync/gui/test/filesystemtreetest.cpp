/*
 *  <one line to give the program's name and a brief idea of what it does.>
 *  Copyright (C) 2010  Vishesh Handa
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "filesystemtreetest.h"
#include "../filesystemtree.h"
#include "nie.h"
#include "nfo.h"

#include <Soprano/Soprano>

#include <kdebug.h>
#include <ktemporaryfile.h>
#include <qtest_kde.h>

using namespace Nepomuk;
using namespace Soprano;

namespace {
    int resNo = 0;
    
    FileSystemTreeItem * createFolder( const QString & url ) {
        Node subject( QUrl( QString("nepomuk:/res/ResourceNum") + QString::number( resNo++ ) ) );
        Statement st1( subject, Nepomuk::Vocabulary::NIE::url(), QUrl(url) );
        Statement st2( subject, Soprano::Vocabulary::RDF::type(), Nepomuk::Vocabulary::NFO::Folder() );
        Statement st3( subject, Soprano::Vocabulary::RDF::type(), Nepomuk::Vocabulary::NFO::FileDataObject() );
        return new FileSystemTreeItem( QList<Statement>() << st1 << st2 << st3 );
    }

    FileSystemTreeItem * createFile( const QString & url ) {
        Node subject( QUrl( QString("nepomuk:/res/ResourceNum") + QString::number( resNo++ ) ) );
        Statement st1( subject, Nepomuk::Vocabulary::NIE::url(), QUrl(url) );
        Statement st2( subject, Soprano::Vocabulary::RDF::type(), Nepomuk::Vocabulary::NFO::FileDataObject() );

        return new FileSystemTreeItem( QList<Statement>() << st1 << st2 );
    }
}


void FileSystemTreeTest::test()
{
    FileSystemTree tree;

    //
    // Crash testing.
    // If it doesn't crash. All is good.
    //
    
    // Test1 : Add Folder and add sub-files and sub-folders
    tree.add( createFolder("/home/user/Nepomuk/Folder1") );
    //kDebug() << tree.toList();
    
    tree.add( createFile("/home/user/Nepomuk/Folder1/File1") );
    //kDebug() << tree.toList();
    tree.add( createFile("/home/user/Nepomuk/Folder1/File2") );
    //kDebug() << tree.toList();
    kDebug();
    kDebug();
    kDebug() <<  "LIST: " << tree.toList();
    kDebug();

    
    tree.add( createFile("/home/user/Nepomuk/Folder1/File0") );
    //kDebug() << tree.toList();
    kDebug();
    kDebug();
    kDebug() <<  "LIST: " << tree.toList();
    kDebug();
    tree.add( createFolder("/home/user/Nepomuk/Folder1/SubFolder") );
    tree.add( createFile("/home/user/Nepomuk/Folder1/SubFolder/File") );
    tree.add( createFile("/home/user/Nepomuk/Folder1/NewFolder/wackyfile") );

    // Test 2 : Insert root of Folder1
    tree.add( createFolder("/home/user/Nepomuk/") ); //the trailing slash should not affect it

    tree.add( createFolder("/home/user/Nepomuk/Folder0") );
    tree.add( createFile("/home/user/Nepomuk/Folder0/File2") );
    tree.add( createFile("/home/user/Nepomuk/Folder0/File1") );
    tree.add( createFile("/home/user/Nepomuk/Folder0/File0") );
    
    // Test 3 : Add Sibbling of /home/user/Nepomuk so that there is an empty root
    tree.add( createFolder("/home/user/Trig") );
    tree.add( createFile("/home/user/Trig/File0") );

    // Test 4 : Add root
    tree.add( createFolder("/home/")  );

    // Test 5 : Add a folder which is between root and others
    tree.add( createFolder("/home/user/")  );

    kDebug() << "Hmm.. nothing has crashed";
    kDebug() << tree.toList();

    tree.remove("/home/user/Nepomuk");
    kDebug() << tree.toList();
    tree.remove("/home/user/Trig/File0");

    tree.remove("/home/");
    kDebug() << tree.toList();
}


QTEST_KDEMAIN(FileSystemTreeTest, NoGUI)

#include "filesystemtreetest.moc"
