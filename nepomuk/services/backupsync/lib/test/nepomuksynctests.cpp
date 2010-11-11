/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "nepomuksynctests.h"

#include <nepomuk/resourceidentifier.h>
#include <nepomuk/resourcemerger.h>

#include <kdebug.h>
#include <ktemporaryfile.h>
#include <qtest_kde.h>

#include <KTempDir>
#include <KTemporaryFile>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Tag>
#include <Nepomuk/Variant>

#include <Soprano/Model>
#include <Soprano/StatementIterator>
#include <Soprano/Graph>
#include <Soprano/NodeIterator>

void NepomukSyncTests::basicIdentification()
{
    Soprano::Model * model = Nepomuk::ResourceManager::instance()->mainModel();
    model->removeAllStatements();

    KTemporaryFile file1;
    KTemporaryFile file2;

    file1.open();
    file2.open();

    Nepomuk::Resource res1( file1.fileName() );
    res1.setRating( 5 );

    Nepomuk::Resource res2( file2.fileName() );
    res2.addTag( Nepomuk::Tag("test-tag") );

    QList<Soprano::Statement> list = model->listStatements().allStatements();
    kDebug() << "SIZE: " << list.size();
    
    //
    // Pass them to the ResourceIdentifier
    //
    Nepomuk::Sync::ResourceIdentifier identifier;
    identifier.addStatements( list );
    identifier.identifyAll();

    kDebug() << "Unidentified: " << identifier.unidentified().size();
    QVERIFY( identifier.unidentified().isEmpty() );

    // The mapped uris should be same the supplied uris
    foreach( const KUrl & uri, identifier.mappedUris() ) {
        QVERIFY( uri == identifier.mappedUri( uri ) );
    }

    model->removeAllStatements();
}


void NepomukSyncTests::resourceMergerTests()
{
    QString resString("nepomuktest:/res/");
    QString propString("nepomuktest:/prop/");
    QString objString("nepomuktest:/obj/");
    
    Soprano::Model * model = Nepomuk::ResourceManager::instance()->mainModel();
    Soprano::Graph graph;
    
    QUrl res1uri( resString + '1' );
    for( int i=0; i<10; i++ ) {
        graph.addStatement( res1uri, QUrl( propString + ( i + '0' ) ),
                            Soprano::Node( Soprano::LiteralValue(i) ) );
    }
    graph.addStatement( res1uri, QUrl( propString + QString::number(10) ),
                        QUrl( objString + QString::number(10) ) );
    
    KTemporaryFile file;
    file.open();
    
    Nepomuk::Resource res1( file.fileName() );
    res1.addProperty( QUrl( propString + '5' ), Nepomuk::Variant( 5 ) );
    
    Soprano::Node context = model->listStatements( res1.resourceUri(), QUrl( propString + '5' ), Soprano::Node( Soprano::LiteralValue( 5 ) ) ).iterateContexts().allElements().first();
    
    QHash<KUrl, Nepomuk::Resource> hash;
    hash.insert( res1uri, res1 );
    
    Nepomuk::Sync::ResourceMerger merger;
    merger.merge( graph, hash );
    
    // Check if res1uri was mapped to res1 and all its properties were added.
    for( int i=0; i<10; i++ ) {
        QVERIFY( model->containsAnyStatement( res1.resourceUri(), QUrl( propString + ( i + '0' ) ), Soprano::Node() ) );
    }
    
    // Make sure that the already present statements were not messed with
    Soprano::Node context2 = model->listStatements( res1.resourceUri(), QUrl( propString + '5' ), Soprano::Node( Soprano::LiteralValue( 5 ) ) ).iterateContexts().allElements().first();
    
    QVERIFY( context == context2 );
    
    // Property10 did not have a valid mapping for its object. A new Resource should
    // have been created
    Soprano::Node newResNode = model->listStatements( res1.resourceUri(), QUrl( propString + "10" ), Soprano::Node() ).iterateObjects().allNodes().first();

    kDebug() << newResNode.uri();
    QVERIFY( !newResNode.uri().isEmpty() );
    
    model->removeAllStatements();
}


QTEST_KDEMAIN(NepomukSyncTests, NoGUI)