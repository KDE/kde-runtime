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
#include <Soprano/Graph>
#define USING_SOPRANO_NRLMODEL_UNSTABLE_API
#include <Soprano/NRLModel>

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

    // DataManagementModel relies on the ussage of a NRLModel in the storage service
    Soprano::NRLModel* nrlModel = new Soprano::NRLModel(m_model);
    nrlModel->setParent(m_model);

    m_dmModel = new Nepomuk::DataManagementModel( nrlModel );
}

void DataManagementModelTest::cleanupTestCase()
{
    delete m_dmModel;
    delete m_model;
    delete m_storageDir;
}

void DataManagementModelTest::init()
{
    resetModel();
}


void DataManagementModelTest::testSetProperty()
{
    // adding the most basic property
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/A"), QVariantList() << QVariant(QLatin1String("foobar")), QLatin1String("Testapp"));

    QVERIFY(!m_dmModel->lastError());

    // check that the actual data is there
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/A"), LiteralValue(QLatin1String("foobar"))));

    QTextStream s(stderr);
    m_model->write(s);

    // check that the app resource has been created with its corresponding graphs
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { ?r a %1 . ?r %2 %3 . } . "
                                                      "graph ?mg { ?g a %4 . ?mg a %5 . ?mg %6 ?g . } . }")
                                  .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::Agent()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::identifier()),
                                       Soprano::Node::literalToN3(QLatin1String("Testapp")),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::InstanceBase()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::GraphMetadata()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::coreGraphMetadataFor())),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    // check that we have an InstanceBase with a GraphMetadata graph
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { <res:/A> <prop:/A> %1 . } . "
                                                      "graph ?mg { ?g a %2 . ?mg a %3 . ?mg %4 ?g . } . "
                                                      "}")
                                  .arg(Soprano::Node::literalToN3(QLatin1String("foobar")),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::InstanceBase()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::GraphMetadata()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::coreGraphMetadataFor())),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    // check the number of graphs (two for the app, two for the actual data, and one for the ontology)
    QCOMPARE(m_model->listContexts().allElements().count(), 5);
}

// TODO: add tests that check handling of nie:url

void DataManagementModelTest::testRemoveProperty()
{
    const int cleanCount = m_model->statementCount();

    const QUrl g1("graph:/A");
    const QUrl mg1("graph:/B");
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/A"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/A"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/A"), Soprano::Vocabulary::NAO::lastModified(), LiteralValue(QDateTime::currentDateTime()), g1);
    m_model->addStatement(g1, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::InstanceBase(), mg1);
    m_model->addStatement(g1, Soprano::Vocabulary::NAO::created(), Soprano::LiteralValue(QDateTime::currentDateTime()), mg1);
    m_model->addStatement(mg1, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::GraphMetadata(), mg1);
    m_model->addStatement(mg1, Soprano::Vocabulary::NRL::coreGraphMetadataFor(), g1, mg1);

    m_dmModel->removeProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/A"), QVariantList() << QLatin1String("hello world"), QLatin1String("Testapp"));

    QVERIFY(!m_dmModel->lastError());

    // test that the data has been removed
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/A"), LiteralValue(QLatin1String("hello world"))));

    // test that the mtime has been updated (and is thus in another graph)
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Soprano::Vocabulary::NAO::lastModified(), Soprano::Node(), g1));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), Soprano::Vocabulary::NAO::lastModified(), Soprano::Node()));

    // test that the other property value is still valid
    QVERIFY(m_model->containsStatement(QUrl("res:/A"), QUrl("prop:/A"), LiteralValue(QLatin1String("foobar")), g1));


    // step 2: remove the second value
    m_dmModel->removeProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/A"), QVariantList() << QLatin1String("foobar"), QLatin1String("Testapp"));

    QVERIFY(!m_dmModel->lastError());

    // the property should be gone entirely
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/A"), Soprano::Node()));

    // even the resource should be gone since the NAO mtime does not count as a "real" property
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Soprano::Node(), Soprano::Node()));

    QTextStream s(stderr);
    m_model->write(s);

    // nothing except the ontology and the Testapp Agent should be left
    QCOMPARE(m_model->statementCount(), cleanCount+6);
}

