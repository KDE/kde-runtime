/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

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

#include "datamanagementmodeltest.h"
#include "../datamanagementmodel.h"
#include "../classandpropertytree.h"
#include "../simpleresource.h"

#include <QtTest>
#include "qtest_kde.h"

#include <Soprano/Soprano>

#include <ktempdir.h>
#include <KDebug>

#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NMM>
#include <Nepomuk/Vocabulary/NCO>
#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Variant>
#include <Nepomuk/ResourceManager>

using namespace Soprano;

void DataManagementModelTest::initTestCase()
{
    const Soprano::Backend* backend = Soprano::PluginManager::instance()->discoverBackendByName( "virtuosobackend" );
    QVERIFY( backend );
    m_storageDir = new KTempDir();
    m_model = backend->createModel( Soprano::BackendSettings() << Soprano::BackendSetting(Soprano::BackendOptionStorageDir, m_storageDir->name()) );
    QVERIFY( m_model );

    m_dmModel = new Nepomuk::DataManagementModel( m_model );
}

void DataManagementModelTest::cleanupTestCase()
{
    delete m_dmModel;
    delete m_model;
    delete m_storageDir;
}

void DataManagementModelTest::init()
{
    // remove all the junk from previous tests
    m_model->removeAllStatements();

    // add some classes and properties
    QUrl graph("graph:/onto");
    m_model->addStatement( graph, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::Ontology(), graph );

    m_model->addStatement( QUrl("prop:/A"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/A"), Soprano::Vocabulary::RDFS::range(), Soprano::Vocabulary::XMLSchema::string(), graph );


    // rebuild the internals of the data management model
    m_dmModel->updateTypeCachesAndSoOn();
}


void DataManagementModelTest::testSetProperty()
{
    // adding the most basic property
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/A"), QVariantList() << QVariant(QLatin1String("foobar")), QLatin1String("Testapp"));

    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/A"), LiteralValue(QLatin1String("foobar"))));

    QTextStream s(stderr);
    m_model->write(s);
}

namespace {
    int push( Soprano::Model * model, Nepomuk::SimpleResource res, QUrl graph ) {
        QHashIterator<QUrl, QVariant> it( res.m_properties );
        if( !res.uri().isValid() )
            return 0;

        int numPushed = 0;
        Soprano::Statement st( res.uri(), Soprano::Node(), Soprano::Node(), graph );
        while( it.hasNext() ) {
            it.next();
            st.setPredicate( it.key() );
            st.setObject( Nepomuk::Variant(it.value()).toNode() );
            if( model->addStatement( st ) == Soprano::Error::ErrorNone )
                numPushed++;
        }
        return numPushed;
    }
}
void DataManagementModelTest::testMergeResources()
{
    Nepomuk::ResourceManager::instance()->setOverrideMainModel( m_model );
    
    using namespace Soprano::Vocabulary;
    using namespace Nepomuk::Vocabulary;

    //
    // Test Identification
    //
    
    QUrl resUri("nepomuk:/mergeTest/res1");
    QUrl graphUri("nepomuk:/ctx:/TestGraph");

    int stCount = m_model->statementCount();
    m_model->addStatement( resUri, RDF::type(), NFO::FileDataObject(), graphUri );
    m_model->addStatement( resUri, QUrl("nepomuk:/mergeTest/prop1"),
                           Soprano::LiteralValue(10), graphUri );
    QVERIFY( stCount + 2 == m_model->statementCount() );

    Nepomuk::SimpleResource res;
    res.setUri( QUrl("nepomuk:/mergeTest/res2") );
    res.m_properties.insert( RDF::type(), NFO::FileDataObject() );
    res.m_properties.insert( QUrl("nepomuk:/mergeTest/prop1"), QVariant(10) );

    Nepomuk::SimpleResourceGraph graph;
    graph.append( res );

    // Try merging it.
    m_dmModel->mergeResources( graph, QLatin1String("Testapp") );
    
    // res2 shouldn't exists as it is the same as res1 and should have gotten merged.
    QVERIFY( !m_model->containsAnyStatement( QUrl("nepomuk:/mergeTest/res2"), QUrl(), QUrl() ) );

    // Try merging it as a blank uri
    res.setUri( QUrl("_:blah") );
    graph.clear();
    graph << res;

    m_dmModel->mergeResources( graph, QLatin1String("Testapp") );
    QVERIFY( !m_model->containsAnyStatement( QUrl("_:blah"), QUrl(), QUrl() ) );

    //
    // Test 2 -
    // This is for testing exactly how Strigi will use mergeResources ie
    // have some blank nodes ( some of which may already exists ) and a
    // main resources which does not exist
    //
    {
        kDebug() << "Starting Strigi merge test";

        QUrl graphUri("nepomuk:/ctx/afd"); // TODO: Actually Create one

        Nepomuk::SimpleResource coldplay;
        coldplay.m_properties.insert( RDF::type(), NCO::Contact() );
        coldplay.m_properties.insert( NCO::fullname(), "Coldplay" );

        // Push it into the model with a proper uri
        coldplay.setUri( QUrl("nepomuk:/res/coldplay") );
        QVERIFY( push( m_model, coldplay, graphUri ) == coldplay.m_properties.size() );

        // Now keep it as a blank node
        coldplay.setUri( QUrl("_:coldplay") );

        Nepomuk::SimpleResource album;
        album.setUri( QUrl("_:XandY") );
        album.m_properties.insert( RDF::type(), NMM::MusicAlbum() );
        album.m_properties.insert( NIE::title(), "X&Y" );

        Nepomuk::SimpleResource res1;
        res1.setUri( QUrl("nepomuk:/res/m/Res1") );
        res1.m_properties.insert( RDF::type(), NFO::FileDataObject() );
        res1.m_properties.insert( RDF::type(), NMM::MusicPiece() );
        res1.m_properties.insert( NFO::fileName(), "Yellow.mp3" );
        res1.m_properties.insert( NMM::performer(), QUrl("_:coldplay") );
        res1.m_properties.insert( NMM::musicAlbum(), QUrl("_:XandY") );
        Nepomuk::SimpleResourceGraph resGraph;
        resGraph << res1 << coldplay << album;

        //
        // Do the actual merging
        //
        kDebug() << "Perform the merge";
        m_dmModel->mergeResources( resGraph, "TestApp" );

        QVERIFY( m_model->containsAnyStatement( res1.uri(), Soprano::Node(),
                                                Soprano::Node() ) );
        QVERIFY( m_model->containsAnyStatement( res1.uri(), NFO::fileName(),
                                                Soprano::LiteralValue("Yellow.mp3") ) );
        kDebug() << m_model->listStatements( res1.uri(), Soprano::Node(), Soprano::Node() ).allStatements();
        QVERIFY( m_model->listStatements( res1.uri(), Soprano::Node(), Soprano::Node() ).allStatements().size() == res1.m_properties.size() );

        QList< Node > objects = m_model->listStatements( res1.uri(), NMM::performer(), Soprano::Node() ).iterateObjects().allNodes();

        QVERIFY( objects.size() == 1 );
        QVERIFY( objects.first().isResource() );

        QUrl coldplayUri = objects.first().uri();
        QVERIFY( coldplayUri == QUrl("nepomuk:/res/coldplay") );
        QList< Soprano::Statement > stList = coldplay.toStatementList();
        foreach( Soprano::Statement st, stList ) {
            st.setSubject( coldplayUri );
            QVERIFY( m_model->containsAnyStatement( st ) );
        }

        objects = m_model->listStatements( res1.uri(), NMM::musicAlbum(), Soprano::Node() ).iterateObjects().allNodes();

        QVERIFY( objects.size() == 1 );
        QVERIFY( objects.first().isResource() );

        QUrl albumUri = objects.first().uri();
        stList = album.toStatementList();
        foreach( Soprano::Statement st, stList ) {
            st.setSubject( albumUri );
            QVERIFY( m_model->containsAnyStatement( st ) );
        }
    }
}

QTEST_KDEMAIN_CORE(DataManagementModelTest)

#include "datamanagementmodeltest.moc"