void DataManagementModelTest::resetModel()
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
    QUrl graphUri("nepomuk:/ctx/TestGraph");

    int stCount = m_model->statementCount();
    m_model->addStatement( resUri, RDF::type(), NFO::FileDataObject(), graphUri );
    m_model->addStatement( resUri, QUrl("nepomuk:/mergeTest/prop1"),
                           Soprano::LiteralValue(10), graphUri );
    QCOMPARE( m_model->statementCount(), stCount + 2 );

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

    //
    // Test 3 -
    // Test the graph rules. If a resource exists in a nrl:DiscardableInstanceBase
    // and it is merged with a non-discardable graph. Then the graph should be replaced
    // In the opposite case - nothing should be done
    {
        Nepomuk::SimpleResource res;
        res.m_properties.insert( RDF::type(), NCO::Contact() );
        res.m_properties.insert( NCO::fullname(), "Lion" );

        QUrl graphUri("nepomuk:/ctx/mergeRes/testGraph");
        QUrl graphMetadataGraph("nepomuk:/ctx/mergeRes/testGraph-metadata");
        int stCount = m_model->statementCount();
        m_model->addStatement( graphUri, RDF::type(), NRL::InstanceBase(), graphMetadataGraph );
        m_model->addStatement( graphUri, RDF::type(), NRL::DiscardableInstanceBase(), graphMetadataGraph );
        m_model->addStatement( graphMetadataGraph, NRL::coreGraphMetadataFor(), graphUri, graphMetadataGraph );
        QVERIFY( m_model->statementCount() == stCount+3 );

        res.setUri(QUrl("nepomuk:/res/Lion"));
        QVERIFY( push( m_model, res, graphUri ) == res.m_properties.size() );
        res.setUri(QUrl("_:lion"));

        Nepomuk::SimpleResourceGraph resGraph;
        resGraph << res;

        QHash<QUrl, QVariant> additionalMetadata;
        additionalMetadata.insert( RDF::type(), NRL::InstanceBase() );
        m_dmModel->mergeResources( resGraph, "TestApp", additionalMetadata );
        
        QList<Soprano::Statement> stList = m_model->listStatements( Soprano::Node(), NCO::fullname(),
                                                            Soprano::LiteralValue("Lion") ).allStatements();
        kDebug() << stList;
        QVERIFY( stList.size() == 1 );
        Soprano::Node lionNode = stList.first().subject();
        QVERIFY( lionNode.isResource() );
        Soprano::Node graphNode = stList.first().context();
        QVERIFY( graphNode.isResource() );

        QVERIFY( !m_model->containsAnyStatement( graphNode, RDF::type(), NRL::DiscardableInstanceBase() ) );
    }
    {
        Nepomuk::SimpleResource res;
        res.m_properties.insert( RDF::type(), NCO::Contact() );
        res.m_properties.insert( NCO::fullname(), "Tiger" );

        QUrl graphUri("nepomuk:/ctx/mergeRes/testGraph2");
        QUrl graphMetadataGraph("nepomuk:/ctx/mergeRes/testGraph2-metadata");
        int stCount = m_model->statementCount();
        m_model->addStatement( graphUri, RDF::type(), NRL::InstanceBase(), graphMetadataGraph );
        m_model->addStatement( graphMetadataGraph, NRL::coreGraphMetadataFor(), graphUri, graphMetadataGraph );
        QVERIFY( m_model->statementCount() == stCount+2 );

        res.setUri(QUrl("nepomuk:/res/Tiger"));
        QVERIFY( push( m_model, res, graphUri ) == res.m_properties.size() );
        res.setUri(QUrl("_:tiger"));

        Nepomuk::SimpleResourceGraph resGraph;
        resGraph << res;

        QHash<QUrl, QVariant> additionalMetadata;
        additionalMetadata.insert( RDF::type(), NRL::InstanceBase() );
        m_dmModel->mergeResources( resGraph, "TestApp", additionalMetadata );

        QList<Soprano::Statement> stList = m_model->listStatements( Soprano::Node(), NCO::fullname(),
                                                           Soprano::LiteralValue("Tiger") ).allStatements();
        kDebug() << stList;
        QVERIFY( stList.size() == 1 );
        Soprano::Node TigerNode = stList.first().subject();
        QVERIFY( TigerNode.isResource() );
        Soprano::Node graphNode = stList.first().context();
        QVERIFY( graphNode.isResource() );

        QVERIFY( !m_model->containsAnyStatement( graphNode, RDF::type(), NRL::DiscardableInstanceBase() ) );
    }
}

void DataManagementModelTest::testMergeResources_createResource()
{
    using namespace Nepomuk;

    ResourceManager::instance()->setOverrideMainModel( m_model );

    //
    // Simple case: create a resource by merging it
    //
    SimpleResource res;
    res.setUri(QUrl("_:A"));
    res.m_properties.insert(Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NAO::Tag());
    res.m_properties.insert(Soprano::Vocabulary::NAO::prefLabel(), QLatin1String("Foobar"));

    m_dmModel->mergeResources(SimpleResourceGraph() << res, QLatin1String("testapp"));

    // check if the resource exists
    m_model->containsAnyStatement(Soprano::Node(), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NAO::Tag());
    m_model->containsAnyStatement(Soprano::Node(), Soprano::Vocabulary::NAO::prefLabel(), Soprano::LiteralValue(QLatin1String("Foobar")));

    // check if all the correct metadata graphs exist
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { ?r a %1 . ?r %2 %3 . } . "
                                                      "graph ?mg { ?g a %4 . ?mg a %5 . ?mg %6 ?g . } . "
                                                      "?g %7 ?a . ?a %8 %9 . "
                                                      "}")
                                  .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::Tag()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::prefLabel()),
                                       Soprano::Node::literalToN3(QLatin1String("Foobar")),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::InstanceBase()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::GraphMetadata()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::coreGraphMetadataFor()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::maintainedBy()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::identifier()),
                                       Soprano::Node::literalToN3(QLatin1String("testapp"))),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    //
    // Now create the same resource again
    //
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();
    m_dmModel->mergeResources(SimpleResourceGraph() << res, QLatin1String("testapp"));

    // nothing should have happened
    QCOMPARE(existingStatements, Soprano::Graph(m_model->listStatements().allStatements()));

    //
    // Now create the same resource with a different app
    //
    m_dmModel->mergeResources(SimpleResourceGraph() << res, QLatin1String("testapp2"));

    // only one thing should have been added: the new app Agent and its role as maintainer for the existing graph
    Q_FOREACH(const Soprano::Statement& s, existingStatements.toList()) {
        QVERIFY(m_model->containsStatement(s));
    }

    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { ?r a %1 . ?r %2 %3 . } . "
                                                      "?g %4 ?a1 . ?a1 %5 %6 . "
                                                      "?g %4 ?a2 . ?a2 %5 %7 . "
                                                      "}")
                                  .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::Tag()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::prefLabel()),
                                       Soprano::Node::literalToN3(QLatin1String("Foobar")),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::maintainedBy()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::identifier()),
                                       Soprano::Node::literalToN3(QLatin1String("testapp")),
                                       Soprano::Node::literalToN3(QLatin1String("testapp2"))),
                                  Soprano::Query::QueryLanguageSparql).boolValue());
}

QTEST_KDEMAIN_CORE(DataManagementModelTest)

#include "datamanagementmodeltest.moc"
