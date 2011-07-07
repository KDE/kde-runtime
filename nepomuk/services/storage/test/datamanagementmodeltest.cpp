/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>
   Copyright (C) 2011 Vishesh Handa <handa.vish@gmail.com>

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
#include "simpleresource.h"
#include "simpleresourcegraph.h"

#include <QtTest>
#include "qtest_kde.h"

#include <Soprano/Soprano>
#include <Soprano/Graph>
#define USING_SOPRANO_NRLMODEL_UNSTABLE_API
#include <Soprano/NRLModel>

#include <KTemporaryFile>
#include <KTempDir>
#include <KProtocolInfo>
#include <KDebug>

#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NMM>
#include <Nepomuk/Vocabulary/NCO>
#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/ResourceManager>

using namespace Soprano;
using namespace Soprano::Vocabulary;
using namespace Nepomuk;
using namespace Nepomuk::Vocabulary;


// TODO: test nao:created and nao:lastModified, these should always be correct for existing resources. This is especially important in the removeDataByApplication methods.

void DataManagementModelTest::resetModel()
{
    // remove all the junk from previous tests
    m_model->removeAllStatements();

    // add some classes and properties
    QUrl graph("graph:/onto");
    m_model->addStatement( graph, RDF::type(), NRL::Ontology(), graph );
    // removeResources depends on type inference
    m_model->addStatement( graph, RDF::type(), NRL::Graph(), graph );

    m_model->addStatement( QUrl("prop:/int"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/int"), RDFS::range(), XMLSchema::xsdInt(), graph );

    m_model->addStatement( QUrl("prop:/int2"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/int2"), RDFS::range(), XMLSchema::xsdInt(), graph );

    m_model->addStatement( QUrl("prop:/int3"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/int3"), RDFS::range(), XMLSchema::xsdInt(), graph );

    m_model->addStatement( QUrl("prop:/int_c1"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/int_c1"), RDFS::range(), XMLSchema::xsdInt(), graph );
    m_model->addStatement( QUrl("prop:/int_c1"), NRL::maxCardinality(), LiteralValue(1), graph );

    m_model->addStatement( QUrl("prop:/string"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/string"), RDFS::range(), XMLSchema::string(), graph );

    m_model->addStatement( QUrl("prop:/res"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/res"), RDFS::range(), RDFS::Resource(), graph );

    m_model->addStatement( QUrl("prop:/res2"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/res2"), RDFS::range(), RDFS::Resource(), graph );

    m_model->addStatement( QUrl("prop:/res3"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/res3"), RDFS::range(), RDFS::Resource(), graph );

    m_model->addStatement( QUrl("prop:/res_c1"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/res_c1"), RDFS::range(), RDFS::Resource(), graph );
    m_model->addStatement( QUrl("prop:/res_c1"), NRL::maxCardinality(), LiteralValue(1), graph );

    m_model->addStatement( QUrl("class:/typeA"), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( QUrl("class:/typeB"), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( QUrl("class:/typeB"), RDFS::subClassOf(), QUrl("class:/typeA"), graph );

    // properties used all the time
    m_model->addStatement( NAO::identifier(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( RDF::type(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( RDF::type(), RDFS::range(), RDFS::Class(), graph );
    m_model->addStatement( NIE::url(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NIE::url(), RDFS::range(), RDFS::Resource(), graph );


    // some ontology things the ResourceMerger depends on
    m_model->addStatement( RDFS::Class(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( RDFS::Class(), RDFS::subClassOf(), RDFS::Resource(), graph );
    m_model->addStatement( NRL::Graph(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NRL::InstanceBase(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NRL::InstanceBase(), RDFS::subClassOf(), NRL::Graph(), graph );
    m_model->addStatement( NAO::prefLabel(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NAO::prefLabel(), RDFS::range(), RDFS::Literal(), graph );
    m_model->addStatement( NFO::fileName(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NFO::fileName(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NCO::fullname(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NCO::fullname(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NIE::title(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NIE::title(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NAO::created(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NAO::created(), RDFS::range(), XMLSchema::dateTime(), graph );
    m_model->addStatement( NAO::created(), NRL::maxCardinality(), LiteralValue(1), graph );
    m_model->addStatement( NAO::lastModified(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NAO::lastModified(), RDFS::range(), XMLSchema::dateTime(), graph );
    m_model->addStatement( NAO::lastModified(), NRL::maxCardinality(), LiteralValue(1), graph );

    // used in testStoreResources_sameNieUrl
    m_model->addStatement( NAO::numericRating(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NAO::numericRating(), RDFS::range(), XMLSchema::xsdInt(), graph );
    m_model->addStatement( NAO::numericRating(), NRL::maxCardinality(), LiteralValue(1), graph );

    // some ontology things we need in testStoreResources_realLife
    m_model->addStatement( NMM::season(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::season(), RDFS::range(), XMLSchema::xsdInt(), graph );
    m_model->addStatement( NMM::episodeNumber(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::episodeNumber(), RDFS::range(), XMLSchema::xsdInt(), graph );
    m_model->addStatement( NMM::hasEpisode(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::hasEpisode(), RDFS::range(), NMM::TVShow(), graph );
    m_model->addStatement( NIE::description(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NIE::description(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NMM::synopsis(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::synopsis(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NMM::series(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::series(), RDFS::range(), NMM::TVSeries(), graph );
    m_model->addStatement( NIE::title(), RDFS::subClassOf(), QUrl("http://www.semanticdesktop.org/ontologies/2007/08/15/nao#identifyingProperty"), graph );

    // some ontology things we need in testStoreResources_strigiCase
    m_model->addStatement( NMM::performer(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::performer(), RDFS::range(), NCO::Contact(), graph );
    m_model->addStatement( NMM::musicAlbum(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::musicAlbum(), RDFS::range(), NMM::MusicAlbum(), graph );
    m_model->addStatement( NMM::MusicAlbum(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NMM::TVShow(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NMM::TVSeries(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NMM::MusicPiece(), RDF::type(), RDFS::Class(), graph );

    // used by testStoreResources_duplicates
    m_model->addStatement( NFO::hashAlgorithm(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NFO::hashAlgorithm(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NFO::hashValue(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NFO::hashValue(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NFO::hashValue(), NRL::maxCardinality(), LiteralValue(1), graph );
    m_model->addStatement( NFO::hasHash(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NFO::hasHash(), RDFS::range(), NFO::FileHash(), graph );
    m_model->addStatement( NFO::hasHash(), RDFS::domain(), NFO::FileDataObject(), graph );
    m_model->addStatement( NFO::FileHash(), RDF::type(), RDFS::Resource(), graph );
    m_model->addStatement( NFO::FileHash(), RDF::type(), RDFS::Class(), graph );


    m_model->addStatement( NIE::isPartOf(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NIE::isPartOf(), RDFS::range(), NFO::FileDataObject(), graph );
    m_model->addStatement( NIE::lastModified(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NIE::lastModified(), RDFS::range(), XMLSchema::dateTime(), graph );

    m_model->addStatement( NCO::fullname(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NCO::fullname(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NCO::fullname(), RDFS::domain(), NCO::Contact(), graph );
    m_model->addStatement( NCO::fullname(), NRL::maxCardinality(), LiteralValue(1), graph );
    m_model->addStatement( NCO::Contact(), RDF::type(), RDFS::Resource(), graph );
    m_model->addStatement( NCO::Contact(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NCO::Contact(), RDFS::subClassOf(), NCO::Role(), graph );
    m_model->addStatement( NCO::Contact(), RDFS::subClassOf(), NAO::Party(), graph );

    m_model->addStatement( NAO::Tag(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NFO::FileDataObject(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NFO::Folder(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NFO::Video(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NIE::InformationElement(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( QUrl("class:/typeA"), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( QUrl("class:/typeB"), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( QUrl("class:/typeC"), RDF::type(), RDFS::Class(), graph );

    // rebuild the internals of the data management model
    m_classAndPropertyTree->rebuildTree(m_dmModel);
}


void DataManagementModelTest::initTestCase()
{
    const Soprano::Backend* backend = Soprano::PluginManager::instance()->discoverBackendByName( "virtuosobackend" );
    QVERIFY( backend );
    m_storageDir = new KTempDir();
    m_model = backend->createModel( Soprano::BackendSettings() << Soprano::BackendSetting(Soprano::BackendOptionStorageDir, m_storageDir->name()) );
    QVERIFY( m_model );

    // DataManagementModel relies on the ussage of a NRLModel in the storage service
    m_nrlModel = new Soprano::NRLModel(m_model);
    m_classAndPropertyTree = new Nepomuk::ClassAndPropertyTree(this);
    m_dmModel = new Nepomuk::DataManagementModel(m_classAndPropertyTree, m_nrlModel);
}

void DataManagementModelTest::cleanupTestCase()
{
    delete m_dmModel;
    delete m_nrlModel;
    delete m_model;
    delete m_storageDir;
    delete m_classAndPropertyTree;
}

void DataManagementModelTest::init()
{
    resetModel();
}


void DataManagementModelTest::testAddProperty()
{
    // we start by simply adding a property
    m_dmModel->addProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl("prop:/string"), QVariantList() << QVariant(QLatin1String("foobar")), QLatin1String("Testapp"));

    QVERIFY(!m_dmModel->lastError());

    // check that the actual data is there
    QVERIFY(m_model->containsAnyStatement(QUrl("nepomuk:/res/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))));

    // check that the app resource has been created with its corresponding graphs
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { ?r a %1 . ?r %2 %3 . } . "
                                                      "graph ?mg { ?g a %4 . ?mg a %5 . ?mg %6 ?g . } . }")
                                  .arg(Soprano::Node::resourceToN3(NAO::Agent()),
                                       Soprano::Node::resourceToN3(NAO::identifier()),
                                       Soprano::Node::literalToN3(QLatin1String("Testapp")),
                                       Soprano::Node::resourceToN3(NRL::InstanceBase()),
                                       Soprano::Node::resourceToN3(NRL::GraphMetadata()),
                                       Soprano::Node::resourceToN3(NRL::coreGraphMetadataFor())),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    // check that we have an InstanceBase with a GraphMetadata graph
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { <nepomuk:/res/A> <prop:/string> %1 . } . "
                                                      "graph ?mg { ?g a %2 . ?mg a %3 . ?mg %4 ?g . } . "
                                                      "}")
                                  .arg(Soprano::Node::literalToN3(QLatin1String("foobar")),
                                       Soprano::Node::resourceToN3(NRL::InstanceBase()),
                                       Soprano::Node::resourceToN3(NRL::GraphMetadata()),
                                       Soprano::Node::resourceToN3(NRL::coreGraphMetadataFor())),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    // check the number of graphs (two for the app, two for the actual data, and one for the ontology)
    QCOMPARE(m_model->listContexts().allElements().count(), 5);


    //
    // add another property value on top of the existing one
    //
    m_dmModel->addProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl("prop:/string"), QVariantList() << QVariant(QLatin1String("hello world")), QLatin1String("Testapp"));

    // verify the values
    QCOMPARE(m_model->listStatements(QUrl("nepomuk:/res/A"), QUrl("prop:/string"), Node()).allStatements().count(), 2);
    QVERIFY(m_model->containsAnyStatement(QUrl("nepomuk:/res/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))));
    QVERIFY(m_model->containsAnyStatement(QUrl("nepomuk:/res/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world"))));

    // check that we only have one agent instance
    QCOMPARE(m_model->listStatements(Node(), RDF::type(), NAO::Agent()).allStatements().count(), 1);

    //
    // rewrite the same property with the same app
    //
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();
    m_dmModel->addProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl("prop:/string"), QVariantList() << QVariant(QLatin1String("hello world")), QLatin1String("Testapp"));

    // nothing should have changed
    QCOMPARE(existingStatements, Soprano::Graph(m_model->listStatements().allStatements()));


    //
    // rewrite the same property with another app
    //
    m_dmModel->addProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl("prop:/string"), QVariantList() << QVariant(QLatin1String("hello world")), QLatin1String("Otherapp"));

    // there should only be the new app, nothing else
    // thus, all previous statements need to be there
    foreach(const Statement& s, existingStatements.toList()) {
        QVERIFY(m_model->containsStatement(s));
    }


    // plus the new app
    existingStatements.addStatements(
                m_model->executeQuery(QString::fromLatin1("select ?g ?s ?p ?o where { graph ?g { ?s ?p ?o . } . filter(bif:exists((select (1) where { graph ?g { ?a a %1 . ?a %2 %3 . } . })))}")
                                      .arg(Soprano::Node::resourceToN3(NAO::Agent()),
                                           Soprano::Node::resourceToN3(NAO::identifier()),
                                           Soprano::Node::literalToN3(QLatin1String("Otherapp"))),
                                      Soprano::Query::QueryLanguageSparql)
                .iterateStatementsFromBindings(QLatin1String("s"), QLatin1String("p"), QLatin1String("o"), QLatin1String("g"))
                .allStatements()
                + m_model->executeQuery(QString::fromLatin1("select ?g ?s ?p ?o where { graph ?g { ?s ?p ?o . } . filter(bif:exists((select (1) where { graph ?gg { ?a a %1 . ?a %2 %3 . } . ?g %4 ?gg . })))}")
                                        .arg(Soprano::Node::resourceToN3(NAO::Agent()),
                                             Soprano::Node::resourceToN3(NAO::identifier()),
                                             Soprano::Node::literalToN3(QLatin1String("Otherapp")),
                                             Soprano::Node::resourceToN3(NRL::coreGraphMetadataFor())),
                                        Soprano::Query::QueryLanguageSparql)
                .iterateStatementsFromBindings(QLatin1String("s"), QLatin1String("p"), QLatin1String("o"), QLatin1String("g"))
                .allStatements()
                + m_model->listStatements(Node(), NAO::maintainedBy(), Node(), Node()).allStatements()
                );

    QCOMPARE(existingStatements, Soprano::Graph(m_model->listStatements().allStatements()));

    QVERIFY(!haveTrailingGraphs());
}

// test that creating a resource by adding a property on its URI properly sets metadata
void DataManagementModelTest::testAddProperty_createRes()
{
    // we create a new res by simply adding a property to it
    m_dmModel->addProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl("prop:/int"), QVariantList() << 42, QLatin1String("Testapp"));

    // now the newly created resource should have all the metadata a resource needs to have
    QVERIFY(m_model->containsAnyStatement(QUrl("nepomuk:/res/A"), NAO::created(), Node()));
    QVERIFY(m_model->containsAnyStatement(QUrl("nepomuk:/res/A"), NAO::lastModified(), Node()));

    // and both created and last modification date should be similar
    QCOMPARE(m_model->listStatements(QUrl("nepomuk:/res/A"), NAO::created(), Node()).iterateObjects().allNodes().first(),
             m_model->listStatements(QUrl("nepomuk:/res/A"), NAO::lastModified(), Node()).iterateObjects().allNodes().first());

    QVERIFY(!haveTrailingGraphs());
}


void DataManagementModelTest::testAddProperty_cardinality()
{
    // adding the same value twice in one call should result in one insert. This also includes the cardinality check
    m_dmModel->addProperty(QList<QUrl>() << QUrl("nepomuk:/res/AA"), QUrl("prop:/res_c1"), QVariantList() << QVariant(QUrl("nepomuk:/res/B")) << QVariant(QUrl("nepomuk:/res/B")), QLatin1String("Testapp"));
    QVERIFY(!m_dmModel->lastError());
    QCOMPARE(m_model->listStatements(QUrl("nepomuk:/res/AA"), QUrl("prop:/res_c1"), QUrl("nepomuk:/res/B")).allStatements().count(), 1);

    // we now add two values for a property with cardinality 1
    m_dmModel->addProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl("prop:/res_c1"), QVariantList() << QVariant(QUrl("nepomuk:/res/B")) << QVariant(QUrl("nepomuk:/res/C")), QLatin1String("Testapp"));
    QVERIFY(m_dmModel->lastError());

    m_dmModel->addProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl("prop:/res_c1"), QVariantList() << QVariant(QUrl("nepomuk:/res/B")), QLatin1String("Testapp"));
    m_dmModel->addProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl("prop:/res_c1"), QVariantList() << QVariant(QUrl("nepomuk:/res/C")), QLatin1String("Testapp"));

    // the second call needs to fail
    QVERIFY(m_dmModel->lastError());

    QVERIFY(!haveTrailingGraphs());
}


void DataManagementModelTest::testAddProperty_file()
{
    QTemporaryFile fileA;
    fileA.open();
    QTemporaryFile fileB;
    fileB.open();
    QTemporaryFile fileC;
    fileC.open();

    m_dmModel->addProperty(QList<QUrl>() << QUrl::fromLocalFile(fileA.fileName()), QUrl("prop:/string"), QVariantList() << QVariant(QLatin1String("foobar")), QLatin1String("Testapp"));

    // make sure the nie:url relation has been created
    QVERIFY(m_model->containsAnyStatement(Node(), NIE::url(), QUrl::fromLocalFile(fileA.fileName())));
    QVERIFY(!m_model->containsAnyStatement(QUrl::fromLocalFile(fileA.fileName()), Node(), Node()));

    // make sure the actual value is there
    QVERIFY(m_model->containsAnyStatement(m_model->listStatements(Node(), NIE::url(), QUrl::fromLocalFile(fileA.fileName())).allStatements().first().subject(), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))));


    // add relation from file to file
    m_dmModel->addProperty(QList<QUrl>() << QUrl::fromLocalFile(fileA.fileName()), QUrl("prop:/res"), QVariantList() << QVariant(QUrl::fromLocalFile(fileB.fileName())), QLatin1String("Testapp"));

    // make sure the nie:url relation has been created
    QVERIFY(m_model->containsAnyStatement(Node(), NIE::url(), QUrl::fromLocalFile(fileB.fileName())));
    QVERIFY(!m_model->containsAnyStatement(QUrl::fromLocalFile(fileB.fileName()), Node(), Node()));

    // make sure the actual value is there
    QVERIFY(m_model->containsAnyStatement(m_model->listStatements(Node(), NIE::url(), QUrl::fromLocalFile(fileA.fileName())).allStatements().first().subject(),
                                          QUrl("prop:/res"),
                                          m_model->listStatements(Node(), NIE::url(), QUrl::fromLocalFile(fileB.fileName())).allStatements().first().subject()));


    // add the same relation but with another app
    m_dmModel->addProperty(QList<QUrl>() << QUrl::fromLocalFile(fileA.fileName()), QUrl("prop:/res"), QVariantList() << QVariant(QUrl::fromLocalFile(fileB.fileName())), QLatin1String("Otherapp"));

    // there is only one prop:/res relation defined
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/res"), Node()).allStatements().count(), 1);

    // we now add two values for a property with cardinality 1
    m_dmModel->addProperty(QList<QUrl>() << QUrl::fromLocalFile(fileA.fileName()), QUrl("prop:/res_c1"), QVariantList() << QVariant(QUrl::fromLocalFile(fileB.fileName())), QLatin1String("Testapp"));
    m_dmModel->addProperty(QList<QUrl>() << QUrl::fromLocalFile(fileA.fileName()), QUrl("prop:/res_c1"), QVariantList() << QVariant(QUrl::fromLocalFile(fileC.fileName())), QLatin1String("Testapp"));

    // the second call needs to fail
    QVERIFY(m_dmModel->lastError());


    // get the res URI for file:/A
    const QUrl fileAUri = m_model->listStatements(Soprano::Node(), NIE::url(), QUrl::fromLocalFile(fileA.fileName())).allStatements().first().subject().uri();

    // test adding a property to both the file and the resource URI. The result should be the exact same as doing it with only one of them
    m_dmModel->addProperty(QList<QUrl>() << fileAUri << QUrl::fromLocalFile(fileA.fileName()), QUrl("prop:/string"), QVariantList() << QVariant(QLatin1String("Whatever")), QLatin1String("Testapp"));

    QCOMPARE(m_model->listStatements(fileAUri, QUrl("prop:/string"), LiteralValue(QLatin1String("Whatever"))).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(Node(), NIE::url(), QUrl::fromLocalFile(fileA.fileName())).allStatements().count(), 1);

    // test the same with the file as object
    m_dmModel->addProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl("prop:/res"), QVariantList() << QVariant(KUrl(fileA.fileName())) << QVariant(fileAUri), QLatin1String("Testapp"));

    QCOMPARE(m_model->listStatements(QUrl("nepomuk:/res/A"), QUrl("prop:/res"), fileAUri).allStatements().count(), 1);
    QVERIFY(!m_model->containsAnyStatement(QUrl("nepomuk:/res/A"), QUrl("prop:/res"), QUrl::fromLocalFile(fileA.fileName())));
    QCOMPARE(m_model->listStatements(Node(), NIE::url(), QUrl::fromLocalFile(fileA.fileName())).allStatements().count(), 1);

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testAddProperty_invalidFile()
{
    KTemporaryFile f1;
    QVERIFY( f1.open() );
    QUrl f1Url( f1.fileName() );
    //f1Url.setScheme("file");

    m_dmModel->addProperty( QList<QUrl>() << f1Url, RDF::type(), QVariantList() << NAO::Tag(), QLatin1String("testapp") );

    // There should be some error that '' protocol doesn't exist
    QVERIFY(m_dmModel->lastError());

    // The support for plain file paths is in the DBus adaptor through the usage of KUrl. If
    // local path support is neccesary on the level of the model, simply use KUrl which
    // will automatically add the file:/ protocol to local paths.
    QVERIFY( !m_model->containsAnyStatement( Node(), NIE::url(), f1Url ) );

    m_dmModel->addProperty( QList<QUrl>() << QUrl("file:///Blah"), NIE::comment(),
                            QVariantList() << "Comment", QLatin1String("testapp") );

    // There should be some error as '/Blah' does not exist
    QVERIFY(m_dmModel->lastError());

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testAddProperty_invalid_args()
{
    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // empty resource list
    m_dmModel->addProperty(QList<QUrl>(), QUrl("prop:/int"), QVariantList() << 42, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty property uri
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl(), QVariantList() << 42, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty value list
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/int"), QVariantList(), QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty app
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/int"), QVariantList() << 42, QString());

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // invalid range
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/int"), QVariantList() << QLatin1String("foobar"), QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // protected properties 1
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), NAO::created(), QVariantList() << QDateTime::currentDateTime(), QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // protected properties 2
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), NAO::lastModified(), QVariantList() << QDateTime::currentDateTime(), QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // make sure we cannot add anything to non-existing files
    const QUrl nonExistingFileUrl("file:///a/file/that/is/very/unlikely/to/exist");
    m_dmModel->addProperty(QList<QUrl>() << nonExistingFileUrl, QUrl("prop:/int"), QVariantList() << 42, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // non-existing file as object
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/res"), QVariantList() << nonExistingFileUrl, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // TODO: try setting protected properties like nie:url, nfo:fileName, nie:isPartOf (only applies to files)
}

void DataManagementModelTest::testAddProperty_protectedTypes()
{
    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // property
    m_dmModel->addProperty(QList<QUrl>() << QUrl("prop:/res"), QUrl("prop:/int"), QVariantList() << 42, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // class
    m_dmModel->addProperty(QList<QUrl>() << NRL::Graph(), QUrl("prop:/int"), QVariantList() << 42, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // graph
    m_dmModel->addProperty(QList<QUrl>() << QUrl("graph:/onto"), QUrl("prop:/int"), QVariantList() << 42, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

void DataManagementModelTest::testAddProperty_akonadi()
{
    // create our app
    const QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);

    // create the graph
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    QUrl resA("nepomuk:/res/A");
    QUrl akonadiUrl("akonadi:id=5");
    m_model->addStatement( resA, RDF::type(), NIE::DataObject(), g1 );
    m_model->addStatement( resA, NIE::url(), akonadiUrl, g1 );

    // add a property using the akonadi URL
    // the tricky thing here is that nao:identifier does not have a range!
    m_dmModel->addProperty( QList<QUrl>() << akonadiUrl,
                            NAO::identifier(),
                            QVariantList() << QString("akon"),
                            QLatin1String("AppA") );

    QVERIFY(!m_dmModel->lastError());

    // check that the akonadi URL has been resolved to the resource URI
    QVERIFY(m_model->containsAnyStatement( resA, NAO::identifier(), Soprano::Node() ));

    // check that the property has the desired value
    QVERIFY(m_model->containsAnyStatement( resA, NAO::identifier(), LiteralValue("akon") ));
}

void DataManagementModelTest::testSetProperty()
{
    // adding the most basic property
    m_dmModel->setProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl("prop:/string"), QVariantList() << QVariant(QLatin1String("foobar")), QLatin1String("Testapp"));

    QVERIFY(!m_dmModel->lastError());

    // check that the actual data is there
    QVERIFY(m_model->containsAnyStatement(QUrl("nepomuk:/res/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))));

    // check that the app resource has been created with its corresponding graphs
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { ?r a %1 . ?r %2 %3 . } . "
                                                      "graph ?mg { ?g a %4 . ?mg a %5 . ?mg %6 ?g . } . }")
                                  .arg(Soprano::Node::resourceToN3(NAO::Agent()),
                                       Soprano::Node::resourceToN3(NAO::identifier()),
                                       Soprano::Node::literalToN3(QLatin1String("Testapp")),
                                       Soprano::Node::resourceToN3(NRL::InstanceBase()),
                                       Soprano::Node::resourceToN3(NRL::GraphMetadata()),
                                       Soprano::Node::resourceToN3(NRL::coreGraphMetadataFor())),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    // check that we have an InstanceBase with a GraphMetadata graph
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { <nepomuk:/res/A> <prop:/string> %1 . } . "
                                                      "graph ?mg { ?g a %2 . ?mg a %3 . ?mg %4 ?g . } . "
                                                      "}")
                                  .arg(Soprano::Node::literalToN3(QLatin1String("foobar")),
                                       Soprano::Node::resourceToN3(NRL::InstanceBase()),
                                       Soprano::Node::resourceToN3(NRL::GraphMetadata()),
                                       Soprano::Node::resourceToN3(NRL::coreGraphMetadataFor())),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    // check the number of graphs (two for the app, two for the actual data, and one for the ontology)
    QCOMPARE(m_model->listContexts().allElements().count(), 5);

    QVERIFY(!haveTrailingGraphs());
}

// test that creating a resource by setting a property on its URI properly sets metadata
void DataManagementModelTest::testSetProperty_createRes()
{
    // we create a new res by simply adding a property to it
    m_dmModel->setProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl("prop:/int"), QVariantList() << 42, QLatin1String("Testapp"));

    // now the newly created resource should have all the metadata a resource needs to have
    QVERIFY(m_model->containsAnyStatement(QUrl("nepomuk:/res/A"), NAO::created(), Node()));
    QVERIFY(m_model->containsAnyStatement(QUrl("nepomuk:/res/A"), NAO::lastModified(), Node()));

    // and both created and last modification date should be similar
    QCOMPARE(m_model->listStatements(QUrl("nepomuk:/res/A"), NAO::created(), Node()).iterateObjects().allNodes().first(),
             m_model->listStatements(QUrl("nepomuk:/res/A"), NAO::lastModified(), Node()).iterateObjects().allNodes().first());

    QVERIFY(!haveTrailingGraphs());
}


void DataManagementModelTest::testSetProperty_overwrite()
{
    // create an app graph
    const QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);

    // add a resource with 2 properties
    QUrl mg;
    const QUrl g = m_nrlModel->createGraph(NRL::InstanceBase(), &mg);
    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);

    m_model->addStatement(g, NAO::maintainedBy(), QUrl("app:/A"), mg);
    m_model->addStatement(g2, NAO::maintainedBy(), QUrl("app:/A"), mg2);

    m_model->addStatement(QUrl("res:/A"), RDF::type(), NAO::Tag(), g);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42), g);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(42), g);

    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int3"), LiteralValue(42), g2);

    QVERIFY(!haveTrailingGraphs());


    //
    // now overwrite the one property
    //
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/int"), QVariantList() << 12, QLatin1String("testapp"));

    // now the model should have replaced the old value and added the new value in a new graph
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(12)));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42)));

    // a new graph
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(12)).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(42)).allStatements().count(), 1);
    QVERIFY(m_model->listStatements(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(12)).allStatements().first().context().uri() != g);
    QVERIFY(m_model->listStatements(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(42)).allStatements().first().context().uri() == g);

    // the testapp Agent as maintainer of the new graph
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { <res:/A> <prop:/int> %1 . } . "
                                                      "graph ?mg { ?g a %2 . ?mg a %3 . ?mg %4 ?g . } . "
                                                      "?g %5 ?a . ?a %6 %7 . "
                                                      "}")
                                  .arg(Soprano::Node::literalToN3(12),
                                       Soprano::Node::resourceToN3(NRL::InstanceBase()),
                                       Soprano::Node::resourceToN3(NRL::GraphMetadata()),
                                       Soprano::Node::resourceToN3(NRL::coreGraphMetadataFor()),
                                       Soprano::Node::resourceToN3(NAO::maintainedBy()),
                                       Soprano::Node::resourceToN3(NAO::identifier()),
                                       Soprano::Node::literalToN3(QLatin1String("testapp"))),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    QVERIFY(!haveTrailingGraphs());


    //
    // Rewrite the same value
    //
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/int2"), QVariantList() << 42, QLatin1String("testapp"));

    // the value should only be there once
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(42)).allStatements().count(), 1);

    // in a new graph since the old one still contains the type
    QVERIFY(m_model->listStatements(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(42)).allStatements().first().context().uri() != g);

    // there should be one graph now which contains the value and which is marked as being maintained by both apps
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { <res:/A> <prop:/int2> %1 . } . "
                                                      "graph ?mg { ?g a %2 . ?mg a %3 . ?mg %4 ?g . } . "
                                                      "?g %5 ?a1 . ?a1 %6 %7 . "
                                                      "?g %5 ?a2 . ?a2 %6 %8 . "
                                                      "}")
                                  .arg(Soprano::Node::literalToN3(42),
                                       Soprano::Node::resourceToN3(NRL::InstanceBase()),
                                       Soprano::Node::resourceToN3(NRL::GraphMetadata()),
                                       Soprano::Node::resourceToN3(NRL::coreGraphMetadataFor()),
                                       Soprano::Node::resourceToN3(NAO::maintainedBy()),
                                       Soprano::Node::resourceToN3(NAO::identifier()),
                                       Soprano::Node::literalToN3(QLatin1String("testapp")),
                                       Soprano::Node::literalToN3(QLatin1String("A"))),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    //
    // Now we rewrite the type which should result in reusing the old graph but with the new app as maintainer
    //
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/A"), RDF::type(), QVariantList() << NAO::Tag(), QLatin1String("testapp"));

    // the type should only be define once
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), RDF::type(), NAO::Tag()).allStatements().count(), 1);

    // the graph should be the same
    QVERIFY(m_model->listStatements(QUrl("res:/A"), RDF::type(), NAO::Tag()).allStatements().first().context().uri() == g);

    // the new app should be listed as maintainer as should be the old one
    QCOMPARE(m_model->listStatements(g, NAO::maintainedBy(), Node(), mg).allStatements().count(), 2);

    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { <res:/A> a %1 . } . "
                                                      "graph ?mg { ?g a %2 . ?mg a %3 . ?mg %4 ?g . } . "
                                                      "?g %5 ?a1 . ?a1 %6 %7 . "
                                                      "?g %5 ?a2 . ?a2 %6 %8 . "
                                                      "}")
                                  .arg(Soprano::Node::resourceToN3(NAO::Tag()),
                                       Soprano::Node::resourceToN3(NRL::InstanceBase()),
                                       Soprano::Node::resourceToN3(NRL::GraphMetadata()),
                                       Soprano::Node::resourceToN3(NRL::coreGraphMetadataFor()),
                                       Soprano::Node::resourceToN3(NAO::maintainedBy()),
                                       Soprano::Node::resourceToN3(NAO::identifier()),
                                       Soprano::Node::literalToN3(QLatin1String("testapp")),
                                       Soprano::Node::literalToN3(QLatin1String("A"))),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testSetProperty_invalid_args()
{
    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // empty resource list
    m_dmModel->setProperty(QList<QUrl>(), QUrl("prop:/int"), QVariantList() << 42, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty property uri
    m_dmModel->setProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl(), QVariantList() << 42, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty value list
    m_dmModel->setProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl("prop:/int"), QVariantList(), QLatin1String("testapp"));

    // the call should NOT have failed
    QVERIFY(!m_dmModel->lastError());

    // but nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty app
    m_dmModel->setProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl("prop:/int"), QVariantList() << 42, QString());

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // invalid range
    m_dmModel->setProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl("prop:/int"), QVariantList() << QLatin1String("foobar"), QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // make sure we cannot add anything to non-existing files
    const QUrl nonExistingFileUrl("file:///a/file/that/is/very/unlikely/to/exist");
    m_dmModel->setProperty(QList<QUrl>() << nonExistingFileUrl, QUrl("prop:/int"), QVariantList() << 42, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // non-existing file as object
    m_dmModel->setProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), QUrl("prop:/res"), QVariantList() << nonExistingFileUrl, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

void DataManagementModelTest::testSetProperty_nieUrl1()
{
    // setting nie:url if it is not there yet should result in a normal setProperty including graph creation
    m_dmModel->setProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), NIE::url(), QVariantList() << QUrl("file:///tmp/A"), QLatin1String("testapp"));

    QVERIFY(m_model->containsAnyStatement(QUrl("nepomuk:/res/A"), NIE::url(), QUrl("file:///tmp/A")));

    // remember the graph since it should not change later on
    const QUrl nieUrlGraph = m_model->listStatements(QUrl("nepomuk:/res/A"), NIE::url(), QUrl("file:///tmp/A")).allStatements().first().context().uri();

    QVERIFY(!haveTrailingGraphs());


    // we reset the URL
    m_dmModel->setProperty(QList<QUrl>() << QUrl("nepomuk:/res/A"), NIE::url(), QVariantList() << QUrl("file:///tmp/B"), QLatin1String("testapp"));

    // the url should have changed
    QVERIFY(!m_model->containsAnyStatement(QUrl("nepomuk:/res/A"), NIE::url(), QUrl("file:///tmp/A")));
    QVERIFY(m_model->containsAnyStatement(QUrl("nepomuk:/res/A"), NIE::url(), QUrl("file:///tmp/B")));

    // the graph should have been kept
    QCOMPARE(m_model->listStatements(QUrl("nepomuk:/res/A"), NIE::url(), Node()).allStatements().first().context().uri(), nieUrlGraph);

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testSetProperty_nieUrl2()
{
    KTempDir* dir = createNieUrlTestData();

    // change the nie:url of one of the top level dirs
    const QUrl newDir1Url = QUrl(QLatin1String("file://") + dir->name() + QLatin1String("dir1-new"));

    // we first need to move the file, otherwise the file check in the dms kicks in
    QVERIFY(QFile::rename(dir->name() + QLatin1String("dir1"), newDir1Url.toLocalFile()));

    // now update the database
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/dir1"), NIE::url(), QVariantList() << newDir1Url, QLatin1String("testapp"));

    // this should have updated the nie:urls of all children, too
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/dir1"), NIE::url(), newDir1Url));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/dir1"), NFO::fileName(), LiteralValue(QLatin1String("dir1-new"))));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/dir11"), NIE::url(), QUrl(newDir1Url.toString() + QLatin1String("/dir11"))));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/dir12"), NIE::url(), QUrl(newDir1Url.toString() + QLatin1String("/dir12"))));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/dir13"), NIE::url(), QUrl(newDir1Url.toString() + QLatin1String("/dir13"))));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/file11"), NIE::url(), QUrl(newDir1Url.toString() + QLatin1String("/file11"))));

    delete dir;

    QVERIFY(!haveTrailingGraphs());
}

// the same test as above only using the file URL
void DataManagementModelTest::testSetProperty_nieUrl3()
{
    KTempDir* dir = createNieUrlTestData();

    // change the nie:url of one of the top level dirs
    const QUrl oldDir1Url = QUrl(QLatin1String("file://") + dir->name() + QLatin1String("dir1"));
    const QUrl newDir1Url = QUrl(QLatin1String("file://") + dir->name() + QLatin1String("dir1-new"));

    // we first need to move the file, otherwise the file check in the dms kicks in
    QVERIFY(QFile::rename(oldDir1Url.toLocalFile(), newDir1Url.toLocalFile()));

    // now update the database
    m_dmModel->setProperty(QList<QUrl>() << oldDir1Url, NIE::url(), QVariantList() << newDir1Url, QLatin1String("testapp"));

    // this should have updated the nie:urls of all children, too
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/dir1"), NIE::url(), newDir1Url));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/dir1"), NFO::fileName(), LiteralValue(QLatin1String("dir1-new"))));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/dir11"), NIE::url(), QUrl(newDir1Url.toString() + QLatin1String("/dir11"))));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/dir12"), NIE::url(), QUrl(newDir1Url.toString() + QLatin1String("/dir12"))));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/dir13"), NIE::url(), QUrl(newDir1Url.toString() + QLatin1String("/dir13"))));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/file11"), NIE::url(), QUrl(newDir1Url.toString() + QLatin1String("/file11"))));

    delete dir;

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testSetProperty_nieUrl4()
{
    KTempDir* dir = createNieUrlTestData();

    // move one of the dirs to a new parent
    const QUrl oldDir121Url = QUrl(QLatin1String("file://") + dir->name() + QLatin1String("dir1/dir12/dir121"));
    const QUrl newDir121Url = QUrl(QLatin1String("file://") + dir->name() + QLatin1String("dir1/dir12/dir121-new"));

    // we first need to move the file, otherwise the file check in the dms kicks in
    QVERIFY(QFile::rename(oldDir121Url.toLocalFile(), newDir121Url.toLocalFile()));

    // now update the database
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/dir121"), NIE::url(), QVariantList() << newDir121Url, QLatin1String("testapp"));

    // the url
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/dir121"), NIE::url(), newDir121Url));

    // the child file
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/file1211"), NIE::url(), QUrl(newDir121Url.toString() + QLatin1String("/file1211"))));

    // the nie:isPartOf relationship should have been updated, too
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/dir121"), NIE::isPartOf(), QUrl("res:/dir12")));

    QVERIFY(!haveTrailingGraphs());
}

// the same test as above only using the file URL
void DataManagementModelTest::testSetProperty_nieUrl5()
{
    KTempDir* dir = createNieUrlTestData();

    // move one of the dirs to a new parent
    const QUrl oldDir121Url = QUrl(QLatin1String("file://") + dir->name() + QLatin1String("dir1/dir12/dir121"));
    const QUrl newDir121Url = QUrl(QLatin1String("file://") + dir->name() + QLatin1String("dir2/dir121"));

    // we first need to move the file, otherwise the file check in the dms kicks in
    QVERIFY(QFile::rename(oldDir121Url.toLocalFile(), newDir121Url.toLocalFile()));

    // now update the database
    m_dmModel->setProperty(QList<QUrl>() << oldDir121Url, NIE::url(), QVariantList() << newDir121Url, QLatin1String("testapp"));

    // the url
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/dir121"), NIE::url(), newDir121Url));

    // the child file
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/file1211"), NIE::url(), QUrl(newDir121Url.toString() + QLatin1String("/file1211"))));

    // the nie:isPartOf relationship should have been updated, too
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/dir121"), NIE::isPartOf(), QUrl("res:/dir2")));

    QVERIFY(!haveTrailingGraphs());
}

// test support for any other URL scheme which already exists as nie:url (This is what libnepomuk does support)
void DataManagementModelTest::testSetProperty_nieUrl6()
{
    // create a resource that has a URL
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase());
    const QUrl url("http://nepomuk.kde.org/");

    m_model->addStatement(QUrl("res:/A"), NIE::url(), url, g1);


    // use the url to set a property
    m_dmModel->setProperty(QList<QUrl>() << url, QUrl("prop:/int"), QVariantList() << 42, QLatin1String("A"));

    // check that the property has been added to the resource
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42)));

    // check that no new resource has been created
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/int"), LiteralValue(42)).allElements().count(), 1);
}

void DataManagementModelTest::testSetProperty_protectedTypes()
{
    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // property
    m_dmModel->setProperty(QList<QUrl>() << QUrl("prop:/res"), QUrl("prop:/int"), QVariantList() << 42, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // class
    m_dmModel->setProperty(QList<QUrl>() << NRL::Graph(), QUrl("prop:/int"), QVariantList() << 42, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // graph
    m_dmModel->setProperty(QList<QUrl>() << QUrl("graph:/onto"), QUrl("prop:/int"), QVariantList() << 42, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

// make sure we reuse legacy resource URIs
void DataManagementModelTest::testSetProperty_legacyData()
{
    // create some legacy data
    QTemporaryFile file;
    file.open();
    const KUrl url(file.fileName());

    const QUrl g = m_nrlModel->createGraph(NRL::InstanceBase());

    m_model->addStatement(url, QUrl("prop:/int"), LiteralValue(42), g);

    // set some data with the url
    m_dmModel->setProperty(QList<QUrl>() << url, QUrl("prop:/int"), QVariantList() << 2, QLatin1String("A"));

    // make sure the resource has changed
    QCOMPARE(m_model->listStatements(url, QUrl("prop:/int"), Node()).allElements().count(), 1);
    QCOMPARE(m_model->listStatements(url, QUrl("prop:/int"), Node()).allElements().first().object().literal(), LiteralValue(2));
}

void DataManagementModelTest::testRemoveProperty()
{
    const int cleanCount = m_model->statementCount();

    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::lastModified(), LiteralValue(QDateTime::currentDateTime()), g1);

    m_dmModel->removeProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/string"), QVariantList() << QLatin1String("hello world"), QLatin1String("Testapp"));

    QVERIFY(!m_dmModel->lastError());

    // test that the data has been removed
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world"))));

    // test that the mtime has been updated (and is thus in another graph)
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), NAO::lastModified(), Soprano::Node(), g1));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NAO::lastModified(), Soprano::Node()));

    // test that the other property value is still valid
    QVERIFY(m_model->containsStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1));

    QVERIFY(!haveTrailingGraphs());


    // step 2: remove the second value
    m_dmModel->removeProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/string"), QVariantList() << QLatin1String("foobar"), QLatin1String("Testapp"));

    QVERIFY(!m_dmModel->lastError());

    // the property should be gone entirely
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), Soprano::Node()));

    // even the resource should be gone since the NAO mtime does not count as a "real" property
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Soprano::Node(), Soprano::Node()));

    // nothing except the ontology and the Testapp Agent should be left
    QCOMPARE(m_model->statementCount(), cleanCount+6);

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testRemoveProperty_file()
{
    QTemporaryFile fileA;
    fileA.open();
    QTemporaryFile fileB;
    fileB.open();

    // prepare some test data
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);

    m_model->addStatement(QUrl("res:/A"), NIE::url(), QUrl::fromLocalFile(fileA.fileName()), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("whatever")), g1);

    m_model->addStatement(QUrl("res:/B"), NIE::url(), QUrl::fromLocalFile(fileB.fileName()), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/B"), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/C"), g1);



    // now we remove one value via the file URL
    m_dmModel->removeProperty(QList<QUrl>() << QUrl::fromLocalFile(fileA.fileName()), QUrl("prop:/string"), QVariantList() << QLatin1String("hello world"), QLatin1String("Testapp"));

    QCOMPARE(m_model->listStatements(QUrl("res:/A"), QUrl("prop:/string"), Node()).allStatements().count(), 2);
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), NIE::url(), Node()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), NIE::url(), QUrl::fromLocalFile(fileA.fileName())).allStatements().count(), 1);


    // test the same with a file URL value
    m_dmModel->removeProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/res"), QVariantList() << QUrl::fromLocalFile(fileB.fileName()), QLatin1String("Testapp"));

    QCOMPARE(m_model->listStatements(QUrl("res:/A"), QUrl("prop:/res"), Node()).allStatements().count(), 1);

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testRemoveProperty_invalid_args()
{
    // prepare some test data
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::lastModified(), LiteralValue(QDateTime::currentDateTime()), g1);

    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // empty resource list
    m_dmModel->removeProperty(QList<QUrl>(), QUrl("prop:/string"), QVariantList() << QLatin1String("foobar"), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // resource list with empty URL
    m_dmModel->removeProperty(QList<QUrl>() << QUrl() << QUrl("res:/A"), QUrl("prop:/string"), QVariantList() << QLatin1String("foobar"), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty property
    m_dmModel->removeProperty(QList<QUrl>() << QUrl("res:/A"), QUrl(), QVariantList() << QLatin1String("foobar"), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty values
    m_dmModel->removeProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/string"), QVariantList(), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty app
    m_dmModel->removeProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/string"), QVariantList() << QLatin1String("foobar"), QString());

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // invalid value type
    m_dmModel->removeProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/int"), QVariantList() << QLatin1String("foobar"), QString("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // protected property 1
    m_dmModel->removeProperty(QList<QUrl>() << QUrl("res:/A"), NAO::created(), QVariantList() << QDateTime::currentDateTime(), QString("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // protected property 2
    m_dmModel->removeProperty(QList<QUrl>() << QUrl("res:/A"), NAO::lastModified(), QVariantList() << QDateTime::currentDateTime(), QString("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // make sure we cannot add anything to non-existing files
    const QUrl nonExistingFileUrl("file:///a/file/that/is/very/unlikely/to/exist");
    m_dmModel->removeProperty(QList<QUrl>() << nonExistingFileUrl, QUrl("prop:/int"), QVariantList() << 42, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // non-existing file as object
    m_dmModel->removeProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/res"), QVariantList() << nonExistingFileUrl, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

// it is not allowed to change properties, classes or graphs through this API
void DataManagementModelTest::testRemoveProperty_protectedTypes()
{
    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // property
    m_dmModel->removeProperty(QList<QUrl>() << QUrl("prop:/res"), QUrl("prop:/int"), QVariantList() << 42, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // class
    m_dmModel->removeProperty(QList<QUrl>() << NRL::Graph(), QUrl("prop:/int"), QVariantList() << 42, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // graph
    m_dmModel->removeProperty(QList<QUrl>() << QUrl("graph:/onto"), QUrl("prop:/int"), QVariantList() << 42, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

void DataManagementModelTest::testRemoveProperties()
{
    QTemporaryFile fileA;
    fileA.open();
    QTemporaryFile fileB;
    fileB.open();

    // prepare some test data
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);

    m_model->addStatement(QUrl("res:/A"), NIE::url(), QUrl::fromLocalFile(fileA.fileName()), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("whatever")), g1);

    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(12), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(2), g1);

    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(12), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(2), g1);

    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("whatever")), g1);

    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/int"), LiteralValue(6), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/int"), LiteralValue(12), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/int"), LiteralValue(2), g1);

    m_model->addStatement(QUrl("res:/B"), NIE::url(), QUrl::fromLocalFile(fileB.fileName()), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/B"), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/C"), g1);


    // test removing one property from one resource
    m_dmModel->removeProperties(QList<QUrl>() << QUrl("res:/A"), QList<QUrl>() << QUrl("prop:/string"), QLatin1String("testapp"));

    // check that all values are gone
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), Node()));

    // check that all other values from that prop are still there
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/string"), Node()).allStatements().count(), 3);
    QCOMPARE(m_model->listStatements(QUrl("res:/B"), QUrl("prop:/string"), Node()).allStatements().count(), 3);


    // test removing a property from more than one resource
    m_dmModel->removeProperties(QList<QUrl>() << QUrl("res:/A") << QUrl("res:/B"), QList<QUrl>() << QUrl("prop:/int"), QLatin1String("testapp"));

    // check that all values are gone
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int"), Node()));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/B"), QUrl("prop:/int"), Node()));

    // check that other properties from res:/B are still there
    QCOMPARE(m_model->listStatements(QUrl("res:/B"), QUrl("prop:/string"), Node()).allStatements().count(), 3);


    // test file URLs in the resources
    m_dmModel->removeProperties(QList<QUrl>() << QUrl::fromLocalFile(fileA.fileName()), QList<QUrl>() << QUrl("prop:/int2"), QLatin1String("testapp"));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int2"), Node()));

    // TODO: verify graphs

    QVERIFY(!haveTrailingGraphs());
}


void DataManagementModelTest::testRemoveProperties_invalid_args()
{
    QTemporaryFile fileA;
    fileA.open();
    QTemporaryFile fileB;
    fileB.open();

    // prepare some test data
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);

    m_model->addStatement(QUrl("res:/A"), NIE::url(), QUrl::fromLocalFile(fileA.fileName()), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("whatever")), g1);

    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(12), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(2), g1);

    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(12), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(2), g1);

    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("whatever")), g1);

    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/int"), LiteralValue(6), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/int"), LiteralValue(12), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/int"), LiteralValue(2), g1);

    m_model->addStatement(QUrl("res:/B"), NIE::url(), QUrl::fromLocalFile(fileB.fileName()), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/B"), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/C"), g1);


    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // empty resource list
    m_dmModel->removeProperties(QList<QUrl>(), QList<QUrl>() << QUrl("prop:/string"), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // resource list with empty url
    m_dmModel->removeProperties(QList<QUrl>() << QUrl() << QUrl("res:/A"), QList<QUrl>() << QUrl("prop:/string"), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty property list
    m_dmModel->removeProperties(QList<QUrl>() << QUrl("res:/A"), QList<QUrl>(), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // property list with empty url
    m_dmModel->removeProperties(QList<QUrl>() << QUrl("res:/A"), QList<QUrl>() << QUrl("prop:/string") << QUrl(), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty app
    m_dmModel->removeProperties(QList<QUrl>() << QUrl("res:/A"), QList<QUrl>() << QUrl("prop:/string"), QString());

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // protected property 1
    m_dmModel->removeProperties(QList<QUrl>() << QUrl("res:/A"), QList<QUrl>() << NAO::created(), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // protected property 2
    m_dmModel->removeProperties(QList<QUrl>() << QUrl("res:/A"), QList<QUrl>() << NAO::lastModified(), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // make sure we cannot add anything to non-existing files
    const QUrl nonExistingFileUrl("file:///a/file/that/is/very/unlikely/to/exist");
    m_dmModel->removeProperties(QList<QUrl>() << nonExistingFileUrl, QList<QUrl>() << QUrl("prop:/int"), QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}


void DataManagementModelTest::testRemoveProperties_protectedTypes()
{
    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // property
    m_dmModel->removeProperties(QList<QUrl>() << QUrl("prop:/res"), QList<QUrl>() << QUrl("prop:/int"), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // class
    m_dmModel->removeProperties(QList<QUrl>() << NRL::Graph(), QList<QUrl>() << QUrl("prop:/int"), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // graph
    m_dmModel->removeProperties(QList<QUrl>() << QUrl("graph:/onto"), QList<QUrl>() << QUrl("prop:/int"), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);}

void DataManagementModelTest::testRemoveResources()
{
    QTemporaryFile fileA;
    fileA.open();
    QTemporaryFile fileB;
    fileB.open();

    // prepare some test data
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);

    m_model->addStatement(QUrl("res:/A"), NIE::url(), QUrl::fromLocalFile(fileA.fileName()), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);

    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("whatever")), g1);

    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);

    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/int"), LiteralValue(6), g2);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/int"), LiteralValue(12), g2);
    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/int"), LiteralValue(2), g2);
    m_model->addStatement(QUrl("res:/B"), NIE::url(), QUrl::fromLocalFile(fileB.fileName()), g2);


    m_dmModel->removeResources(QList<QUrl>() << QUrl("res:/A"), Nepomuk::NoRemovalFlags, QLatin1String("testapp"));

    // verify that the resource is gone
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));

    // verify that other resources were not touched
    QCOMPARE(m_model->listStatements(QUrl("res:/B"), Node(), Node()).allStatements().count(), 4);
    QCOMPARE(m_model->listStatements(QUrl("res:/C"), Node(), Node()).allStatements().count(), 2);

    // verify that removing resources by file URL works
    m_dmModel->removeResources(QList<QUrl>() << QUrl::fromLocalFile(fileB.fileName()), Nepomuk::NoRemovalFlags, QLatin1String("testapp"));

    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/B"), Node(), Node()));

    // verify that other resources were not touched
    QCOMPARE(m_model->listStatements(QUrl("res:/C"), Node(), Node()).allStatements().count(), 2);

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testRemoveResources_subresources()
{
    // create our apps (we use more than one since we also test that it is ignored)
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/B"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/B"), NAO::identifier(), LiteralValue(QLatin1String("B")), appG);

    // create the graphs
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);
    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);
    m_model->addStatement(g2, NAO::maintainedBy(), QUrl("app:/B"), mg2);
    QUrl mg3;
    const QUrl g3 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg3);
    m_model->addStatement(g3, NAO::maintainedBy(), QUrl("app:/A"), mg3);

    // create the resource to delete
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);

    // sub-resource 1: can be deleted
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/B"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/B"), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);

    // sub-resource 2: can be deleted (is defined in another graph by the same app)
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/AA"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/AA"), g1);
    m_model->addStatement(QUrl("res:/AA"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g3);

    // sub-resource 3: can be deleted although another res refs it (we also delete the other res)
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/C"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/C"), g1);
    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/res"), QUrl("res:/C"), g1);

    // sub-resource 4: cannot be deleted since another res refs it
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/D"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/D"), g1);
    m_model->addStatement(QUrl("res:/D"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/E"), QUrl("prop:/res"), QUrl("res:/D"), g1);
    m_model->addStatement(QUrl("res:/E"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);

    // sub-resource 5: can be deleted although another app added properties
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/F"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/F"), g1);
    m_model->addStatement(QUrl("res:/F"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/F"), QUrl("prop:/int"), LiteralValue(42), g2);

    // delete the resource
    m_dmModel->removeResources(QList<QUrl>() << QUrl("res:/A"), Nepomuk::RemoveSubResoures, QLatin1String("A"));

    // this should have removed A, B and C
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/B"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/C"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/F"), Node(), Node()));

    // E and F need to be preserved
    QCOMPARE(m_model->listStatements(QUrl("res:/D"), Node(), Node()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(QUrl("res:/E"), Node(), Node()).allStatements().count(), 2);

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testRemoveResources_invalid_args()
{
    QTemporaryFile fileA;
    fileA.open();

    // prepare some test data
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);

    m_model->addStatement(QUrl("res:/A"), NIE::url(), QUrl::fromLocalFile(fileA.fileName()), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);

    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("whatever")), g1);


    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // empty resource list
    m_dmModel->removeResources(QList<QUrl>(), Nepomuk::NoRemovalFlags, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // resource list with empty URL
    m_dmModel->removeResources(QList<QUrl>() << QUrl("res:/A") << QUrl(), Nepomuk::NoRemovalFlags, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty app
    m_dmModel->removeResources(QList<QUrl>() << QUrl("res:/A"), Nepomuk::NoRemovalFlags, QString());

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // non-existing file
    const QUrl nonExistingFileUrl("file:///a/file/that/is/very/unlikely/to/exist");
    m_dmModel->removeResources(QList<QUrl>() << nonExistingFileUrl, Nepomuk::NoRemovalFlags, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

// make sure we do not allow to remove classes, properties, and graphs
void DataManagementModelTest::testRemoveResources_protectedTypes()
{
    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // property
    m_dmModel->removeResources(QList<QUrl>() << QUrl("prop:/res"), Nepomuk::NoRemovalFlags, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // class
    m_dmModel->removeResources(QList<QUrl>() << NRL::Graph(), Nepomuk::NoRemovalFlags, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // graph
    m_dmModel->removeResources(QList<QUrl>() << QUrl("graph:/onto"), Nepomuk::NoRemovalFlags, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

// make sure the mtime of related resources is updated properly
void DataManagementModelTest::testRemoveResources_mtimeRelated()
{
    // first we create our apps and graphs (just to have some pseudo real data)
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    m_model->addStatement(QUrl("app:/B"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/B"), NAO::identifier(), LiteralValue(QLatin1String("B")), appG);

    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);
    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);
    m_model->addStatement(g2, NAO::maintainedBy(), QUrl("app:/B"), mg2);

    const QDateTime date = QDateTime::currentDateTime();


    // now we create different resources
    // A is the resource to be deleted
    // B is related to A and its mtime needs update
    // C is unrelated and no mtime change should occur
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(date), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::lastModified(), LiteralValue(date), g1);

    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g2);
    m_model->addStatement(QUrl("res:/B"), NAO::created(), LiteralValue(date), g1);
    m_model->addStatement(QUrl("res:/B"), NAO::lastModified(), LiteralValue(date), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/res"), QUrl("res:/A"), g2);

    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g2);
    m_model->addStatement(QUrl("res:/C"), NAO::created(), LiteralValue(date), g2);
    m_model->addStatement(QUrl("res:/C"), NAO::lastModified(), LiteralValue(date), g2);


    // now we remove res:/A
    m_dmModel->removeResources(QList<QUrl>() << QUrl("res:/A"), Nepomuk::NoRemovalFlags, QLatin1String("A"));


    // now only the mtime of B should have changed
    QCOMPARE(m_model->listStatements(QUrl("res:/B"), NAO::lastModified(), Node()).allElements().count(), 1);
    QVERIFY(m_model->listStatements(QUrl("res:/B"), NAO::lastModified(), Node()).allElements().first().object().literal().toDateTime() > date);

    QCOMPARE(m_model->listStatements(QUrl("res:/C"), NAO::lastModified(), Node()).allElements().count(), 1);
    QCOMPARE(m_model->listStatements(QUrl("res:/C"), NAO::lastModified(), Node()).allElements().first().object().literal().toDateTime(), date);
}

// make sure we can remove data from non-existing files
void DataManagementModelTest::testRemoveResources_deletedFile()
{
    QTemporaryFile fileA;
    fileA.open();

    const KUrl fileUrl(fileA.fileName());

    // create our app
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);

    // create the data graph
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    // create the resource
    m_model->addStatement(QUrl("res:/A"), NIE::url(), fileUrl, g1);
    m_model->addStatement(QUrl("res:/A"), RDF::type(), NFO::FileDataObject(), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);

    // now remove the file
    fileA.close();
    QFile::remove(fileUrl.toLocalFile());

    // now try removing the data
    m_dmModel->removeResources(QList<QUrl>() << fileUrl, NoRemovalFlags, QLatin1String("A"));

    // the call should succeed
    QVERIFY(!m_dmModel->lastError());

    // the resource should be gone
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(Node(), Node(), fileUrl));
}

void DataManagementModelTest::testCreateResource()
{
    // the simple test: we just create a resource using all params
    const QUrl resUri = m_dmModel->createResource(QList<QUrl>() << QUrl("class:/typeA") << QUrl("class:/typeB"), QLatin1String("the label"), QLatin1String("the desc"), QLatin1String("A"));

    // this call should succeed
    QVERIFY(!m_dmModel->lastError());

    // check if the returned uri is valid
    QVERIFY(!resUri.isEmpty());
    QCOMPARE(resUri.scheme(), QString(QLatin1String("nepomuk")));

    // check if the resource was created properly
    QVERIFY(m_model->containsAnyStatement(resUri, RDF::type(), QUrl("class:/typeA")));
    QVERIFY(m_model->containsAnyStatement(resUri, RDF::type(), QUrl("class:/typeB")));
    QVERIFY(m_model->containsAnyStatement(resUri, NAO::prefLabel(), LiteralValue::createPlainLiteral(QLatin1String("the label"))));
    QVERIFY(m_model->containsAnyStatement(resUri, NAO::description(), LiteralValue::createPlainLiteral(QLatin1String("the desc"))));
}

void DataManagementModelTest::testCreateResource_invalid_args()
{
    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // try to create a resource without any types
    m_dmModel->createResource(QList<QUrl>(), QString(), QString(), QLatin1String("A"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // use an invalid type
    m_dmModel->createResource(QList<QUrl>() << QUrl("class:/non-existing-type"), QString(), QString(), QLatin1String("A"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // use a property as type
    m_dmModel->createResource(QList<QUrl>() << NAO::prefLabel(), QString(), QString(), QLatin1String("A"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

// the isolated test: create one graph with one resource, delete that resource
void DataManagementModelTest::testRemoveDataByApplication1()
{
    // create our app
    const QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);

    // create the resource to delete
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);

    // delete the resource
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), Nepomuk::NoRemovalFlags, QLatin1String("A"));

    // verify that nothing is left, not even the graph
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(Node(), NAO::maintainedBy(), QUrl("app:/A")));
    QCOMPARE(m_model->listStatements(Node(), RDF::type(), NRL::InstanceBase()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(Node(), RDF::type(), NRL::GraphMetadata()).allStatements().count(), 1);

    QVERIFY(!haveTrailingGraphs());
}

// scatter resource over two graphs, only one of which is supposed to be removed
void DataManagementModelTest::testRemoveDataByApplication2()
{
    // create our apps
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/B"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/B"), NAO::identifier(), LiteralValue(QLatin1String("B")), appG);

    // create the resource to delete
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);
    m_model->addStatement(g2, NAO::maintainedBy(), QUrl("app:/B"), mg2);

    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g2);

    // delete the resource
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), Nepomuk::NoRemovalFlags, QLatin1String("A"));

    // verify that graph1 is gone completely
    QVERIFY(!m_model->containsAnyStatement(Node(), Node(), Node(), g1));

    // only two statements left: the one in the second graph and the last modification date
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), Node(), Node()).allStatements().count(), 2);
    QVERIFY(m_model->containsStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g2));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NAO::lastModified(), Node()));

    // four graphs: g2, the 2 app graphs, and the mtime graph
    QCOMPARE(m_model->listStatements(Node(), RDF::type(), NRL::InstanceBase()).allStatements().count(), 4);

    QVERIFY(!haveTrailingGraphs());
}

// two apps that maintain a graph should keep the data when one removes it
void DataManagementModelTest::testRemoveDataByApplication3()
{
    // create our apps
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/B"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/B"), NAO::identifier(), LiteralValue(QLatin1String("B")), appG);

    // create the resource to delete
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/B"), mg1);

    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);

    // delete the resource
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), Nepomuk::NoRemovalFlags, QLatin1String("A"));

    // the resource should still be there, without any changes, not even a changed mtime
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), Node(), Node()).allStatements().count(), 2);

    QVERIFY(!m_model->containsAnyStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1));
    QCOMPARE(m_model->listStatements(g1, NAO::maintainedBy(), Node()).allStatements().count(), 1);

    QVERIFY(!haveTrailingGraphs());
}

// test file URLs + not removing nie:url
void DataManagementModelTest::testRemoveDataByApplication4()
{
    QTemporaryFile fileA;
    fileA.open();

    // create our apps
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/B"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/B"), NAO::identifier(), LiteralValue(QLatin1String("B")), appG);

    // create the resource to delete
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);
    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);
    m_model->addStatement(g2, NAO::maintainedBy(), QUrl("app:/B"), mg2);

    m_model->addStatement(QUrl("res:/A"), NIE::url(), QUrl::fromLocalFile(fileA.fileName()), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g2);

    // delete the resource
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl::fromLocalFile(fileA.fileName()), Nepomuk::NoRemovalFlags, QLatin1String("A"));

    // now the nie:url should still be there even though A created it
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NIE::url(), QUrl::fromLocalFile(fileA.fileName())));

    // creation time should have been created
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NAO::lastModified(), Node()));

    // the foobar value should be gone
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))));

    // the "hello world" should still be there
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world"))));

    QVERIFY(!haveTrailingGraphs());
}

// test sub-resource handling the easy kind
void DataManagementModelTest::testRemoveDataByApplication5()
{
    // create our apps
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);

    // create the resource to delete
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/B"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/B"), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);

    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);

    // delete the resource
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), Nepomuk::RemoveSubResoures, QLatin1String("A"));

    // this should have removed both A and B
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/B"), Node(), Node()));

    QVERIFY(!haveTrailingGraphs());
}

// test sub-resource handling
void DataManagementModelTest::testRemoveDataByApplication6()
{
    // create our apps
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/B"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/B"), NAO::identifier(), LiteralValue(QLatin1String("B")), appG);

    // create the graphs
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);
    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);
    m_model->addStatement(g2, NAO::maintainedBy(), QUrl("app:/B"), mg2);
    QUrl mg3;
    const QUrl g3 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg3);
    m_model->addStatement(g3, NAO::maintainedBy(), QUrl("app:/A"), mg3);

    // create the resource to delete
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);

    // sub-resource 1: can be deleted
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/B"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/B"), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);

    // sub-resource 2: can be deleted (is defined in another graph by the same app)
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/AA"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/AA"), g1);
    m_model->addStatement(QUrl("res:/AA"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g3);

    // sub-resource 3: can be deleted although another res refs it (we also delete the other res)
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/C"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/C"), g1);
    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/res"), QUrl("res:/C"), g1);

    // sub-resource 4: cannot be deleted since another res refs it
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/D"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/D"), g1);
    m_model->addStatement(QUrl("res:/D"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/E"), QUrl("prop:/res"), QUrl("res:/D"), g1);
    m_model->addStatement(QUrl("res:/E"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);

    // sub-resource 5: cannot be deleted since another app added properties
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/F"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/F"), g1);
    m_model->addStatement(QUrl("res:/F"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/F"), QUrl("prop:/int"), LiteralValue(42), g2);

    // sub-resource 6: can be deleted since the prop another app added is only metadata
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/G"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/G"), g1);
    m_model->addStatement(QUrl("res:/G"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/G"), NAO::lastModified(), LiteralValue(QDateTime::currentDateTime()), g2);

    // delete the resource
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), Nepomuk::RemoveSubResoures, QLatin1String("A"));

    // this should have removed A, B and C
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/B"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/C"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/G"), Node(), Node()));

    // E and F need to be preserved
    QCOMPARE(m_model->listStatements(QUrl("res:/D"), Node(), Node()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(QUrl("res:/E"), Node(), Node()).allStatements().count(), 2);
    QCOMPARE(m_model->listStatements(QUrl("res:/F"), Node(), Node()).allStatements().count(), 2);

    QVERIFY(!haveTrailingGraphs());
}

// make sure that we do not remove metadata from resources that were also touched by other apps
void DataManagementModelTest::testRemoveDataByApplication7()
{
    // create our apps
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/B"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/B"), NAO::identifier(), LiteralValue(QLatin1String("B")), appG);

    // create the resource to delete
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);
    m_model->addStatement(g2, NAO::maintainedBy(), QUrl("app:/B"), mg2);

    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g2);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);

    // delete the resource
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), Nepomuk::NoRemovalFlags, QLatin1String("A"));

    // verify that the creation date is still there
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NAO::created(), Soprano::Node()));

    QVERIFY(!haveTrailingGraphs());
}

// make sure everything is removed even if splitted in more than one graph
void DataManagementModelTest::testRemoveDataByApplication8()
{
    // create our app
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);

    // create the resource to delete
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);
    m_model->addStatement(g2, NAO::maintainedBy(), QUrl("app:/A"), mg2);

    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g2);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);

    // delete the resource
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), Nepomuk::NoRemovalFlags, QLatin1String("A"));

    // verify that all has gone
    // verify that nothing is left, not even the graph
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(Node(), NAO::maintainedBy(), QUrl("app:/A")));
    QCOMPARE(m_model->listStatements(Node(), RDF::type(), NRL::InstanceBase()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(Node(), RDF::type(), NRL::GraphMetadata()).allStatements().count(), 1);

    QVERIFY(!haveTrailingGraphs());
}

// make sure that we still maintain other resources in the same graph after deleting one resource
void DataManagementModelTest::testRemoveDataByApplication9()
{
    // create our apps
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/B"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/B"), NAO::identifier(), LiteralValue(QLatin1String("B")), appG);

    // create the graph
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/B"), mg1);

    // create the resources
    const QDateTime dt = QDateTime::currentDateTime();
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(dt), g1);

    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/B"), NAO::created(), LiteralValue(dt), g1);

    // now remove res:/A by app A
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), Nepomuk::NoRemovalFlags, QLatin1String("A"));

    // now there should be 2 graphs - once for res:/A which is only maintained by B, and one for res:/B which is still
    // maintained by A and B
    // 1. check that B still maintains res:/A (all of it in one graph)
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { graph ?g { <res:/A> <prop:/string> %1 . <res:/A> %2 %3 . } . ?g %4 <app:/B> . }")
                                  .arg(Soprano::Node::literalToN3(LiteralValue(QLatin1String("foobar"))),
                                       Soprano::Node::resourceToN3(NAO::created()),
                                       Soprano::Node::literalToN3(LiteralValue(dt)),
                                       Soprano::Node::resourceToN3(NAO::maintainedBy())),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    // 2. check that A does not maintain res:/A anymore
    QVERIFY(!m_model->executeQuery(QString::fromLatin1("ask where { graph ?g { <res:/A> ?p ?o } . ?g %1 <app:/A> . }")
                                  .arg(Soprano::Node::resourceToN3(NAO::maintainedBy())),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    // 3. check that both A and B do still maintain res:/B
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { graph ?g { <res:/B> <prop:/string> %1 . <res:/B> %2 %3 . } . ?g %4 <app:/A> . ?g %4 <app:/B> . }")
                                  .arg(Soprano::Node::literalToN3(LiteralValue(QLatin1String("hello world"))),
                                       Soprano::Node::resourceToN3(NAO::created()),
                                       Soprano::Node::literalToN3(LiteralValue(dt)),
                                       Soprano::Node::resourceToN3(NAO::maintainedBy())),
                                  Soprano::Query::QueryLanguageSparql).boolValue());
}

// This test simply creates a lot of resources using storeResources and then
// removes all of them using removeDataByApplication.
// This is exactly what the strigi service does.
void DataManagementModelTest::testRemoveDataByApplication10()
{
    QLatin1String app("AppA");
    QList<QUrl> uris;

    for( int i=0; i<10; i++ ) {
        QTemporaryFile fileA;
        fileA.open();

        SimpleResource res;
        res.addProperty( RDF::type(), NFO::FileDataObject() );
        res.addProperty( NIE::url(), QUrl(fileA.fileName()) );

        m_dmModel->storeResources( SimpleResourceGraph() << res, app );
        QVERIFY( !m_dmModel->lastError() );

        QString query = QString::fromLatin1("select ?r where { ?r %1 %2 . }")
                        .arg( Node::resourceToN3( NIE::url() ),
                            Node::resourceToN3( QUrl(fileA.fileName()) ) );

        QList<Node> list = m_dmModel->executeQuery( query, Soprano::Query::QueryLanguageSparql ).iterateBindings(0).allNodes();
        QCOMPARE( list.size(), 1 );

        uris << list.first().uri();
    }

    //
    // Remove the data
    //

    m_dmModel->removeDataByApplication( uris, Nepomuk::RemoveSubResoures,
                                        QLatin1String("AppA") );
    QVERIFY( !m_dmModel->lastError() );

    QString query = QString::fromLatin1("ask where { graph ?g { ?r ?p ?o . } ?g %1 ?app . ?app %2 %3 . }")
                    .arg( Node::resourceToN3( NAO::maintainedBy() ),
                          Node::resourceToN3( NAO::identifier() ),
                          Node::literalToN3( app ) );

    QVERIFY( !m_dmModel->executeQuery( query, Soprano::Query::QueryLanguageSparql ).boolValue() );

    foreach( const QUrl resUri, uris ) {

        // The Resource should no longer have any statements
        QList<Soprano::Statement> l = m_dmModel->listStatements( resUri, Node(), Node() ).allStatements();
        QVERIFY( l.isEmpty() );
    }

    QVERIFY(!haveTrailingGraphs());
}

// make sure that graphs which do not have a maintaining app are handled properly, too
void DataManagementModelTest::testRemoveDataByApplication11()
{
    QTemporaryFile fileA;
    fileA.open();

    // create our app
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);

    // create the resource to delete
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    // graph 2 does not have a maintaining app!
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase());

    m_model->addStatement(QUrl("res:/A"), NIE::url(), QUrl::fromLocalFile(fileA.fileName()), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g2);

    // delete the resource
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl::fromLocalFile(fileA.fileName()), Nepomuk::NoRemovalFlags, QLatin1String("A"));

    // now the nie:url should still be there even though A created it
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NIE::url(), QUrl::fromLocalFile(fileA.fileName())));

    // creation time should have been created
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NAO::lastModified(), Node()));

    // the foobar value should be gone
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))));

    // the "hello world" should still be there
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world"))));

    QVERIFY(!haveTrailingGraphs());
}

// make sure that weird cross sub-resource'ing is handled properly. This is very unlikely to ever happen, but still...
void DataManagementModelTest::testRemoveDataByApplication_subResourcesOfSubResources()
{
    // create our app
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);

    // create the graph
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    // create the resource to delete
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);

    // sub-resource 1
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/B"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/B"), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);

    // sub-resource 2
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/C"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/C"), g1);
    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);

    // sub-resource 3 (also sub-resource to res:/C)
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/D"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/D"), g1);
    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/res"), QUrl("res:/D"), g1);
    m_model->addStatement(QUrl("res:/C"), NAO::hasSubResource(), QUrl("res:/D"), g1);
    m_model->addStatement(QUrl("res:/D"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);


    // delete the resource
    QBENCHMARK_ONCE
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), Nepomuk::RemoveSubResoures, QLatin1String("A"));


    // all resources should have been removed
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/B"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/C"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/D"), Node(), Node()));
}

// This is some real data that I have in my nepomuk repo
void DataManagementModelTest::testRemoveDataByApplication_realLife()
{
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/c07bb72a-6aec-450a-9622-fe8f05f83d79>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.w3.org/2000/01/rdf-schema#Resource>"),Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/c07bb72a-6aec-450a-9622-fe8f05f83d79>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.w3.org/2000/01/rdf-schema#Resource>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/c07bb72a-6aec-450a-9622-fe8f05f83d79>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#userVisible>"),Soprano::Node::fromN3("\"1\"^^<http://www.w3.org/2001/XMLSchema#int>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/c07bb72a-6aec-450a-9622-fe8f05f83d79>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#lastModified>"),Soprano::Node::fromN3("\"2011-05-24T10:35:39.414Z\"^^<http://www.w3.org/2001/XMLSchema#dateTime>"),Soprano::Node::fromN3("<nepomuk:/ctx/1b99f767-3652-4bb5-97a5-dcb469ffa186>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/c07bb72a-6aec-450a-9622-fe8f05f83d79>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#hasTag>"),Soprano::Node::fromN3("<nepomuk:/res/8e251223-a066-4c90-863f-2f49237c870f>"),Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/c07bb72a-6aec-450a-9622-fe8f05f83d79>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#created>"),Soprano::Node::fromN3("\"2011-04-26T19:25:21.435Z\"^^<http://www.w3.org/2001/XMLSchema#dateTime>"),Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/c07bb72a-6aec-450a-9622-fe8f05f83d79>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/01/19/nie#url>"),Soprano::Node::fromN3("<file:///home/vishesh/Videos/The%20Big%20Bang%20Theory/Season%201/7_Dumpling%20Paradox.avi>"),Soprano::Node::fromN3("<nepomuk:/ctx/33966b99-de71-4d77-8cbc-a3e3c104d681>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/8e251223-a066-4c90-863f-2f49237c870f>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.w3.org/2000/01/rdf-schema#Resource>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/8e251223-a066-4c90-863f-2f49237c870f>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#Tag>"),Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/8e251223-a066-4c90-863f-2f49237c870f>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#userVisible>"),Soprano::Node::fromN3("\"1\"^^<http://www.w3.org/2001/XMLSchema#int>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/8e251223-a066-4c90-863f-2f49237c870f>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#identifier>"),Soprano::Node::fromN3("\"Big%20Bang\"^^<http://www.w3.org/2001/XMLSchema#string>"),Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/8e251223-a066-4c90-863f-2f49237c870f>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#created>"),Soprano::Node::fromN3("\"2011-04-26T19:25:21.475Z\"^^<http://www.w3.org/2001/XMLSchema#dateTime>"),Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.w3.org/2000/01/rdf-schema#Resource>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#Data>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#Graph>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#InstanceBase>"),Soprano::Node::fromN3("<nepomuk:/ctx/a33dd431-cbf7-4aef-8217-a0dedfa7b56d>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#userVisible>"),Soprano::Node::fromN3("\"1\"^^<http://www.w3.org/2001/XMLSchema#int>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#created>"),Soprano::Node::fromN3("\"2011-04-26T19:25:21.443Z\"^^<http://www.w3.org/2001/XMLSchema#dateTime>"),Soprano::Node::fromN3("<nepomuk:/ctx/a33dd431-cbf7-4aef-8217-a0dedfa7b56d>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/1b99f767-3652-4bb5-97a5-dcb469ffa186>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.w3.org/2000/01/rdf-schema#Resource>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/1b99f767-3652-4bb5-97a5-dcb469ffa186>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#Data>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/1b99f767-3652-4bb5-97a5-dcb469ffa186>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#Graph>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/1b99f767-3652-4bb5-97a5-dcb469ffa186>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#InstanceBase>"),Soprano::Node::fromN3("<nepomuk:/ctx/81107abc-443a-4d75-9cb3-3a93d18af6c2>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/1b99f767-3652-4bb5-97a5-dcb469ffa186>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#userVisible>"),Soprano::Node::fromN3("\"1\"^^<http://www.w3.org/2001/XMLSchema#int>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/1b99f767-3652-4bb5-97a5-dcb469ffa186>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#created>"),Soprano::Node::fromN3("\"2011-05-24T10:35:39.52Z\"^^<http://www.w3.org/2001/XMLSchema#dateTime>"),Soprano::Node::fromN3("<nepomuk:/ctx/81107abc-443a-4d75-9cb3-3a93d18af6c2>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/1b99f767-3652-4bb5-97a5-dcb469ffa186>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#maintainedBy>"),Soprano::Node::fromN3("<nepomuk:/res/e2eb2efb-14ee-4038-ac24-698f916289b0>"),Soprano::Node::fromN3("<nepomuk:/ctx/81107abc-443a-4d75-9cb3-3a93d18af6c2>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/e2eb2efb-14ee-4038-ac24-698f916289b0>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.w3.org/2000/01/rdf-schema#Resource>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/e2eb2efb-14ee-4038-ac24-698f916289b0>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#Agent>"),Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/e2eb2efb-14ee-4038-ac24-698f916289b0>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#userVisible>"),Soprano::Node::fromN3("\"1\"^^<http://www.w3.org/2001/XMLSchema#int>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/e2eb2efb-14ee-4038-ac24-698f916289b0>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#identifier>"),Soprano::Node::fromN3("\"nepomukindexer\"^^<http://www.w3.org/2001/XMLSchema#string>"),Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.w3.org/2000/01/rdf-schema#Resource>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#Data>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#Graph>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#InstanceBase>"),Soprano::Node::fromN3("<nepomuk:/ctx/2e4f3918-7da3-4736-b998-2d1ab7cbd6e4>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#userVisible>"),Soprano::Node::fromN3("\"1\"^^<http://www.w3.org/2001/XMLSchema#int>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#created>"),Soprano::Node::fromN3("\"2011-05-16T20:42:12.551Z\"^^<http://www.w3.org/2001/XMLSchema#dateTime>"),Soprano::Node::fromN3("<nepomuk:/ctx/2e4f3918-7da3-4736-b998-2d1ab7cbd6e4>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.w3.org/2000/01/rdf-schema#Resource>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#Data>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#Graph>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#InstanceBase>"),Soprano::Node::fromN3("<nepomuk:/ctx/a33dd431-cbf7-4aef-8217-a0dedfa7b56d>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#userVisible>"),Soprano::Node::fromN3("\"1\"^^<http://www.w3.org/2001/XMLSchema#int>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/4da68d35-c6a2-4029-8fd9-a84ef7c2c60f>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#created>"),Soprano::Node::fromN3("\"2011-04-26T19:25:21.443Z\"^^<http://www.w3.org/2001/XMLSchema#dateTime>"),Soprano::Node::fromN3("<nepomuk:/ctx/a33dd431-cbf7-4aef-8217-a0dedfa7b56d>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/33966b99-de71-4d77-8cbc-a3e3c104d681>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.w3.org/2000/01/rdf-schema#Resource>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/33966b99-de71-4d77-8cbc-a3e3c104d681>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#Data>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/33966b99-de71-4d77-8cbc-a3e3c104d681>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#Graph>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/33966b99-de71-4d77-8cbc-a3e3c104d681>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#InstanceBase>"),Soprano::Node::fromN3("<nepomuk:/ctx/873b6b73-5dd5-44d3-b138-4b280065f5af>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/33966b99-de71-4d77-8cbc-a3e3c104d681>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#userVisible>"),Soprano::Node::fromN3("\"1\"^^<http://www.w3.org/2001/XMLSchema#int>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/33966b99-de71-4d77-8cbc-a3e3c104d681>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#created>"),Soprano::Node::fromN3("\"2011-04-26T19:25:21.443Z\"^^<http://www.w3.org/2001/XMLSchema#dateTime>"),Soprano::Node::fromN3("<nepomuk:/ctx/873b6b73-5dd5-44d3-b138-4b280065f5af>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/33966b99-de71-4d77-8cbc-a3e3c104d681>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#maintainedBy>"),Soprano::Node::fromN3("<nepomuk:/res/e2eb2efb-14ee-4038-ac24-698f916289b0>"),Soprano::Node::fromN3("<nepomuk:/ctx/873b6b73-5dd5-44d3-b138-4b280065f5af>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/e2eb2efb-14ee-4038-ac24-698f916289b0>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.w3.org/2000/01/rdf-schema#Resource>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/e2eb2efb-14ee-4038-ac24-698f916289b0>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#Agent>"),Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/e2eb2efb-14ee-4038-ac24-698f916289b0>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#userVisible>"),Soprano::Node::fromN3("\"1\"^^<http://www.w3.org/2001/XMLSchema#int>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/res/e2eb2efb-14ee-4038-ac24-698f916289b0>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#identifier>"),Soprano::Node::fromN3("\"nepomukindexer\"^^<http://www.w3.org/2001/XMLSchema#string>"),Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.w3.org/2000/01/rdf-schema#Resource>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#Data>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#Graph>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>"),Soprano::Node::fromN3("<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nrl#InstanceBase>"),Soprano::Node::fromN3("<nepomuk:/ctx/2e4f3918-7da3-4736-b998-2d1ab7cbd6e4>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#userVisible>"),Soprano::Node::fromN3("\"1\"^^<http://www.w3.org/2001/XMLSchema#int>"),Soprano::Node::fromN3("<urn:crappyinference2:inferredtriples>") );
    m_model->addStatement( Soprano::Node::fromN3("<nepomuk:/ctx/90105034-0d05-444a-8f7f-5a957dae9f14>"),Soprano::Node::fromN3("<http://www.semanticdesktop.org/ontologies/2007/08/15/nao#created>"),Soprano::Node::fromN3("\"2011-05-16T20:42:12.551Z\"^^<http://www.w3.org/2001/XMLSchema#dateTime>"),Soprano::Node::fromN3("<nepomuk:/ctx/2e4f3918-7da3-4736-b998-2d1ab7cbd6e4>") );


    QUrl resUri("nepomuk:/res/c07bb72a-6aec-450a-9622-fe8f05f83d79");
    QUrl nieUrl("file:///home/vishesh/Videos/The%20Big%20Bang%20Theory/Season%201/7_Dumpling%20Paradox.avi");

    QString query = QString::fromLatin1("select ?app where { graph ?g { %1 %2 %3 .} "
                                        "?g %4 ?app . ?app %5 %6 . }" )
                    .arg( Node::resourceToN3( resUri ),
                          Node::resourceToN3( NIE::url() ),
                          Node::resourceToN3( nieUrl ),
                          Node::resourceToN3( NAO::maintainedBy() ),
                          Node::resourceToN3( NAO::identifier() ),
                          Node::literalToN3( LiteralValue("nepomukindexer") ) );

    QueryResultIterator it = m_model->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    QUrl appUri;
    if( it.next() ) {
        appUri = it[0].uri();
    }
    QCOMPARE( appUri, QUrl("nepomuk:/res/e2eb2efb-14ee-4038-ac24-698f916289b0") );

    m_dmModel->removeDataByApplication( QList<QUrl>() << resUri,
                                        Nepomuk::RemoveSubResoures,
                                        QLatin1String("nepomukindexer") );

    QString query2 = QString::fromLatin1("ask where { graph ?g { ?r %1 %2 .} "
                                        "?g %3 ?app . ?app %4 %5 . }" )
                    .arg( Node::resourceToN3( NIE::url() ),
                          Node::resourceToN3( nieUrl ),
                          Node::resourceToN3( NAO::maintainedBy() ),
                          Node::resourceToN3( NAO::identifier() ),
                          Node::literalToN3( LiteralValue("nepomukindexer") ) );

    QVERIFY( !m_model->executeQuery( query2, Soprano::Query::QueryLanguageSparql ).boolValue() );
}

void DataManagementModelTest::testRemoveDataByApplication_nieUrl()
{
    KTemporaryFile file;
    file.open();
    const QUrl fileUrl( file.fileName() );

    const QUrl res1("nepomuk:/res/1");

    // The file is tagged via Dolphin
    const QUrl g1 = m_nrlModel->createGraph( NRL::InstanceBase() );
    m_model->addStatement( res1, RDF::type(), NFO::FileDataObject() );
    m_model->addStatement( res1, NIE::url(), fileUrl, g1 );
    QDateTime now = QDateTime::currentDateTime();
    m_model->addStatement( res1, NAO::created(), LiteralValue(QVariant(now)), g1 );
    m_model->addStatement( res1, NAO::lastModified(), LiteralValue(QVariant(now)), g1 );

    const QUrl tag("nepomuk:/res/tag");
    m_model->addStatement( tag, RDF::type(), NAO::Tag(), g1 );
    m_model->addStatement( tag, NAO::identifier(), LiteralValue("tag"), g1 );
    m_model->addStatement( res1, NAO::hasTag(), tag, g1 );

    // Indexed via strigi
    SimpleResource simpleRes( res1 );
    simpleRes.addProperty( RDF::type(), NFO::FileDataObject() );
    simpleRes.addProperty( RDF::type(), NMM::MusicPiece() );
    simpleRes.addProperty( NIE::url(), fileUrl );

    m_dmModel->storeResources( SimpleResourceGraph() << simpleRes, QLatin1String("nepomukindexer"));
    QVERIFY( !m_dmModel->lastError() );

    // Remove strigi indexed content
    m_dmModel->removeDataByApplication( QList<QUrl>() << res1, Nepomuk::RemoveSubResoures,
                                        QLatin1String("nepomukindexer") );

    // The tag should still be there
    QVERIFY( m_model->containsStatement( tag, RDF::type(), NAO::Tag(), g1 ) );
    QVERIFY( m_model->containsStatement( tag, NAO::identifier(), LiteralValue("tag"), g1 ) );
    QVERIFY( m_model->containsStatement( res1, NAO::hasTag(), tag, g1 ) );

    // The resource should not have any data maintained by "nepomukindexer"
    QString query = QString::fromLatin1("select * where { graph ?g { %1 ?p ?o. } ?g %2 ?a . ?a %3 %4. }")
                    .arg( Node::resourceToN3( res1 ),
                          Node::resourceToN3( NAO::maintainedBy() ),
                          Node::resourceToN3( NAO::identifier() ),
                          Node::literalToN3( LiteralValue("nepomukindexer") ) );

    QList< BindingSet > bs = m_model->executeQuery( query, Soprano::Query::QueryLanguageSparql ).allBindings();
    kDebug() << bs;
    QVERIFY( bs.isEmpty() );

    // The nie:url should still exist
    QVERIFY( m_model->containsAnyStatement( res1, NIE::url(), fileUrl ) );
}

// make sure the mtime is updated properly in different situations
void DataManagementModelTest::testRemoveDataByApplication_mtime()
{
    // first we create our apps and graphs
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    m_model->addStatement(QUrl("app:/B"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/B"), NAO::identifier(), LiteralValue(QLatin1String("B")), appG);

    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);
    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);
    m_model->addStatement(g2, NAO::maintainedBy(), QUrl("app:/B"), mg2);

    const QDateTime date = QDateTime::currentDateTime();


    // now we create different resources
    // A has actual data maintained by app:/A
    // B has only metadata maintained by app:/A
    // C has nothing maintained by app:/A
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello")), g2);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(date), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::lastModified(), LiteralValue(date), g1);

    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g2);
    m_model->addStatement(QUrl("res:/B"), NAO::created(), LiteralValue(date), g1);
    m_model->addStatement(QUrl("res:/B"), NAO::lastModified(), LiteralValue(date), g1);

    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g2);
    m_model->addStatement(QUrl("res:/C"), NAO::created(), LiteralValue(date), g2);
    m_model->addStatement(QUrl("res:/C"), NAO::lastModified(), LiteralValue(date), g2);


    // we delete all three
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A") << QUrl("res:/B") << QUrl("res:/C"), Nepomuk::NoRemovalFlags, QLatin1String("A"));


    // now only the mtime of A should have changed
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), NAO::lastModified(), Node()).allElements().count(), 1);
    QVERIFY(m_model->listStatements(QUrl("res:/A"), NAO::lastModified(), Node()).allElements().first().object().literal().toDateTime() > date);

    QCOMPARE(m_model->listStatements(QUrl("res:/B"), NAO::lastModified(), Node()).allElements().count(), 1);
    QCOMPARE(m_model->listStatements(QUrl("res:/B"), NAO::lastModified(), Node()).allElements().first().object().literal().toDateTime(), date);

    QCOMPARE(m_model->listStatements(QUrl("res:/C"), NAO::lastModified(), Node()).allElements().count(), 1);
    QCOMPARE(m_model->listStatements(QUrl("res:/C"), NAO::lastModified(), Node()).allElements().first().object().literal().toDateTime(), date);
}

// make sure the mtime of resources that are related to deleted ones is updated
void DataManagementModelTest::testRemoveDataByApplication_mtimeRelated()
{
    // first we create our apps
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    m_model->addStatement(QUrl("app:/B"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/B"), NAO::identifier(), LiteralValue(QLatin1String("B")), appG);

    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);
    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);
    m_model->addStatement(g2, NAO::maintainedBy(), QUrl("app:/B"), mg2);

    const QDateTime date = QDateTime::currentDateTime();

    // we three two resources - one to delete, and one which is related to the one to be deleted,
    // one which is also related but will not be changed
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello")), g2);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/res"), QUrl("res:/A"), g1);
    m_model->addStatement(QUrl("res:/B"), NAO::created(), LiteralValue(date), g1);
    m_model->addStatement(QUrl("res:/B"), NAO::lastModified(), LiteralValue(date), g1);
    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/res"), QUrl("res:/A"), g2);
    m_model->addStatement(QUrl("res:/C"), NAO::created(), LiteralValue(date), g2);
    m_model->addStatement(QUrl("res:/C"), NAO::lastModified(), LiteralValue(date), g2);

    // now we remove res:/A
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), Nepomuk::NoRemovalFlags, QLatin1String("A"));

    // now the mtime of res:/B should have been changed
    QCOMPARE(m_model->listStatements(QUrl("res:/B"), NAO::lastModified(), Node()).allElements().count(), 1);
    QVERIFY(m_model->listStatements(QUrl("res:/B"), NAO::lastModified(), Node()).allElements().first().object().literal().toDateTime() > date);

    // the mtime of res:/C should NOT have changed
    QCOMPARE(m_model->listStatements(QUrl("res:/C"), NAO::lastModified(), Node()).allElements().count(), 1);
    QCOMPARE(m_model->listStatements(QUrl("res:/C"), NAO::lastModified(), Node()).allElements().first().object().literal().toDateTime(), date);
}

// make sure relations to removed resources are handled properly
void DataManagementModelTest::testRemoveDataByApplication_related()
{
    // first we create our apps
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    m_model->addStatement(QUrl("app:/B"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/B"), NAO::identifier(), LiteralValue(QLatin1String("B")), appG);

    // we create 3 graphs, maintained by different apps
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);
    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);
    m_model->addStatement(g2, NAO::maintainedBy(), QUrl("app:/B"), mg2);
    QUrl mg3;
    const QUrl g3 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg3);
    m_model->addStatement(g3, NAO::maintainedBy(), QUrl("app:/A"), mg3);

    const QDateTime date = QDateTime::currentDateTime();

    // create two resources:
    // A is split across both graphs
    // B is only in one graph
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("Hello World")), g2);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(date), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::lastModified(), LiteralValue(date), g1);

    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/B"), NAO::created(), LiteralValue(date), g1);
    m_model->addStatement(QUrl("res:/B"), NAO::lastModified(), LiteralValue(date), g1);

    // three relations B -> A, one in each graph
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/res"), QUrl("res:/A"), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/res2"), QUrl("res:/A"), g2);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/res3"), QUrl("res:/A"), g3);

    // a third resource which is deleted entirely but has one relation in another graph
    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/C"), NAO::created(), LiteralValue(date), g1);
    m_model->addStatement(QUrl("res:/C"), NAO::lastModified(), LiteralValue(date), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/res"), QUrl("res:/C"), g2);


    // now remove A
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A") << QUrl("res:/C"), Nepomuk::NoRemovalFlags, QLatin1String("A"));

    // now only the relation in the first graph should have been removed
    QVERIFY(!m_model->containsStatement(QUrl("res:/B"), QUrl("prop:/res"), QUrl("res:/A"), g1));
    QVERIFY(!m_model->containsStatement(QUrl("res:/B"), QUrl("prop:/res3"), QUrl("res:/A"), g3));
    QVERIFY(m_model->containsStatement(QUrl("res:/B"), QUrl("prop:/res2"), QUrl("res:/A"), g2));
    QVERIFY(!m_model->containsStatement(QUrl("res:/B"), QUrl("prop:/res3"), QUrl("res:/C"), g2));

    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/B"), QUrl("prop:/res"), QUrl("res:/A")));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/B"), QUrl("prop:/res3"), QUrl("res:/A")));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/B"), QUrl("prop:/res2"), QUrl("res:/A")));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/B"), QUrl("prop:/res3"), QUrl("res:/C")));
}

// make sure legacy indexer data (the graphs marked with indexGraphFor) is removed properly
void DataManagementModelTest::testRemoveDataByApplication_legacyIndexerData()
{
    // create our file
    QTemporaryFile fileA;
    fileA.open();
    const QUrl fileAUrl = QUrl::fromLocalFile(fileA.fileName());
    const QUrl fileARes("res:/A");

    // create the graph containing the legacy data
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::DiscardableInstanceBase(), &mg1);

    // mark the graph as being the legacy index graph
    m_model->addStatement(g1, QUrl("http://www.strigi.org/fields#indexGraphFor"), fileARes, mg1);

    // create the index data
    m_model->addStatement(fileARes, NIE::url(), fileAUrl, g1);
    m_model->addStatement(fileARes, RDF::type(), NFO::FileDataObject(), g1);
    m_model->addStatement(fileARes, RDF::type(), NIE::InformationElement(), g1);
    m_model->addStatement(fileARes, NIE::title(), LiteralValue(QLatin1String("foobar")), g1);


    // remove the information claiming to be the indexer
    QBENCHMARK_ONCE
    m_dmModel->removeDataByApplication(QList<QUrl>() << fileAUrl, NoRemovalFlags, QLatin1String("nepomukindexer"));

    // the call should succeed
    QVERIFY(!m_dmModel->lastError());

    // now make sure that everything is gone
    QVERIFY(!m_model->containsAnyStatement(fileARes, Node(), Node(), Node()));

    QVERIFY(!m_model->containsAnyStatement(Node(), Node(), Node(), g1));
    QVERIFY(!m_model->containsAnyStatement(Node(), Node(), Node(), mg1));
}

// make sure we can remove data from non-existing files
void DataManagementModelTest::testRemoveDataByApplication_deletedFile()
{
    QTemporaryFile* fileA = new QTemporaryFile();
    fileA->open();
    const KUrl fileUrl(fileA->fileName());
    delete fileA;
    QVERIFY(!QFile::exists(fileUrl.toLocalFile()));

    // create our app
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);

    // create the data graph
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    // create the resource
    m_model->addStatement(QUrl("res:/A"), NIE::url(), fileUrl, g1);
    m_model->addStatement(QUrl("res:/A"), RDF::type(), NFO::FileDataObject(), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);


    // now try removing the data
    m_dmModel->removeDataByApplication(QList<QUrl>() << fileUrl, NoRemovalFlags, QLatin1String("A"));

    // the call should succeed
    QVERIFY(!m_dmModel->lastError());

    // the resource should be gone
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(Node(), Node(), fileUrl));
}

// test that all is removed, ie. storage is clear afterwards
void DataManagementModelTest::testRemoveAllDataByApplication1()
{
    // create our app
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);

    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();

    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);
    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);
    m_model->addStatement(g2, NAO::maintainedBy(), QUrl("app:/A"), mg2);

    // create two resources to remove
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g2);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar 2")), g2);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world 2")), g2);
    m_model->addStatement(QUrl("res:/B"), NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);

    m_dmModel->removeDataByApplication(Nepomuk::NoRemovalFlags, QLatin1String("A"));

    // make sure nothing is there anymore
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/B"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(Node(), NAO::maintainedBy(), QUrl("app:/A")));

    QVERIFY(!haveTrailingGraphs());

    // everything should be as before
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

// test that other resources are not removed - the easy way
void DataManagementModelTest::testRemoveAllDataByApplication2()
{
    // create our apps
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/B"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/B"), NAO::identifier(), LiteralValue(QLatin1String("B")), appG);

    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);
    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);
    m_model->addStatement(g2, NAO::maintainedBy(), QUrl("app:/B"), mg2);

    // create two resources to remove
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar 2")), g2);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world 2")), g2);
    m_model->addStatement(QUrl("res:/B"), NAO::created(), LiteralValue(QDateTime::currentDateTime()), g2);

    m_dmModel->removeDataByApplication(Nepomuk::NoRemovalFlags, QLatin1String("A"));

    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));
    QCOMPARE(m_model->listStatements(QUrl("res:/B"), Node(), Node()).allStatements().count(), 3);
    QVERIFY(!m_model->containsAnyStatement(Node(), NAO::maintainedBy(), QUrl("app:/A")));

    QVERIFY(!haveTrailingGraphs());
}

// test that an app is simply removed as maintainer of a graph
void DataManagementModelTest::testRemoveAllDataByApplication3()
{
    // create our apps
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/B"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/B"), NAO::identifier(), LiteralValue(QLatin1String("B")), appG);

    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/B"), mg1);

    // create two resources to remove
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar 2")), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world 2")), g1);
    m_model->addStatement(QUrl("res:/B"), NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);

    m_dmModel->removeDataByApplication(Nepomuk::NoRemovalFlags, QLatin1String("A"));

    QCOMPARE(m_model->listStatements(QUrl("res:/A"), Node(), Node()).allStatements().count(), 3);
    QCOMPARE(m_model->listStatements(QUrl("res:/B"), Node(), Node()).allStatements().count(), 3);
    QVERIFY(!m_model->containsAnyStatement(Node(), NAO::maintainedBy(), QUrl("app:/A")));
    QCOMPARE(m_model->listStatements(Node(), NAO::maintainedBy(), QUrl("app:/B")).allStatements().count(), 1);

    QVERIFY(!haveTrailingGraphs());
}

// test that metadata is not removed if the resource still exists even if its in a deleted graph
void DataManagementModelTest::testRemoveAllDataByApplication4()
{
    // create our apps
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/B"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/B"), NAO::identifier(), LiteralValue(QLatin1String("B")), appG);

    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);
    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);
    m_model->addStatement(g2, NAO::maintainedBy(), QUrl("app:/B"), mg2);

    // create two resources to remove
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g2);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);

    m_dmModel->removeDataByApplication(Nepomuk::NoRemovalFlags, QLatin1String("A"));

    QCOMPARE(m_model->listStatements(QUrl("res:/A"), Node(), Node()).allStatements().count(), 3);
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NAO::created(), Soprano::Node()));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NAO::lastModified(), Soprano::Node()));
    QVERIFY(m_model->containsAnyStatement(Node(), NAO::maintainedBy(), QUrl("app:/A")));

    QVERIFY(!haveTrailingGraphs());
}


namespace {
    int push( Soprano::Model * model, Nepomuk::SimpleResource res, QUrl graph ) {
        QHashIterator<QUrl, QVariant> it( res.properties() );
        if( !res.uri().isValid() )
            return 0;

        int numPushed = 0;
        Soprano::Statement st( res.uri(), Soprano::Node(), Soprano::Node(), graph );
        while( it.hasNext() ) {
            it.next();
            st.setPredicate( it.key() );
            if(it.value().type() == QVariant::Url)
                st.setObject(it.value().toUrl());
            else
                st.setObject( Soprano::LiteralValue(it.value()) );
            if( model->addStatement( st ) == Soprano::Error::ErrorNone )
                numPushed++;
        }
        return numPushed;
    }
}


void DataManagementModelTest::testStoreResources_strigiCase()
{
    //
    // This is for testing exactly how Strigi will use storeResources ie
    // have some blank nodes ( some of which may already exists ) and a
    // main resources which does not exist
    //

    kDebug() << "Starting Strigi merge test";

    QUrl graphUri = m_nrlModel->createGraph(NRL::InstanceBase());

    Nepomuk::SimpleResource coldplay;
    coldplay.addProperty( RDF::type(), NCO::Contact() );
    coldplay.addProperty( NCO::fullname(), "Coldplay" );

    // Push it into the model with a proper uri
    coldplay.setUri( QUrl("nepomuk:/res/coldplay") );
    QVERIFY( push( m_model, coldplay, graphUri ) == coldplay.properties().size() );

    // Now keep it as a blank node
    coldplay.setUri( QUrl("_:coldplay") );

    Nepomuk::SimpleResource album;
    album.setUri( QUrl("_:XandY") );
    album.addProperty( RDF::type(), NMM::MusicAlbum() );
    album.addProperty( NIE::title(), "X&Y" );

    Nepomuk::SimpleResource res1;
    res1.setUri( QUrl("nepomuk:/res/m/Res1") );
    res1.addProperty( RDF::type(), NFO::FileDataObject() );
    res1.addProperty( RDF::type(), NMM::MusicPiece() );
    res1.addProperty( NFO::fileName(), "Yellow.mp3" );
    res1.addProperty( NMM::performer(), QUrl("_:coldplay") );
    res1.addProperty( NMM::musicAlbum(), QUrl("_:XandY") );
    Nepomuk::SimpleResourceGraph resGraph;
    resGraph << res1 << coldplay << album;

    //
    // Do the actual merging
    //
    kDebug() << "Perform the merge";
    m_dmModel->storeResources( resGraph, "TestApp" );
    QVERIFY( !m_dmModel->lastError() );

    QVERIFY( m_model->containsAnyStatement( res1.uri(), Soprano::Node(),
                                            Soprano::Node() ) );
    QVERIFY( m_model->containsAnyStatement( res1.uri(), NFO::fileName(),
                                            Soprano::LiteralValue("Yellow.mp3") ) );
    // Make sure we have the nao:created and nao:lastModified
    QVERIFY( m_model->containsAnyStatement( res1.uri(), NAO::lastModified(),
                                            Soprano::Node() ) );
    QVERIFY( m_model->containsAnyStatement( res1.uri(), NAO::created(),
                                            Soprano::Node() ) );
    kDebug() << m_model->listStatements( res1.uri(), Soprano::Node(), Soprano::Node() ).allStatements();
    // The +2 is because nao:created and nao:lastModified would have also been added
    QCOMPARE( m_model->listStatements( res1.uri(), Soprano::Node(), Soprano::Node() ).allStatements().size(),
                res1.properties().size() + 2 );

    QList< Node > objects = m_model->listStatements( res1.uri(), NMM::performer(), Soprano::Node() ).iterateObjects().allNodes();

    QVERIFY( objects.size() == 1 );
    QVERIFY( objects.first().isResource() );

    QUrl coldplayUri = objects.first().uri();
    QCOMPARE( coldplayUri, QUrl("nepomuk:/res/coldplay") );
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

    QVERIFY(!haveTrailingGraphs());
}


void DataManagementModelTest::testStoreResources_graphRules()
{
    //
    // Test the graph rules. If a resource exists in a nrl:DiscardableInstanceBase
    // and it is merged with a non-discardable graph. Then the graph should be replaced
    // In the opposite case - nothing should be done
    {
        Nepomuk::SimpleResource res;
        res.addProperty( RDF::type(), NCO::Contact() );
        res.addProperty( NCO::fullname(), "Lion" );

        QUrl graphUri = m_nrlModel->createGraph( NRL::DiscardableInstanceBase() );

        res.setUri(QUrl("nepomuk:/res/Lion"));
        QVERIFY( push( m_model, res, graphUri ) == res.properties().size() );
        res.setUri(QUrl("_:lion"));

        Nepomuk::SimpleResourceGraph resGraph;
        resGraph << res;

        QHash<QUrl, QVariant> additionalMetadata;
        additionalMetadata.insert( RDF::type(), NRL::InstanceBase() );
        m_dmModel->storeResources( resGraph, "TestApp", Nepomuk::IdentifyNew, Nepomuk::NoStoreResourcesFlags, additionalMetadata );
        QVERIFY( !m_dmModel->lastError() );

        QList<Soprano::Statement> stList = m_model->listStatements( Soprano::Node(), NCO::fullname(),
                                                            Soprano::LiteralValue("Lion") ).allStatements();
        kDebug() << stList;
        QVERIFY( stList.size() == 1 );
        Soprano::Node lionNode = stList.first().subject();
        QVERIFY( lionNode.isResource() );
        Soprano::Node graphNode = stList.first().context();
        QVERIFY( graphNode.isResource() );

        QVERIFY( !m_model->containsAnyStatement( graphNode, RDF::type(), NRL::DiscardableInstanceBase() ) );

        QVERIFY(!haveTrailingGraphs());
    }
    {
        Nepomuk::SimpleResource res;
        res.addProperty( RDF::type(), NCO::Contact() );
        res.addProperty( NCO::fullname(), "Tiger" );

        QUrl graphUri = m_nrlModel->createGraph( NRL::DiscardableInstanceBase() );

        res.setUri(QUrl("nepomuk:/res/Tiger"));
        QVERIFY( push( m_model, res, graphUri ) == res.properties().size() );
        res.setUri(QUrl("_:tiger"));

        Nepomuk::SimpleResourceGraph resGraph;
        resGraph << res;

        QHash<QUrl, QVariant> additionalMetadata;
        additionalMetadata.insert( RDF::type(), NRL::InstanceBase() );
        m_dmModel->storeResources( resGraph, "TestApp", Nepomuk::IdentifyNew, Nepomuk::NoStoreResourcesFlags, additionalMetadata );
        QVERIFY( !m_dmModel->lastError() );

        QList<Soprano::Statement> stList = m_model->listStatements( Soprano::Node(), NCO::fullname(),
                                                           Soprano::LiteralValue("Tiger") ).allStatements();
        QVERIFY( stList.size() == 1 );
        Soprano::Node TigerNode = stList.first().subject();
        QVERIFY( TigerNode.isResource() );
        Soprano::Node graphNode = stList.first().context();
        QVERIFY( graphNode.isResource() );

        QVERIFY( !m_model->containsAnyStatement( graphNode, RDF::type(), NRL::DiscardableInstanceBase() ) );

        QVERIFY(!haveTrailingGraphs());
    }
}


void DataManagementModelTest::testStoreResources_createResource()
{
    //
    // Simple case: create a resource by merging it
    //
    SimpleResource res;
    res.setUri(QUrl("_:A"));
    res.addProperty(RDF::type(), NAO::Tag());
    res.addProperty(NAO::prefLabel(), QLatin1String("Foobar"));

    m_dmModel->storeResources(SimpleResourceGraph() << res, QLatin1String("testapp"));
    QVERIFY( !m_dmModel->lastError() );

    // check if the resource exists
    QVERIFY(m_model->containsAnyStatement(Soprano::Node(), RDF::type(), NAO::Tag()));
    QVERIFY(m_model->containsAnyStatement(Soprano::Node(), NAO::prefLabel(), Soprano::LiteralValue::createPlainLiteral(QLatin1String("Foobar"))));

    // make sure only one tag resource was created
    QCOMPARE(m_model->listStatements(Node(), RDF::type(), NAO::Tag()).allElements().count(), 1);

    // get the new resources URI
    const QUrl resUri = m_model->listStatements(Node(), RDF::type(), NAO::Tag()).iterateSubjects().allNodes().first().uri();

    // check that it has the default metadata
    QVERIFY(m_model->containsAnyStatement(resUri, NAO::created(), Node()));
    QVERIFY(m_model->containsAnyStatement(resUri, NAO::lastModified(), Node()));

    // and both created and last modification date should be similar
    QCOMPARE(m_model->listStatements(resUri, NAO::created(), Node()).iterateObjects().allNodes().first(),
             m_model->listStatements(resUri, NAO::lastModified(), Node()).iterateObjects().allNodes().first());

    // check if all the correct metadata graphs exist
    // ask where {
    //  graph ?g { ?r a nao:Tag . ?r nao:prefLabel "Foobar" . } .
    //  graph ?mg { ?g a nrl:InstanceBase . ?mg a nrl:GraphMetadata . ?mg nrl:coreGraphMetadataFor ?g . } .
    //  ?g nao:maintainedBy ?a . ?a nao:identifier "testapp"
    // }
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { ?r a %1 . ?r %2 %3 . } . "
                                                      "graph ?mg { ?g a %4 . ?mg a %5 . ?mg %6 ?g . } . "
                                                      "?g %7 ?a . ?a %8 %9 . "
                                                      "}")
                                  .arg(Soprano::Node::resourceToN3(NAO::Tag()),
                                       Soprano::Node::resourceToN3(NAO::prefLabel()),
                                       Soprano::Node::literalToN3(LiteralValue::createPlainLiteral(QLatin1String("Foobar"))),
                                       Soprano::Node::resourceToN3(NRL::InstanceBase()),
                                       Soprano::Node::resourceToN3(NRL::GraphMetadata()),
                                       Soprano::Node::resourceToN3(NRL::coreGraphMetadataFor()),
                                       Soprano::Node::resourceToN3(NAO::maintainedBy()),
                                       Soprano::Node::resourceToN3(NAO::identifier()),
                                       Soprano::Node::literalToN3(QLatin1String("testapp"))),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    //
    // Now create the same resource again
    //
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();
    m_dmModel->storeResources(SimpleResourceGraph() << res, QLatin1String("testapp"));
    QVERIFY( !m_dmModel->lastError() );

    // nothing should have happened
    QCOMPARE(existingStatements, Soprano::Graph(m_model->listStatements().allStatements()));

    //
    // Now create the same resource with a different app
    //
    m_dmModel->storeResources(SimpleResourceGraph() << res, QLatin1String("testapp2"));
    QVERIFY( !m_dmModel->lastError() );

    // only one thing should have been added: the new app Agent and its role as maintainer for the existing graph
    // vHanda: Shouldn't there be a new graph, with the resources statements which hash both
    //         testapp and testapp2 as maintainers?

    //Q_FOREACH(const Soprano::Statement& s, existingStatements.toList()) {
    //    kDebug() << s;
    //    QVERIFY(m_model->containsStatement(s));
    //}

    // ask where {
    //      graph ?g { ?r a nao:Tag. ?r nao:prefLabel "Foobar" . } .
    //      ?g nao:maintainedBy ?a1 . ?a1 nao:identifier "testapp" .
    //      ?g nao:maintainedBy ?a2 . ?a2 nao:identifier "testapp2" .
    // }
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { ?r a %1 . ?r %2 %3 . } . "
                                                      "?g %4 ?a1 . ?a1 %5 %6 . "
                                                      "?g %4 ?a2 . ?a2 %5 %7 . "
                                                      "}")
                                  .arg(Soprano::Node::resourceToN3(NAO::Tag()),
                                       Soprano::Node::resourceToN3(NAO::prefLabel()),
                                       Soprano::Node::literalToN3(LiteralValue::createPlainLiteral(QLatin1String("Foobar"))),
                                       Soprano::Node::resourceToN3(NAO::maintainedBy()),
                                       Soprano::Node::resourceToN3(NAO::identifier()),
                                       Soprano::Node::literalToN3(QLatin1String("testapp")),
                                       Soprano::Node::literalToN3(QLatin1String("testapp2"))),
                                  Soprano::Query::QueryLanguageSparql).boolValue());


    // create a resource by specifying the URI
    SimpleResource res2;
    res2.setUri(QUrl("nepomuk:/res/A"));
    res2.addProperty(QUrl("prop:/string"), QVariant(QLatin1String("foobar")));
    m_dmModel->storeResources(SimpleResourceGraph() << res2, QLatin1String("testapp"));
    QVERIFY( !m_dmModel->lastError() );

    QVERIFY(m_model->containsAnyStatement( res2.uri(), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))));

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testStoreResources_invalid_args()
{
    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // empty resources -> no error but no change either
    m_dmModel->storeResources(SimpleResourceGraph(), QLatin1String("testapp"));

    // no error
    QVERIFY(!m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty app
    m_dmModel->storeResources(SimpleResourceGraph(), QString());

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // invalid resource in graph
    m_dmModel->storeResources(SimpleResourceGraph() << SimpleResource(), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // invalid range used in one resource
    SimpleResource res;
    res.setUri(QUrl("nepomuk:/res/A"));
    res.addProperty(QUrl("prop:/int"), QVariant(QLatin1String("foobar")));
    m_dmModel->storeResources(SimpleResourceGraph() << res, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // non-existing file
    const QUrl nonExistingFileUrl("file:///a/file/that/is/very/unlikely/to/exist");
    SimpleResource nonExistingFileRes;
    nonExistingFileRes.setUri(nonExistingFileUrl);
    nonExistingFileRes.addProperty(QUrl("prop:/int"), QVariant(42));
    m_dmModel->storeResources(SimpleResourceGraph() << nonExistingFileRes, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // non-existing file as object
    nonExistingFileRes.setUri(QUrl("nepomuk:/res/A"));
    nonExistingFileRes.addProperty(QUrl("prop:/res"), nonExistingFileUrl);
    m_dmModel->storeResources(SimpleResourceGraph() << nonExistingFileRes, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // invalid graph metadata 1
    SimpleResource res2;
    res.setUri(QUrl("nepomuk:/res/A"));
    res.addProperty(QUrl("prop:/int"), QVariant(42));
    QHash<QUrl, QVariant> invalidMetadata;
    invalidMetadata.insert(QUrl("prop:/int"), QLatin1String("foobar"));
    m_dmModel->storeResources(SimpleResourceGraph() << res2, QLatin1String("testapp"), Nepomuk::IdentifyNew, Nepomuk::NoStoreResourcesFlags, invalidMetadata);

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // invalid graph metadata 2
    invalidMetadata.clear();
    invalidMetadata.insert(NAO::maintainedBy(), QLatin1String("foobar"));
    m_dmModel->storeResources(SimpleResourceGraph() << res2, QLatin1String("testapp"), Nepomuk::IdentifyNew, Nepomuk::NoStoreResourcesFlags, invalidMetadata);

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

void DataManagementModelTest::testStoreResources_invalid_args_with_existing()
{
    // create a test resource
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    // create the resource to delete
    QUrl resA("nepomuk:/res/A");
    QUrl resB("nepomuk:/res/B");
    m_model->addStatement(resA, QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(resA, QUrl("prop:/res"), resB, g1);
    const QDateTime now = QDateTime::currentDateTime();
    m_model->addStatement(resA, NAO::created(), LiteralValue(now), g1);
    m_model->addStatement(resA, NAO::lastModified(), LiteralValue(now), g1);


    // create a resource to merge
    SimpleResource a(resA);
    a.addProperty(QUrl("prop:/int"), 42);


    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // empty resources -> no error but no change either
    m_dmModel->storeResources(SimpleResourceGraph(), QLatin1String("testapp"));

    // no error
    QVERIFY(!m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty app
    m_dmModel->storeResources(SimpleResourceGraph() << a, QString());

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // invalid resource in graph
    m_dmModel->storeResources(SimpleResourceGraph() << a << SimpleResource(), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // invalid range used in one resource
    SimpleResource res(a);
    res.addProperty(QUrl("prop:/int"), QVariant(QLatin1String("foobar")));
    m_dmModel->storeResources(SimpleResourceGraph() << res, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // non-existing file as object
    const QUrl nonExistingFileUrl("file:///a/file/that/is/very/unlikely/to/exist");
    SimpleResource nonExistingFileRes(a);
    nonExistingFileRes.addProperty(QUrl("prop:/res"), nonExistingFileUrl);
    m_dmModel->storeResources(SimpleResourceGraph() << nonExistingFileRes, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // invalid cardinality in resource
    SimpleResource invalidCRes(a);
    invalidCRes.addProperty(QUrl("prop:/int_c1"), 42);
    invalidCRes.addProperty(QUrl("prop:/int_c1"), 2);
    m_dmModel->storeResources(SimpleResourceGraph() << invalidCRes, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

void DataManagementModelTest::testStoreResources_file1()
{
    QTemporaryFile fileA;
    fileA.open();

    // merge a file URL
    SimpleResource r1;
    r1.setUri(QUrl::fromLocalFile(fileA.fileName()));
    r1.addProperty(RDF::type(), NAO::Tag());
    r1.addProperty(QUrl("prop:/string"), QLatin1String("Foobar"));

    m_dmModel->storeResources(SimpleResourceGraph() << r1, QLatin1String("testapp"));
    QVERIFY( !m_dmModel->lastError() );

    // a nie:url relation should have been created
    QVERIFY(m_model->containsAnyStatement(Node(), NIE::url(), QUrl::fromLocalFile(fileA.fileName())));

    // the file URL should never be used as subject
    QVERIFY(!m_model->containsAnyStatement(QUrl::fromLocalFile(fileA.fileName()), Node(), Node()));

    // make sure file URL and res URI are properly related including the properties
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { ?r %1 %4 . "
                                                      "?r a %2 . "
                                                      "?r <prop:/string> %3 . }")
                                  .arg(Node::resourceToN3(NIE::url()),
                                       Node::resourceToN3(NAO::Tag()),
                                       Node::literalToN3(LiteralValue(QLatin1String("Foobar"))),
                                       Node::resourceToN3(QUrl::fromLocalFile(fileA.fileName()))),
                                  Query::QueryLanguageSparql).boolValue());

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testStoreResources_file2()
{
    // merge a property with non-existing file value
    QTemporaryFile fileA;
    fileA.open();
    const QUrl fileUrl = QUrl::fromLocalFile(fileA.fileName());

    SimpleResource r1;
    r1.setUri(QUrl("nepomuk:/res/A"));
    r1.addProperty(QUrl("prop:/res"), fileUrl);

    m_dmModel->storeResources(SimpleResourceGraph() << r1, QLatin1String("testapp"));
    QVERIFY( !m_dmModel->lastError() );

    // the property should have been created
    QVERIFY(m_model->containsAnyStatement(QUrl("nepomuk:/res/A"), QUrl("prop:/res"), Node()));

    // but it should not be related to the file URL
    QVERIFY(!m_model->containsAnyStatement(QUrl("nepomuk:/res/A"), QUrl("prop:/res"), fileUrl));

    // there should be a nie:url for the file URL
    QVERIFY(m_model->containsAnyStatement(Node(), NIE::url(), fileUrl));

    // make sure file URL and res URI are properly related including the properties
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { <nepomuk:/res/A> <prop:/res> ?r . "
                                                      "?r %1 %2 . }")
                                  .arg(Node::resourceToN3(NIE::url()),
                                       Node::resourceToN3(fileUrl)),
                                  Query::QueryLanguageSparql).boolValue());

    QVERIFY(!haveTrailingGraphs());
}


void DataManagementModelTest::testStoreResources_file3()
{
    QTemporaryFile fileA;
    fileA.open();
    const QUrl fileUrl = QUrl::fromLocalFile(fileA.fileName());

    SimpleResource r1;
    r1.setUri( fileUrl );
    r1.addProperty( RDF::type(), QUrl("class:/typeA") );

    m_dmModel->storeResources( SimpleResourceGraph() << r1, QLatin1String("origApp") );
    QVERIFY( !m_dmModel->lastError() );

    // Make sure all the data has been added and it belongs to origApp
    QString query = QString::fromLatin1("ask { graph ?g { ?r a %6 . "
                                        " ?r %7 %1 . "
                                        " ?r a %2 . } ?g %3 ?app . ?app %4 %5 . }")
                    .arg( Soprano::Node::resourceToN3( fileUrl ),
                          Soprano::Node::resourceToN3( QUrl("class:/typeA") ),
                          Soprano::Node::resourceToN3( NAO::maintainedBy() ),
                          Soprano::Node::resourceToN3( NAO::identifier() ),
                          Soprano::Node(Soprano::LiteralValue("origApp")).toN3(),
                          Soprano::Node::resourceToN3( NFO::FileDataObject()),
                          Soprano::Node::resourceToN3( NIE::url() )
                        );

    QVERIFY( m_model->executeQuery( query, Soprano::Query::QueryLanguageSparql ).boolValue() );

    QList< Statement > stList = m_model->listStatements( Soprano::Node(), NIE::url(), fileUrl ).allStatements();
    QCOMPARE( stList.size(), 1 );

    QUrl resUri = stList.first().subject().uri();
    QVERIFY( resUri.scheme() == QLatin1String("nepomuk") );

    SimpleResource r2;
    r2.setUri( fileUrl );
    r2.addProperty( QUrl("prop:/res"), NFO::FileDataObject() );

    m_dmModel->storeResources( SimpleResourceGraph() << r2, QLatin1String("newApp") );
    QVERIFY( !m_dmModel->lastError() );

    // Make sure it was identified properly
    stList = m_model->listStatements( Soprano::Node(), NIE::url(), fileUrl ).allStatements();
    QCOMPARE( stList.size(), 1 );

    QUrl resUri2 = stList.first().subject().uri();
    QVERIFY( resUri2.scheme() == QLatin1String("nepomuk") );
    QCOMPARE( resUri, resUri2 );

    // The only statement that should have acquired "newApp" as the maintainer should be
    // r2 <prop:/res> <object:/custom>

    query = QString::fromLatin1("select ?name where { graph ?g { %1 %2 %3 . %1 a %4. }"
                                " ?g %5 ?app . ?app %6 ?name . }")
                .arg( Soprano::Node::resourceToN3( resUri ),
                      Soprano::Node::resourceToN3( NIE::url() ),
                      Soprano::Node::resourceToN3( fileUrl ),
                      Soprano::Node::resourceToN3( NFO::FileDataObject() ),
                      Soprano::Node::resourceToN3( NAO::maintainedBy() ),
                      Soprano::Node::resourceToN3( NAO::identifier() ) );

    QueryResultIterator it = m_model->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    QStringList appList;
    while( it.next() )
        appList << it["name"].literal().toString();

    QCOMPARE( appList.size(), 1 );

    //query = QString::fromLatin1("ask { )

}


void DataManagementModelTest::testStoreResources_file4()
{
    QTemporaryFile fileA;
    fileA.open();
    const QUrl fileUrl = QUrl::fromLocalFile(fileA.fileName());

    SimpleResource res;
    res.addProperty( RDF::type(), fileUrl );
    res.addProperty( QUrl("prop:/res"), fileUrl );

    m_dmModel->storeResources( SimpleResourceGraph() << res, QLatin1String("app1") );
    QVERIFY( !m_dmModel->lastError() );

    // Make sure the fileUrl got added
    QList< Statement > stList = m_model->listStatements( Node(), NIE::url(), fileUrl ).allStatements();
    QCOMPARE( stList.size(), 1 );

    QUrl fileResUri = stList.first().subject().uri();

    // Make sure the SimpleResource was stored
    stList = m_model->listStatements( Node(), RDF::type(), fileResUri ).allStatements();
    QCOMPARE( stList.size(), 1 );

    QUrl resUri = stList.first().subject().uri();

    // Check for the other statement
    stList = m_model->listStatements( resUri, QUrl("prop:/res"), Node() ).allStatements();
    QCOMPARE( stList.size(), 1 );

    QUrl fileResUri2 = stList.first().object().uri();
    QCOMPARE( fileResUri, fileResUri2 );
}


void DataManagementModelTest::testStoreResources_fileExists()
{
    SimpleResource res(QUrl("file:///a/b/v/c/c"));
    res.addType( NMM::MusicPiece() );
    res.addProperty( NAO::numericRating(), 10 );

    m_dmModel->storeResources( SimpleResourceGraph() << res, QLatin1String("app") );

    // Should give an error - The file does not exist ( probably )
    QVERIFY( m_dmModel->lastError() );
}


void DataManagementModelTest::testStoreResources_sameNieUrl()
{
    QTemporaryFile fileA;
    fileA.open();
    const QUrl fileUrl = QUrl::fromLocalFile(fileA.fileName());

    SimpleResource res;
    res.setUri( fileUrl );
    res.addProperty( RDF::type(), NFO::FileDataObject() );

    m_dmModel->storeResources( SimpleResourceGraph() << res, QLatin1String("app1") );
    QVERIFY( !m_dmModel->lastError() );

    // Make sure the fileUrl got added
    QList< Statement > stList = m_model->listStatements( Node(), NIE::url(), fileUrl ).allStatements();
    QCOMPARE( stList.size(), 1 );

    QUrl fileResUri = stList.first().subject().uri();

    // Make sure there is only one rdf:type nfo:FileDataObject
    stList = m_model->listStatements( Node(), RDF::type(), NFO::FileDataObject() ).allStatements();
    QCOMPARE( stList.size(), 1 );
    QCOMPARE( stList.first().subject().uri(), fileResUri );

    SimpleResource res2;
    res2.addProperty( NIE::url(), fileUrl );
    res2.addProperty( NAO::numericRating(), QVariant(10) );

    m_dmModel->storeResources( SimpleResourceGraph() << res2, QLatin1String("app1") );
    QVERIFY( !m_dmModel->lastError() );

    // Make sure it got mapped
    stList = m_model->listStatements( Node(), NAO::numericRating(), LiteralValue(10) ).allStatements();
    QCOMPARE( stList.size(), 1 );
    QCOMPARE( stList.first().subject().uri(), fileResUri );
}

// metadata should be ignored when merging one resource into another
void DataManagementModelTest::testStoreResources_metadata()
{
    // create our app
    const QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);

    // create a resource
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    const QDateTime now = QDateTime::currentDateTime();
    QUrl resA("nepomuk:/res/A");
    m_model->addStatement(resA, RDF::type(), NAO::Tag(), g1);
    m_model->addStatement(resA, QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(resA, QUrl("prop:/string"), LiteralValue(QLatin1String("Foobar")), g1);
    m_model->addStatement(resA, QUrl("prop:/string2"), LiteralValue(QLatin1String("Foobar2")), g1);
    m_model->addStatement(resA, NAO::created(), LiteralValue(now), g1);
    m_model->addStatement(resA, NAO::lastModified(), LiteralValue(now), g1);


    // now we merge the same resource (with differing metadata)
    SimpleResource a;
    a.addProperty(RDF::type(), NAO::Tag());
    a.addProperty(QUrl("prop:/int"), QVariant(42));
    a.addProperty(QUrl("prop:/string"), QVariant(QLatin1String("Foobar")));
    QDateTime creationDateTime(QDate(2010, 12, 24), QTime::currentTime());
    a.addProperty(NAO::created(), QVariant(creationDateTime));

    // merge the resource
    m_dmModel->storeResources(SimpleResourceGraph() << a, QLatin1String("B"));
    QVERIFY(!m_dmModel->lastError());

    // make sure no new resource has been created
    QCOMPARE(m_model->listStatements(Node(), RDF::type(), NAO::Tag()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/int"), Node()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/string"), Node()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(resA, NAO::created(), Node()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(resA, NAO::lastModified(), Node()).allStatements().count(), 1);

    // make sure the new app has been created
    QueryResultIterator it = m_model->executeQuery(QString::fromLatin1("select ?a where { ?a a %1 . ?a %2 %3 . }")
                                                   .arg(Node::resourceToN3(NAO::Agent()),
                                                        Node::resourceToN3(NAO::identifier()),
                                                        Node::literalToN3(QLatin1String("B"))),
                                                   Soprano::Query::QueryLanguageSparql);
    QVERIFY(it.next());
    const QUrl appBRes = it[0].uri();

    // make sure the data is now maintained by both apps
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { graph ?g { %1 <prop:/int> %2 . } . ?g %3 %4 . ?g %3 <app:/A> . }")
                                  .arg(Node::resourceToN3(resA),
                                       Node::literalToN3(42),
                                       Node::resourceToN3(NAO::maintainedBy()),
                                       Node::resourceToN3(appBRes)),
                                  Soprano::Query::QueryLanguageSparql).boolValue());
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { graph ?g { %1 <prop:/string> %2 . } . ?g %3 %4 . ?g %3 <app:/A> . }")
                                  .arg(Node::resourceToN3(resA),
                                       Node::literalToN3(QLatin1String("Foobar")),
                                       Node::resourceToN3(NAO::maintainedBy()),
                                       Node::resourceToN3(appBRes)),
                                  Soprano::Query::QueryLanguageSparql).boolValue());
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { graph ?g { %1 a %2 . } . ?g %3 %4 . ?g %3 <app:/A> . }")
                                  .arg(Node::resourceToN3(resA),
                                       Node::resourceToN3(NAO::Tag()),
                                       Node::resourceToN3(NAO::maintainedBy()),
                                       Node::resourceToN3(appBRes)),
                                  Soprano::Query::QueryLanguageSparql).boolValue());
    QVERIFY(!m_model->executeQuery(QString::fromLatin1("ask where { graph ?g { %1 <prop:/string2> %2 . } . ?g %3 %4 . ?g %3 <app:/A> . }")
                                  .arg(Node::resourceToN3(resA),
                                       Node::literalToN3(QLatin1String("Foobar2")),
                                       Node::resourceToN3(NAO::maintainedBy()),
                                       Node::resourceToN3(appBRes)),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    // Make sure that the nao:lastModified and nao:created have not changed
    QDateTime mod = m_model->listStatements( resA, NAO::lastModified(), Soprano::Node() ).iterateObjects().allNodes().first().literal().toDateTime();
    QDateTime creation = m_model->listStatements( resA, NAO::created(), Soprano::Node() ).iterateObjects().allNodes().first().literal().toDateTime();

    QCOMPARE( mod, now );
    // The creation date for now will always stay the same.
    // FIXME: Should we allow changes?
    QVERIFY( creation != creationDateTime );

    QVERIFY(!haveTrailingGraphs());


    // now merge the same resource with some new data - just to make sure the metadata is updated properly
    SimpleResource sResA(resA);
    sResA.addProperty(RDF::type(), NAO::Tag());
    sResA.addProperty(QUrl("prop:/int2"), 42);

    // merge the resource
    m_dmModel->storeResources(SimpleResourceGraph() << sResA, QLatin1String("B"));
    QVERIFY(!m_dmModel->lastError());

    // make sure the new data is there
    QVERIFY(m_model->containsAnyStatement(resA, QUrl("prop:/int2"), LiteralValue(42)));

    // make sure creation date did not change
    QCOMPARE(m_model->listStatements(resA, NAO::created(), Node()).allStatements().count(), 1);
    QVERIFY(m_model->containsAnyStatement(resA, NAO::created(), LiteralValue(now)));

    // make sure mtime has changed - the resource has changed
    QCOMPARE(m_model->listStatements(resA, NAO::lastModified(), Node()).allStatements().count(), 1);
    QVERIFY(m_model->listStatements(resA, NAO::lastModified(), Node()).iterateObjects().allNodes().first().literal().toDateTime() != now);
}

void DataManagementModelTest::testStoreResources_protectedTypes()
{
    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // property
    SimpleResource propertyRes(QUrl("prop:/res"));
    propertyRes.addProperty(QUrl("prop:/int"), 42);

    // store the resource
    m_dmModel->storeResources(SimpleResourceGraph() << propertyRes, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // class
    SimpleResource classRes(NRL::Graph());
    classRes.addProperty(QUrl("prop:/int"), 42);

    // store the resource
    m_dmModel->storeResources(SimpleResourceGraph() << classRes, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // graph
    SimpleResource graphRes(QUrl("graph:/onto"));
    propertyRes.addProperty(QUrl("prop:/int"), 42);

    // store the resource
    m_dmModel->storeResources(SimpleResourceGraph() << graphRes, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

// make sure storeResources ignores supertypes
void DataManagementModelTest::testStoreResources_superTypes()
{
    // 1. create a resource to merge
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    QUrl resA("nepomuk:/res/A");
    m_model->addStatement(resA, QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(resA, RDF::type(), QUrl("class:/typeA"), g1);
    m_model->addStatement(resA, RDF::type(), QUrl("class:/typeB"), g1);
    const QDateTime now = QDateTime::currentDateTime();
    m_model->addStatement(resA, NAO::created(), LiteralValue(now), g1);
    m_model->addStatement(resA, NAO::lastModified(), LiteralValue(now), g1);


    // now merge the same resource (excluding the super-type A)
    SimpleResource a;
    a.addProperty(RDF::type(), QUrl("class:/typeB"));
    a.addProperty(QUrl("prop:/string"), QLatin1String("hello world"));

    m_dmModel->storeResources(SimpleResourceGraph() << a, QLatin1String("A"));
    QVERIFY( !m_dmModel->lastError() );


    // make sure the existing resource was reused
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world"))).allElements().count(), 1);
}

// make sure merging even works with missing metadata in store
void DataManagementModelTest::testStoreResources_missingMetadata()
{
    // create our app
    const QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);

    // create a resource (without creation date)
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    const QDateTime now = QDateTime::currentDateTime();
    QUrl resA("nepomuk:/res/A");
    m_model->addStatement(resA, RDF::type(), NAO::Tag(), g1);
    m_model->addStatement(resA, QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(resA, QUrl("prop:/string"), LiteralValue(QLatin1String("Foobar")), g1);
    m_model->addStatement(resA, NAO::lastModified(), LiteralValue(now), g1);


    // now we merge the same resource
    SimpleResource a;
    a.addProperty(RDF::type(), NAO::Tag());
    a.addProperty(QUrl("prop:/int"), QVariant(42));
    a.addProperty(QUrl("prop:/string"), QVariant(QLatin1String("Foobar")));

    // merge the resource
    m_dmModel->storeResources(SimpleResourceGraph() << a, QLatin1String("B"));
    QVERIFY( !m_dmModel->lastError() );

    // make sure no new resource has been created
    QCOMPARE(m_model->listStatements(Node(), RDF::type(), NAO::Tag()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/int"), Node()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/string"), Node()).allStatements().count(), 1);

    QVERIFY(!haveTrailingGraphs());


    // now merge the same resource with some new data - just to make sure the metadata is updated properly
    SimpleResource simpleResA(resA);
    simpleResA.addProperty(RDF::type(), NAO::Tag());
    simpleResA.addProperty(QUrl("prop:/int2"), 42);

    // merge the resource
    m_dmModel->storeResources(SimpleResourceGraph() << simpleResA, QLatin1String("B"));
    QVERIFY( !m_dmModel->lastError() );

    // make sure the new data is there
    QVERIFY(m_model->containsAnyStatement(resA, QUrl("prop:/int2"), LiteralValue(42)));

    // make sure creation date did not change, ie. it was not created as that would be wrong
    QVERIFY(!m_model->containsAnyStatement(resA, NAO::created(), Node()));

    // make sure the last mtime has been updated
    QCOMPARE(m_model->listStatements(resA, NAO::lastModified(), Node()).allStatements().count(), 1);
    QDateTime newDt = m_model->listStatements(resA, NAO::lastModified(), Node()).iterateObjects().allNodes().first().literal().toDateTime();
    QVERIFY( newDt > now);

    //
    // Merge the resource again, but this time make sure it is identified as well
    //
    SimpleResource resB;
    resB.addProperty(RDF::type(), NAO::Tag());
    resB.addProperty(QUrl("prop:/int"), QVariant(42));
    resB.addProperty(QUrl("prop:/string"), QVariant(QLatin1String("Foobar")));
    resB.addProperty(QUrl("prop:/int2"), 42);
    resB.addProperty(QUrl("prop:/int3"), 50);

    // merge the resource
    m_dmModel->storeResources(SimpleResourceGraph() << resB, QLatin1String("B"));
    QVERIFY( !m_dmModel->lastError() );

    // make sure the new data is there
    QVERIFY(m_model->containsAnyStatement(resA, QUrl("prop:/int3"), LiteralValue(50)));

    // make sure creation date did not change, ie. it was not created as that would be wrong
    QVERIFY(!m_model->containsAnyStatement(resA, NAO::created(), Node()));

    // make sure the last mtime has been updated
    QCOMPARE(m_model->listStatements(resA, NAO::lastModified(), Node()).allStatements().count(), 1);
    QVERIFY(m_model->listStatements(resA, NAO::lastModified(), Node()).iterateObjects().allNodes().first().literal().toDateTime() > newDt);
}

// test merging when there is more than one candidate resource to merge with
void DataManagementModelTest::testStoreResources_multiMerge()
{
    // create two resource which could be matches for the one we will store
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase());

    // the resource in which we want to merge
    QUrl resA("nepomuk:/res/A");
    m_model->addStatement(resA, QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(resA, RDF::type(), NAO::Tag(), g1);
    m_model->addStatement(resA, QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(resA, NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);
    m_model->addStatement(resA, NAO::lastModified(), LiteralValue(QDateTime::currentDateTime()), g1);

    QUrl resB("nepomuk:/res/B");
    m_model->addStatement(resB, QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(resB, RDF::type(), NAO::Tag(), g1);
    m_model->addStatement(resB, QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(resB, NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);
    m_model->addStatement(resB, NAO::lastModified(), LiteralValue(QDateTime::currentDateTime()), g1);


    // now store the exact same resource
    SimpleResource res;
    res.addProperty(RDF::type(), NAO::Tag());
    res.addProperty(QUrl("prop:/int"), 42);
    res.addProperty(QUrl("prop:/string"), QLatin1String("foobar"));

    m_dmModel->storeResources(SimpleResourceGraph() << res, QLatin1String("A"));
    QVERIFY(!m_dmModel->lastError());

    // make sure no new resource was created
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/int"), LiteralValue(42)).allElements().count(), 2);
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))).allElements().count(), 2);

    // make sure both resources still exist
    QVERIFY(m_model->containsAnyStatement(resA, Node(), Node()));
    QVERIFY(m_model->containsAnyStatement(resB, Node(), Node()));
}

// an example from real-life which made an early version of DMS fail
void DataManagementModelTest::testStoreResources_realLife()
{
    // we deal with one file
    QTemporaryFile theFile;
    theFile.open();

    // the full data - slightly cleanup up (excluding additional video track resources)
    // this data is a combination from the file indexing service and DMS storeResources calls

    // the resource URIs used
    const QUrl fileResUri("nepomuk:/res/3ff603a5-4023-4c2f-bd89-372002a0ffd2");
    const QUrl tvSeriesUri("nepomuk:/res/e6fbe22d-bb5c-416c-a935-407a34b58c76");
    const QUrl appRes("nepomuk:/res/275907b0-c120-4581-83d5-ea9ec034dbcd");

    // the graph URIs
    // two strigi graphs due to the nie:url preservation
    const QUrl strigiG1("nepomuk:/ctx/5ca62cd0-ccff-4484-99e3-1fd9f782a3a4");
    const QUrl strigiG2("nepomuk:/ctx/9f0bac21-f8e4-4e82-b51d-e0a7585f1c8d");
    const QUrl strigiMG1("nepomuk:/ctx/0117104c-d501-48ce-badd-6f363bfde3e2");
    const QUrl strigiMG2("nepomuk:/ctx/e52c8b8a-2e32-4a27-8633-03f27fec441b");

    const QUrl dmsG1("nepomuk:/ctx/8bea556f-cacf-4f31-be73-7f7c0f14024b");
    const QUrl dmsG2("nepomuk:/ctx/374d3968-0d20-4807-8a87-d2d6b87e7de3");
    const QUrl dmsMG1("nepomuk:/ctx/9902847f-dfe8-489a-881b-4abf1707fee7");
    const QUrl dmsMG2("nepomuk:/ctx/72ef2cdf-26e7-42b6-9093-0b7b0a7c25fc");

    const QUrl appG1("nepomuk:/ctx/7dc9f013-4e45-42bf-8595-a12e78adde81");
    const QUrl appMG1("nepomuk:/ctx/1ffcb2bb-525d-4173-b211-ebdf28c0897b");

    // strings we reuse
    const QString seriesTitle("Nepomuk The Series");
    const QString episodeTitle("Who are you?");
    const QString seriesOverview("Nepomuk is a series about information and the people needing this information for informational purposes.");
    const QString episodeOverview("The series pilot focusses on this and that, nothing in particular and it not really that interesting at all.");

    // the file resource
    m_model->addStatement(fileResUri, NIE::isPartOf(), QUrl("nepomuk:/res/e9f85f29-150d-49b6-9ffb-264ae7ec3864"), strigiG1);
    m_model->addStatement(fileResUri, NIE::contentSize(), Soprano::LiteralValue::fromString("369532928", XMLSchema::xsdInt()), strigiG1);
    m_model->addStatement(fileResUri, NIE::mimeType(), Soprano::LiteralValue::fromString("audio/x-riff", XMLSchema::string()), strigiG1);
    m_model->addStatement(fileResUri, NIE::mimeType(), Soprano::LiteralValue::fromString("video/x-msvideo", XMLSchema::string()), strigiG1);
    m_model->addStatement(fileResUri, NFO::fileName(), Soprano::LiteralValue(KUrl(theFile.fileName()).fileName()), strigiG1);
    m_model->addStatement(fileResUri, NIE::lastModified(), Soprano::LiteralValue::fromString("2010-06-29T15:44:44Z", XMLSchema::dateTime()), strigiG1);
    m_model->addStatement(fileResUri, NFO::codec(), Soprano::LiteralValue::fromString("MP3", XMLSchema::string()), strigiG1);
    m_model->addStatement(fileResUri, NFO::codec(), Soprano::LiteralValue::fromString("xvid", XMLSchema::string()), strigiG1);
    m_model->addStatement(fileResUri, NFO::averageBitrate(), Soprano::LiteralValue::fromString("1132074", XMLSchema::xsdInt()), strigiG1);
    m_model->addStatement(fileResUri, NFO::duration(), Soprano::LiteralValue::fromString("2567", XMLSchema::xsdInt()), strigiG1);
    m_model->addStatement(fileResUri, NFO::duration(), Soprano::LiteralValue::fromString("2611", XMLSchema::xsdInt()), strigiG1);
    m_model->addStatement(fileResUri, NFO::frameRate(), Soprano::LiteralValue::fromString("23", XMLSchema::xsdInt()), strigiG1);
    m_model->addStatement(fileResUri, NIE::hasPart(), QUrl("nepomuk:/res/b805e3bb-db13-4561-b457-8da8d13ce34d"), strigiG1);
    m_model->addStatement(fileResUri, NIE::hasPart(), QUrl("nepomuk:/res/c438df3c-1446-4931-9d9e-3665567025b9"), strigiG1);
    m_model->addStatement(fileResUri, NFO::horizontalResolution(), Soprano::LiteralValue::fromString("624", XMLSchema::xsdInt()), strigiG1);
    m_model->addStatement(fileResUri, NFO::verticalResolution(), Soprano::LiteralValue::fromString("352", XMLSchema::xsdInt()), strigiG1);

    m_model->addStatement(fileResUri, RDF::type(), NFO::FileDataObject(), strigiG2);
    m_model->addStatement(fileResUri, NIE::url(), QUrl::fromLocalFile(theFile.fileName()), strigiG2);

    m_model->addStatement(fileResUri, RDF::type(), NMM::TVShow(), dmsG1);
    m_model->addStatement(fileResUri, NAO::created(), Soprano::LiteralValue::fromString("2011-03-14T10:06:38.317Z", XMLSchema::dateTime()), dmsG1);
    m_model->addStatement(fileResUri, NIE::title(), Soprano::LiteralValue(episodeTitle), dmsG1);
    m_model->addStatement(fileResUri, NAO::lastModified(), Soprano::LiteralValue::fromString("2011-03-14T10:06:38.317Z", XMLSchema::dateTime()), dmsG1);
    m_model->addStatement(fileResUri, NMM::synopsis(), Soprano::LiteralValue(episodeOverview), dmsG1);
    m_model->addStatement(fileResUri, NMM::series(), tvSeriesUri, dmsG1);
    m_model->addStatement(fileResUri, NMM::season(), Soprano::LiteralValue::fromString("1", XMLSchema::xsdInt()), dmsG1);
    m_model->addStatement(fileResUri, NMM::episodeNumber(), Soprano::LiteralValue::fromString("1", XMLSchema::xsdInt()), dmsG1);
    m_model->addStatement(fileResUri, RDF::type(), NIE::InformationElement(), dmsG2);
    m_model->addStatement(fileResUri, RDF::type(), NFO::Video(), dmsG2);

    // the TV Series resource
    m_model->addStatement(tvSeriesUri, RDF::type(), NMM::TVSeries(), dmsG1);
    m_model->addStatement(tvSeriesUri, NAO::created(), Soprano::LiteralValue::fromString("2011-03-14T10:06:38.317Z", XMLSchema::dateTime()), dmsG1);
    m_model->addStatement(tvSeriesUri, NIE::title(), Soprano::LiteralValue(seriesTitle), dmsG1);
    m_model->addStatement(tvSeriesUri, NAO::lastModified(), Soprano::LiteralValue::fromString("2011-03-14T10:06:38.317Z", XMLSchema::dateTime()), dmsG1);
    m_model->addStatement(tvSeriesUri, NIE::description(), Soprano::LiteralValue(seriesOverview), dmsG1);
    m_model->addStatement(tvSeriesUri, NMM::hasEpisode(), fileResUri, dmsG1);
    m_model->addStatement(tvSeriesUri, RDF::type(), NIE::InformationElement(), dmsG2);

    // the app that called storeResources
    m_model->addStatement(appRes, RDF::type(), NAO::Agent(), appG1);
    m_model->addStatement(appRes, NAO::prefLabel(), Soprano::LiteralValue::fromString("Nepomuk TVNamer", XMLSchema::string()), appG1);
    m_model->addStatement(appRes, NAO::identifier(), Soprano::LiteralValue::fromString("nepomuktvnamer", XMLSchema::string()), appG1);

    // all the graph metadata
    m_model->addStatement(strigiG1, RDF::type(), NRL::DiscardableInstanceBase(), strigiMG1);
    m_model->addStatement(strigiG1, NAO::created(), Soprano::LiteralValue::fromString("2010-10-22T14:13:42.204Z", XMLSchema::dateTime()), strigiMG1);
    m_model->addStatement(strigiG1, QUrl("http://www.strigi.org/fields#indexGraphFor"), fileResUri, strigiMG1);
    m_model->addStatement(strigiMG1, RDF::type(), NRL::GraphMetadata(), strigiMG1);
    m_model->addStatement(strigiMG1, NRL::coreGraphMetadataFor(), strigiG1, strigiMG1);

    m_model->addStatement(strigiG2, RDF::type(), NRL::InstanceBase(), strigiMG2);
    m_model->addStatement(strigiG2, NAO::created(), Soprano::LiteralValue::fromString("2010-10-22T14:13:42.204Z", XMLSchema::dateTime()), strigiMG2);
    m_model->addStatement(strigiG2, QUrl("http://www.strigi.org/fields#indexGraphFor"), fileResUri, strigiMG2);
    m_model->addStatement(strigiG2, NAO::maintainedBy(), appRes, strigiMG2);
    m_model->addStatement(strigiMG2, RDF::type(), NRL::GraphMetadata(), strigiMG2);
    m_model->addStatement(strigiMG2, NRL::coreGraphMetadataFor(), strigiG2, strigiMG2);

    m_model->addStatement(dmsG1, RDF::type(), NRL::InstanceBase(), dmsMG1);
    m_model->addStatement(dmsG1, NAO::created(), Soprano::LiteralValue::fromString("2011-03-14T10:06:38.343Z", XMLSchema::dateTime()), dmsMG1);
    m_model->addStatement(dmsG1, NAO::maintainedBy(), appRes, dmsMG1);
    m_model->addStatement(dmsMG1, RDF::type(), NRL::GraphMetadata(), dmsMG1);
    m_model->addStatement(dmsMG1, NRL::coreGraphMetadataFor(), dmsG1, dmsMG1);

    m_model->addStatement(dmsG2, RDF::type(), NRL::InstanceBase(), dmsMG2);
    m_model->addStatement(dmsG2, NAO::created(), Soprano::LiteralValue::fromString("2011-03-14T10:06:38.621Z", XMLSchema::dateTime()), dmsMG2);
    m_model->addStatement(dmsG2, NAO::maintainedBy(), appRes, dmsMG2);
    m_model->addStatement(dmsMG2, RDF::type(), NRL::GraphMetadata(), dmsMG2);
    m_model->addStatement(dmsMG2, NRL::coreGraphMetadataFor(), dmsG2, dmsMG2);

    m_model->addStatement(appG1, RDF::type(), NRL::InstanceBase(), appMG1);
    m_model->addStatement(appG1, NAO::created(), Soprano::LiteralValue::fromString("2011-03-12T17:48:44.307Z", XMLSchema::dateTime()), appMG1);
    m_model->addStatement(appMG1, RDF::type(), NRL::GraphMetadata(), appMG1);
    m_model->addStatement(appMG1, NRL::coreGraphMetadataFor(), appG1, appMG1);


    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // now the TV show information is stored again
    SimpleResourceGraph graph;
    SimpleResource tvShowRes(QUrl::fromLocalFile(theFile.fileName()));
    tvShowRes.addProperty(RDF::type(), NMM::TVShow());
    tvShowRes.addProperty(NMM::episodeNumber(), 1);
    tvShowRes.addProperty(NMM::season(), 1);
    tvShowRes.addProperty(NIE::title(), episodeTitle);
    tvShowRes.addProperty(NMM::synopsis(), episodeOverview);

    SimpleResource tvSeriesRes;
    tvSeriesRes.addProperty(RDF::type(), NMM::TVSeries());
    tvSeriesRes.addProperty(NIE::title(), seriesTitle);
    tvSeriesRes.addProperty(NIE::description(), seriesOverview);
    tvSeriesRes.addProperty(NMM::hasEpisode(), tvShowRes.uri());

    tvShowRes.addProperty(NMM::series(), tvSeriesRes.uri());

    graph << tvShowRes << tvSeriesRes;

    m_dmModel->storeResources(graph, QLatin1String("nepomuktvnamer"));
    QVERIFY(!m_dmModel->lastError());

    // now test the data - nothing should have changed at all
    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

void DataManagementModelTest::testStoreResources_trivialMerge()
{
    // we create a resource with some properties
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase());

    QUrl resA("nepomuk:/res/A");
    m_model->addStatement(resA, RDF::type(), QUrl("class:/typeA"), g1);
    m_model->addStatement(resA, QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(resA, QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(resA, NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);
    m_model->addStatement(resA, NAO::lastModified(), LiteralValue(QDateTime::currentDateTime()), g1);


    // now we store a trivial resource
    SimpleResource res;
    res.addProperty(RDF::type(), QUrl("class:/typeA"));

    m_dmModel->storeResources(SimpleResourceGraph() << res, QLatin1String("A"));
    QVERIFY( !m_dmModel->lastError() );

    // the two resources should NOT have been merged
    QCOMPARE(m_model->listStatements(Node(), RDF::type(), QUrl("class:/typeA")).allElements().count(), 2);
}

// make sure that two resources are not merged if they have no matching type even if the rest of the idenfifying props match.
// the merged resource does not have any type
void DataManagementModelTest::testStoreResources_noTypeMatch1()
{
    // we create a resource with some properties
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase());

    QUrl resA("nepomuk:/res/A");
    m_model->addStatement(resA, RDF::type(), QUrl("class:/typeA"), g1);
    m_model->addStatement(resA, QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(resA, QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(resA, NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);
    m_model->addStatement(resA, NAO::lastModified(), LiteralValue(QDateTime::currentDateTime()), g1);

    // now we store the resource without a type
    SimpleResource res;
    res.addProperty(QUrl("prop:/int"), 42);
    res.addProperty(QUrl("prop:/string"), QLatin1String("foobar"));

    m_dmModel->storeResources(SimpleResourceGraph() << res, QLatin1String("A"));
    QVERIFY( !m_dmModel->lastError() );

    // the two resources should NOT have been merged - we should have a new resource
    QCOMPARE(m_model->listStatements(Soprano::Node(), QUrl("prop:/int"), Soprano::LiteralValue(42)).allStatements().count(), 2);
    QCOMPARE(m_model->listStatements(Soprano::Node(), QUrl("prop:/string"), Soprano::LiteralValue(QLatin1String("foobar"))).allStatements().count(), 2);

    // two different subjects
    QCOMPARE(m_model->listStatements(Soprano::Node(), QUrl("prop:/int"), Soprano::LiteralValue(42)).iterateSubjects().allNodes().toSet().count(), 2);
}

// make sure that two resources are not merged if they have no matching type even if the rest of the idenfifying props match.
// the merged resource has a different type than the one in store
void DataManagementModelTest::testStoreResources_noTypeMatch2()
{
    // we create a resource with some properties
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase());

    QUrl resA("nepomuk:/res/A");
    m_model->addStatement(resA, RDF::type(), QUrl("class:/typeA"), g1);
    m_model->addStatement(resA, QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(resA, QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(resA, NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);
    m_model->addStatement(resA, NAO::lastModified(), LiteralValue(QDateTime::currentDateTime()), g1);

    // now we store the resource with a different type
    SimpleResource res;
    res.addType(QUrl("class:/typeB"));
    res.addProperty(QUrl("prop:/int"), 42);
    res.addProperty(QUrl("prop:/string"), QLatin1String("foobar"));

    m_dmModel->storeResources(SimpleResourceGraph() << res, QLatin1String("A"));
    QVERIFY( !m_dmModel->lastError() );

    // the two resources should NOT have been merged - we should have a new resource
    QCOMPARE(m_model->listStatements(Soprano::Node(), QUrl("prop:/int"), Soprano::LiteralValue(42)).allStatements().count(), 2);
    QCOMPARE(m_model->listStatements(Soprano::Node(), QUrl("prop:/string"), Soprano::LiteralValue(QLatin1String("foobar"))).allStatements().count(), 2);

    // two different subjects
    QCOMPARE(m_model->listStatements(Soprano::Node(), QUrl("prop:/int"), Soprano::LiteralValue(42)).iterateSubjects().allNodes().toSet().count(), 2);
}

void DataManagementModelTest::testStoreResources_faultyMetadata()
{
    KTemporaryFile file;
    file.open();
    const QUrl fileUrl( file.fileName() );

    SimpleResource res;
    res.addProperty( RDF::type(), NFO::FileDataObject() );
    res.addProperty( NIE::url(), fileUrl );
    res.addProperty( NAO::lastModified(), QVariant( 5 ) );
    res.addProperty( NAO::created(), QVariant(QLatin1String("oh no") ) );

    QList<Soprano::Statement> list = m_model->listStatements().allStatements();
    m_dmModel->storeResources( SimpleResourceGraph() << res, QLatin1String("testApp") );

    // The should be an error
    QVERIFY(m_dmModel->lastError());

    // And the statements should not exist
    QVERIFY(!m_model->containsAnyStatement( Node(), RDF::type(), NFO::FileDataObject() ));
    QVERIFY(!m_model->containsAnyStatement( Node(), NIE::url(), fileUrl ));
    QVERIFY(!m_model->containsAnyStatement( Node(), NAO::lastModified(), Node() ));
    QVERIFY(!m_model->containsAnyStatement( Node(), NAO::created(), Node() ));

    QList<Soprano::Statement> list2 = m_model->listStatements().allStatements();
    QCOMPARE( list, list2 );
}

void DataManagementModelTest::testStoreResources_additionalMetadataApp()
{
    KTemporaryFile file;
    file.open();
    const QUrl fileUrl( file.fileName() );

    SimpleResource res;
    res.addProperty( RDF::type(), NFO::FileDataObject() );
    res.addProperty( NIE::url(), fileUrl );

    SimpleResource app;
    app.addProperty( RDF::type(), NAO::Agent() );
    app.addProperty( NAO::identifier(), "appB" );

    SimpleResourceGraph g;
    g << res << app;

    QHash<QUrl, QVariant> additionalMetadata;
    additionalMetadata.insert( NAO::maintainedBy(), app.uri() );

    m_dmModel->storeResources( g, QLatin1String("appA"), Nepomuk::IdentifyNew, Nepomuk::NoStoreResourcesFlags, additionalMetadata );

    //FIXME: for now this should fail as nao:maintainedBy is protected,
    //       but what if we want to add some additionalMetadata which references
    //       a node in the SimpleResourceGraph. There needs to be a test for that.
    QVERIFY(m_dmModel->lastError());
}

void DataManagementModelTest::testStoreResources_itemUris()
{
    SimpleResourceGraph g;

    for (int i = 0; i < 10; i++) {
        QUrl uri( "testuri:?item="+QString::number(i) );
        SimpleResource r(uri);
        r.addType( NIE::DataObject() );
        r.addType( NIE::InformationElement() );

        QString label = QLatin1String("label") + QString::number(i);
        r.setProperty( NAO::prefLabel(), label );
        g.insert(r);
    }

    m_dmModel->storeResources( g, "app" );

    // Should give an error 'testuri' is an unknown protocol
    QVERIFY(m_dmModel->lastError());
}

void DataManagementModelTest::testStoreResources_kioProtocols()
{
    QStringList protocolList = KProtocolInfo::protocols();
    protocolList.removeAll( QLatin1String("nepomuk") );
    protocolList.removeAll( QLatin1String("file") );

    kDebug() << "List: " << protocolList;
    foreach( const QString& protocol, protocolList ) {
        SimpleResource res( QUrl(protocol + ":/item") );
        res.addType( NFO::FileDataObject() );
        res.addType( NMM::MusicPiece() );

        m_dmModel->storeResources( SimpleResourceGraph() << res, QLatin1String("app") );
        QVERIFY(!m_dmModel->lastError());

        QVERIFY( m_model->containsAnyStatement( Node(), NIE::url(), res.uri() ) );

        const QUrl resUri = m_model->listStatements( Node(), NIE::url(), res.uri() ).allStatements().first().subject().uri();

        QVERIFY( m_model->containsAnyStatement( resUri, RDF::type(), NFO::FileDataObject() ) );
        QVERIFY( m_model->containsAnyStatement( resUri, RDF::type(), NMM::MusicPiece() ) );
    }
}


void DataManagementModelTest::testStoreResources_duplicates()
{
    KTemporaryFile file;
    file.open();
    const QUrl fileUrl( file.fileName() );

    SimpleResource res;
    res.addType( NFO::FileDataObject() );
    res.addProperty( NIE::url(), fileUrl );

    SimpleResource hash1;
    hash1.addType( NFO::FileHash() );
    hash1.addProperty( NFO::hashAlgorithm(), QLatin1String("SHA1") );
    hash1.addProperty( NFO::hashValue(), QLatin1String("ddaa6b339428b75ee1545f80f1f35fb89c166bf9") );

    SimpleResource hash2;
    hash2.addType( NFO::FileHash() );
    hash2.addProperty( NFO::hashAlgorithm(), QLatin1String("SHA1") );
    hash2.addProperty( NFO::hashValue(), QLatin1String("ddaa6b339428b75ee1545f80f1f35fb89c166bf9") );

    res.addProperty( NFO::hasHash(), hash1.uri() );
    res.addProperty( NFO::hasHash(), hash2.uri() );

    SimpleResourceGraph graph;
    graph << res << hash1 << hash2;

    m_dmModel->storeResources( graph, "appA" );
    QVERIFY(!m_dmModel->lastError());

    // hash1 and hash2 are the same, they should have been merged together
    int hashCount = m_model->listStatements( Node(), RDF::type(), NFO::FileHash() ).allStatements().size();
    QCOMPARE( hashCount, 1 );

    // res should have only have one hash1
    QCOMPARE( m_model->listStatements( Node(), NFO::hasHash(), Node() ).allStatements().size(), 1 );

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testStoreResources_overwriteProperties()
{
    SimpleResource contact;
    contact.addType( NCO::Contact() );
    contact.addProperty( NCO::fullname(), QLatin1String("Spiderman") );

    m_dmModel->storeResources( SimpleResourceGraph() << contact, QLatin1String("app") );
    QVERIFY( !m_dmModel->lastError() );

    QList< Statement > stList = m_model->listStatements( Node(), RDF::type(), NCO::Contact() ).allStatements();
    QCOMPARE( stList.size(), 1 );

    const QUrl resUri = stList.first().subject().uri();

    SimpleResource contact2( resUri );
    contact2.addType( NCO::Contact() );
    contact2.addProperty( NCO::fullname(), QLatin1String("Peter Parker") );

    //m_dmModel->storeResources( SimpleResourceGraph() << contact2, QLatin1String("app") );
    //QVERIFY( m_dmModel->lastError() ); // should fail without the merge flags

    // Now everyone will know who Spiderman really is
    m_dmModel->storeResources( SimpleResourceGraph() << contact2, QLatin1String("app"), IdentifyNew, OverwriteProperties );
    QVERIFY( !m_dmModel->lastError() );

    stList = m_model->listStatements( resUri, NCO::fullname(), Node() ).allStatements();
    QCOMPARE( stList.size(), 1 );

    QString newName = stList.first().object().literal().toString();
    QCOMPARE( newName, QLatin1String("Peter Parker") );
}

// make sure that already existing resource types are taken into account for domain checks
void DataManagementModelTest::testStoreResources_correctDomainInStore()
{
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase());

    // create the resource
    const QUrl resA("nepomuk:/res/A");
    m_model->addStatement(resA, RDF::type(), NMM::MusicPiece(), g1);
    m_model->addStatement(resA, NAO::lastModified(), Soprano::LiteralValue(QDateTime::currentDateTime()), g1);

    // now store a music piece with a performer.
    // the performer does not have a type in the simple res but only in store
    SimpleResource piece(resA);
    piece.addProperty(NIE::title(), QLatin1String("Hello World"));
    SimpleResource artist;
    artist.addType(NCO::Contact());
    artist.addProperty(NCO::fullname(), QLatin1String("foobar"));
    piece.addProperty(NMM::performer(), artist);

    m_dmModel->storeResources(SimpleResourceGraph() << piece << artist, QLatin1String("testapp"));

    QVERIFY(!m_dmModel->lastError());
}

// make sure that already existing resource types are taken into account for range checks
void DataManagementModelTest::testStoreResources_correctRangeInStore()
{
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase());

    // create the resource
    const QUrl resA("nepomuk:/res/A");
    m_model->addStatement(resA, RDF::type(), NCO::Contact(), g1);
    m_model->addStatement(resA, NAO::lastModified(), Soprano::LiteralValue(QDateTime::currentDateTime()), g1);

    // now store a music piece with a performer.
    // the performer does not have a type in the simple res but only in store
    SimpleResource piece;
    piece.addType(NMM::MusicPiece());
    piece.addProperty(NIE::title(), QLatin1String("Hello World"));
    SimpleResource artist(resA);
    artist.addProperty(NCO::fullname(), QLatin1String("foobar"));
    piece.addProperty(NMM::performer(), artist);

    m_dmModel->storeResources(SimpleResourceGraph() << piece << artist, QLatin1String("testapp"));

    QVERIFY(!m_dmModel->lastError());
}

// make sure that the same values are simply merged even if encoded differently
void DataManagementModelTest::testStoreResources_duplicateValuesAsString()
{
    SimpleResource res;

    // add the same type twice
    res.addType(QUrl("class:/typeA"));
    res.addProperty(RDF::type(), QLatin1String("class:/typeA"));

    // add the same value twice
    res.addProperty(QUrl("prop:/int"), 42);
    res.addProperty(QUrl("prop:/int"), QLatin1String("42"));

    // now add the resource
    m_dmModel->storeResources(SimpleResourceGraph() << res, QLatin1String("testapp"));

    // this should succeed
    QVERIFY(!m_dmModel->lastError());

    // make sure all is well
    QCOMPARE(m_model->listStatements(Soprano::Node(), RDF::type(), QUrl("class:/typeA")).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(Soprano::Node(), QUrl("prop:/int"), LiteralValue(42)).allStatements().count(), 1);
}

void DataManagementModelTest::testStoreResources_ontology()
{
    SimpleResource res( NFO::FileDataObject() );
    res.addType( NCO::Contact() );

    m_dmModel->storeResources(SimpleResourceGraph() << res, QLatin1String("testapp"));

    // There should be some error, we're trying to set an ontology
    QVERIFY( m_dmModel->lastError() );
}

void DataManagementModelTest::testStoreResources_legacyUris()
{
    const QUrl uri("res:/A");

    const QUrl graphUri = m_nrlModel->createGraph( NRL::InstanceBase() );
    m_model->addStatement( uri, RDF::type(), NFO::FileDataObject(), graphUri );
    m_model->addStatement( uri, RDF::type(), NFO::Folder(), graphUri );
    m_model->addStatement( uri, NAO::numericRating(), LiteralValue(5), graphUri );

    SimpleResource res( uri );
    res.addType( NFO::Folder() );
    res.addType( NFO::FileDataObject() );
    res.addProperty( NAO::numericRating(), QLatin1String("5") );

    m_dmModel->storeResources( SimpleResourceGraph() << res, QLatin1String("app"), IdentifyAll, OverwriteProperties );
    QVERIFY( !m_dmModel->lastError() );

    QVERIFY( m_model->containsAnyStatement( uri, NAO::numericRating(), LiteralValue(5) ) );

    SimpleResource res2;
    res2.addType( NFO::FileDataObject() );
    res2.addProperty( NIE::isPartOf(), uri );

    m_dmModel->storeResources( SimpleResourceGraph() << res2, QLatin1String("app") );
    QVERIFY( !m_dmModel->lastError() );

    QVERIFY( m_model->containsAnyStatement( Node(), NIE::isPartOf(), uri ) );
}

void DataManagementModelTest::testStoreResources_lazyCardinalities()
{
    SimpleResource res;
    res.addType( NCO::Contact() );
    res.addProperty( NCO::fullname(), QLatin1String("Superman") );
    res.addProperty( NCO::fullname(), QLatin1String("Clark Kent") ); // Don't tell Lex!

    m_dmModel->storeResources( SimpleResourceGraph() << res, QLatin1String("testApp"),
                               Nepomuk::IdentifyAll, Nepomuk::LazyCardinalities );

    // There shouldn't be any error, even though nco:fullname has maxCardinality = 1
    QVERIFY( !m_dmModel->lastError() );

    QList< Statement > stList = m_model->listStatements( Node(), NCO::fullname(), Node() ).allStatements();
    QCOMPARE( stList.size(), 1 );

    QString name = stList.first().object().literal().toString();
    bool isClark = ( name == QLatin1String("Clark Kent") );
    bool isSuperMan = ( name == QLatin1String("Superman") );

    QVERIFY( isClark || isSuperMan );

    QUrl resUri = stList.first().subject().uri();

    // Lets try this again
    m_dmModel->storeResources( SimpleResourceGraph() << res, QLatin1String("testApp"),
                               Nepomuk::IdentifyAll, Nepomuk::LazyCardinalities );

    stList = m_model->listStatements( Node(), NCO::fullname(), Node() ).allStatements();
    QCOMPARE( stList.size(), 1 );

    name = stList.first().object().literal().toString();
    isClark = ( name == QLatin1String("Clark Kent") );
    isSuperMan = ( name == QLatin1String("Superman") );

    QVERIFY( isClark || isSuperMan );
}

void DataManagementModelTest::testMergeResources()
{
    // first we need to create the two resources we want to merge as well as one that should not be touched
    // for this simple test we put everything into one graph
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);

    // the resource in which we want to merge
    QUrl resA("nepomuk:/res/A");
    m_model->addStatement(resA, QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(resA, QUrl("prop:/int_c1"), LiteralValue(42), g1);
    m_model->addStatement(resA, QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);

    // the resource that is going to be merged
    // one duplicate property and one that differs, one backlink to ignore,
    // one property with cardinality 1 to ignore
    QUrl resB("nepomuk:/res/B");
    m_model->addStatement(resB, QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(resB, QUrl("prop:/int_c1"), LiteralValue(12), g1);
    m_model->addStatement(resB, QUrl("prop:/string"), LiteralValue(QLatin1String("hello")), g1);
    m_model->addStatement(resA, QUrl("prop:/res"), resB, g1);

    // resource C to ignore (except the backlink which needs to be updated)
    QUrl resC("nepomuk:/res/C");
    m_model->addStatement(resC, QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(resC, QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(resC, QUrl("prop:/res"), resB, g1);


    // now merge the resources
    m_dmModel->mergeResources(resA, resB, QLatin1String("A"));

    // make sure B is gone
    QVERIFY(!m_model->containsAnyStatement(resB, Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(Node(), Node(), resB));

    // make sure A has all the required properties
    QVERIFY(m_model->containsAnyStatement(resA, QUrl("prop:/int"), LiteralValue(42)));
    QVERIFY(m_model->containsAnyStatement(resA, QUrl("prop:/int_c1"), LiteralValue(42)));
    QVERIFY(m_model->containsAnyStatement(resA, QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))));
    QVERIFY(m_model->containsAnyStatement(resA, QUrl("prop:/string"), LiteralValue(QLatin1String("hello"))));

    // make sure A has no superfluous properties
    QVERIFY(!m_model->containsAnyStatement(resA, QUrl("prop:/int_c1"), LiteralValue(12)));
    QCOMPARE(m_model->listStatements(resA, QUrl("prop:/int"), Node()).allElements().count(), 1);

    // make sure the backlink was updated
    QVERIFY(m_model->containsAnyStatement(resC, QUrl("prop:/res"), resA));

    // make sure C was not touched apart from the backlink
    QVERIFY(m_model->containsStatement(resC, QUrl("prop:/int"), LiteralValue(42), g1));
    QVERIFY(m_model->containsStatement(resC, QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1));

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testMergeResources_protectedTypes()
{
    // create one resource to be merged with something else
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);

    QUrl resA("res:/A");
    m_model->addStatement(resA, RDF::type(), NAO::Tag(), g1);
    m_model->addStatement(resA, QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(resA, QUrl("prop:/string"), LiteralValue(QLatin1String("Foobar")), g1);
    m_model->addStatement(resA, NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);


    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // property 1
    m_dmModel->mergeResources(resA, QUrl("prop:/int"), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // property 2
    m_dmModel->mergeResources(QUrl("prop:/int"), resA, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // class 1
    m_dmModel->mergeResources(resA, NRL::Graph(), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // property 2
    m_dmModel->mergeResources(NRL::Graph(), resA, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // graph 1
    m_dmModel->mergeResources(resA, QUrl("graph:/onto"), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // graph 2
    m_dmModel->mergeResources(QUrl("graph:/onto"), resA, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

void DataManagementModelTest::testDescribeResources()
{
    QTemporaryFile fileC;
    fileC.open();

    // create some resources
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase());

    m_model->addStatement(QUrl("res:/A"), RDF::type(), NAO::Tag(), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/B"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/B"), g1);

    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);

    m_model->addStatement(QUrl("res:/C"), NIE::url(), QUrl::fromLocalFile(fileC.fileName()), g1);
    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/C"), NAO::hasSubResource(), QUrl("res:/D"), g1);

    m_model->addStatement(QUrl("res:/D"), QUrl("prop:/string"), LiteralValue(QLatin1String("Hello")), g1);


    // get one resource without sub-res
    QList<SimpleResource> g = m_dmModel->describeResources(QList<QUrl>() << QUrl("res:/A"), false).toList();

    // no error
    QVERIFY(!m_dmModel->lastError());

    // only one resource in the result
    QCOMPARE(g.count(), 1);

    // the one result is res:/A
    QCOMPARE(g.first().uri(), QUrl("res:/A"));

    // res:/A has 3 properties
    QCOMPARE(g.first().properties().count(), 3);


    // get one resource by file-url without sub-res
    g = m_dmModel->describeResources(QList<QUrl>() << QUrl::fromLocalFile(fileC.fileName()), false).toList();

    // no error
    QVERIFY(!m_dmModel->lastError());

    // only one resource in the result
    QCOMPARE(g.count(), 1);

    // the one result is res:/C
    QCOMPARE(g.first().uri(), QUrl("res:/C"));

    // res:/C has 3 properties
    QCOMPARE(g.first().properties().count(), 3);


    // get one resource with sub-res
    g = m_dmModel->describeResources(QList<QUrl>() << QUrl("res:/A"), true).toList();

    // no error
    QVERIFY(!m_dmModel->lastError());

    // only one resource in the result
    QCOMPARE(g.count(), 2);

    // the results are res:/A and res:/B
    SimpleResource r1 = g.first();
    SimpleResource r2 = g.back();
    QVERIFY(r1.uri() == QUrl("res:/A") || r2.uri() == QUrl("res:/A"));
    QVERIFY(r1.uri() == QUrl("res:/B") || r2.uri() == QUrl("res:/B"));

    // res:/A has 3 properties
    if(r1.uri() == QUrl("res:/A")) {
        QCOMPARE(r1.properties().count(), 3);
        QCOMPARE(r2.properties().count(), 1);
    }
    else {
        QCOMPARE(r1.properties().count(), 1);
        QCOMPARE(r2.properties().count(), 3);
    }


    // get one resource via file URL with sub-res
    g = m_dmModel->describeResources(QList<QUrl>() << QUrl::fromLocalFile(fileC.fileName()), true).toList();

    // no error
    QVERIFY(!m_dmModel->lastError());

    // only one resource in the result
    QCOMPARE(g.count(), 2);

    // the results are res:/C and res:/D
    r1 = g.first();
    r2 = g.back();
    QVERIFY(r1.uri() == QUrl("res:/C") || r2.uri() == QUrl("res:/C"));
    QVERIFY(r1.uri() == QUrl("res:/D") || r2.uri() == QUrl("res:/D"));

    // res:/A has 3 properties
    if(r1.uri() == QUrl("res:/C")) {
        QCOMPARE(r1.properties().count(), 3);
        QCOMPARE(r2.properties().count(), 1);
    }
    else {
        QCOMPARE(r1.properties().count(), 1);
        QCOMPARE(r2.properties().count(), 3);
    }


    // get two resources without sub-res
    g = m_dmModel->describeResources(QList<QUrl>() << QUrl("res:/A") << QUrl("res:/C"), false).toList();

    // no error
    QVERIFY(!m_dmModel->lastError());

    // only one resource in the result
    QCOMPARE(g.count(), 2);

    // the results are res:/A and res:/C
    r1 = g.first();
    r2 = g.back();
    QVERIFY(r1.uri() == QUrl("res:/A") || r2.uri() == QUrl("res:/A"));
    QVERIFY(r1.uri() == QUrl("res:/C") || r2.uri() == QUrl("res:/C"));

    // res:/A has 3 properties
    QCOMPARE(r1.properties().count(), 3);
    QCOMPARE(r2.properties().count(), 3);


    // get two resources with sub-res
    g = m_dmModel->describeResources(QList<QUrl>() << QUrl("res:/A") << QUrl("res:/C"), true).toList();

    // no error
    QVERIFY(!m_dmModel->lastError());

    // only one resource in the result
    QCOMPARE(g.count(), 4);

    // the results are res:/A, res:/B, res:/C and res:/D
    QList<SimpleResource>::const_iterator it = g.constBegin();
    r1 = *it;
    ++it;
    r2 = *it;
    ++it;
    SimpleResource r3 = *it;
    ++it;
    SimpleResource r4 = *it;
    QVERIFY(r1.uri() == QUrl("res:/A") || r2.uri() == QUrl("res:/A") || r3.uri() == QUrl("res:/A") || r4.uri() == QUrl("res:/A"));
    QVERIFY(r1.uri() == QUrl("res:/B") || r2.uri() == QUrl("res:/B") || r3.uri() == QUrl("res:/B") || r4.uri() == QUrl("res:/B"));
    QVERIFY(r1.uri() == QUrl("res:/C") || r2.uri() == QUrl("res:/C") || r3.uri() == QUrl("res:/C") || r4.uri() == QUrl("res:/C"));
    QVERIFY(r1.uri() == QUrl("res:/D") || r2.uri() == QUrl("res:/D") || r3.uri() == QUrl("res:/D") || r4.uri() == QUrl("res:/D"));


    // get two resources with sub-res and mixed URL/URI
    g = m_dmModel->describeResources(QList<QUrl>() << QUrl("res:/A") << QUrl::fromLocalFile(fileC.fileName()), true).toList();

    // no error
    QVERIFY(!m_dmModel->lastError());

    // only one resource in the result
    QCOMPARE(g.count(), 4);

    // the results are res:/A, res:/B, res:/C and res:/D
    it = g.constBegin();
    r1 = *it;
    ++it;
    r2 = *it;
    ++it;
    r3 = *it;
    ++it;
    r4 = *it;
    QVERIFY(r1.uri() == QUrl("res:/A") || r2.uri() == QUrl("res:/A") || r3.uri() == QUrl("res:/A") || r4.uri() == QUrl("res:/A"));
    QVERIFY(r1.uri() == QUrl("res:/B") || r2.uri() == QUrl("res:/B") || r3.uri() == QUrl("res:/B") || r4.uri() == QUrl("res:/B"));
    QVERIFY(r1.uri() == QUrl("res:/C") || r2.uri() == QUrl("res:/C") || r3.uri() == QUrl("res:/C") || r4.uri() == QUrl("res:/C"));
    QVERIFY(r1.uri() == QUrl("res:/D") || r2.uri() == QUrl("res:/D") || r3.uri() == QUrl("res:/D") || r4.uri() == QUrl("res:/D"));
}

KTempDir * DataManagementModelTest::createNieUrlTestData()
{
    // now we create a real example with some real files:
    // mainDir
    // |- dir1
    //    |- dir11
    //       |- file111
    //    |- dir12
    //       |- dir121
    //          |- file1211
    //    |- file11
    //    |- dir13
    // |- dir2
    KTempDir* mainDir = new KTempDir();
    QDir dir(mainDir->name());
    dir.mkdir(QLatin1String("dir1"));
    dir.mkdir(QLatin1String("dir2"));
    dir.cd(QLatin1String("dir1"));
    dir.mkdir(QLatin1String("dir11"));
    dir.mkdir(QLatin1String("dir12"));
    dir.mkdir(QLatin1String("dir13"));
    QFile file(dir.filePath(QLatin1String("file11")));
    file.open(QIODevice::WriteOnly);
    file.close();
    dir.cd(QLatin1String("dir12"));
    dir.mkdir(QLatin1String("dir121"));
    dir.cd(QLatin1String("dir121"));
    file.setFileName(dir.filePath(QLatin1String("file1211")));
    file.open(QIODevice::WriteOnly);
    file.close();
    dir.cdUp();
    dir.cdUp();
    dir.cd(QLatin1String("dir11"));
    file.setFileName(dir.filePath(QLatin1String("file111")));
    file.open(QIODevice::WriteOnly);
    file.close();

    // We now create the situation in the model
    // for that we use 2 graphs
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase());
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase());
    const QString basePath = mainDir->name();

    // nie:url properties for all of them (spread over both graphs)
    m_model->addStatement(QUrl("res:/dir1"), NIE::url(), QUrl(QLatin1String("file://") + basePath + QLatin1String("dir1")), g1);
    m_model->addStatement(QUrl("res:/dir2"), NIE::url(), QUrl(QLatin1String("file://") + basePath + QLatin1String("dir2")), g2);
    m_model->addStatement(QUrl("res:/dir11"), NIE::url(), QUrl(QLatin1String("file://") + basePath + QLatin1String("dir1/dir11")), g1);
    m_model->addStatement(QUrl("res:/dir12"), NIE::url(), QUrl(QLatin1String("file://") + basePath + QLatin1String("dir1/dir12")), g2);
    m_model->addStatement(QUrl("res:/dir13"), NIE::url(), QUrl(QLatin1String("file://") + basePath + QLatin1String("dir1/dir13")), g1);
    m_model->addStatement(QUrl("res:/file11"), NIE::url(), QUrl(QLatin1String("file://") + basePath + QLatin1String("dir1/file11")), g2);
    m_model->addStatement(QUrl("res:/file111"), NIE::url(), QUrl(QLatin1String("file://") + basePath + QLatin1String("dir1/dir11/file111")), g1);
    m_model->addStatement(QUrl("res:/dir121"), NIE::url(), QUrl(QLatin1String("file://") + basePath + QLatin1String("dir2/dir121")), g2);
    m_model->addStatement(QUrl("res:/file1211"), NIE::url(), QUrl(QLatin1String("file://") + basePath + QLatin1String("dir2/dir121/file1211")), g1);

    // we define filename and parent folder only for some to test if the optional clause in the used query works properly
    m_model->addStatement(QUrl("res:/dir1"), NFO::fileName(), LiteralValue(QLatin1String("dir1")), g1);
    m_model->addStatement(QUrl("res:/dir2"), NFO::fileName(), LiteralValue(QLatin1String("dir2")), g1);
    m_model->addStatement(QUrl("res:/dir11"), NFO::fileName(), LiteralValue(QLatin1String("dir11")), g2);
    m_model->addStatement(QUrl("res:/dir12"), NFO::fileName(), LiteralValue(QLatin1String("dir12")), g2);
    m_model->addStatement(QUrl("res:/file11"), NFO::fileName(), LiteralValue(QLatin1String("file11")), g1);
    m_model->addStatement(QUrl("res:/file111"), NFO::fileName(), LiteralValue(QLatin1String("file111")), g2);
    m_model->addStatement(QUrl("res:/dir121"), NFO::fileName(), LiteralValue(QLatin1String("dir121")), g2);

    m_model->addStatement(QUrl("res:/dir11"), NIE::isPartOf(), QUrl("res:/dir1"), g1);
    m_model->addStatement(QUrl("res:/dir12"), NIE::isPartOf(), QUrl(QLatin1String("res:/dir1")), g2);
    m_model->addStatement(QUrl("res:/dir13"), NIE::isPartOf(), QUrl(QLatin1String("res:/dir1")), g1);
    m_model->addStatement(QUrl("res:/file111"), NIE::isPartOf(), QUrl(QLatin1String("res:/dir11")), g1);
    m_model->addStatement(QUrl("res:/dir121"), NIE::isPartOf(), QUrl(QLatin1String("res:/dir2")), g2);
    m_model->addStatement(QUrl("res:/file1211"), NIE::isPartOf(), QUrl(QLatin1String("res:/dir121")), g1);

    return mainDir;
}

bool DataManagementModelTest::haveTrailingGraphs() const
{
    return m_model->executeQuery(QString::fromLatin1("ask where { "
                                                     "?g a ?t . "
                                                     "FILTER(!bif:exists( (select (1) where { graph ?g { ?s ?p ?o . } . }))) . "
                                                     "FILTER(?t in (%1,%2,%3)) . "
                                                     "}")
                                 .arg(Node::resourceToN3(NRL::InstanceBase()),
                                      Node::resourceToN3(NRL::DiscardableInstanceBase()),
                                      Node::resourceToN3(NRL::GraphMetadata())),
                                 Soprano::Query::QueryLanguageSparql).boolValue();
}

void DataManagementModelTest::testImportResources()
{
    // create the test data
    QTemporaryFile fileA;
    fileA.open();

    Soprano::Graph graph;
    graph.addStatement(Node(QString::fromLatin1("res1")), QUrl("prop:/int"), LiteralValue(42));
    graph.addStatement(Node(QString::fromLatin1("res1")), RDF::type(), QUrl("class:/typeA"));
    graph.addStatement(Node(QString::fromLatin1("res1")), QUrl("prop:/res"), Node(QString::fromLatin1("res2")));
    graph.addStatement(Node(QString::fromLatin1("res2")), RDF::type(), QUrl("class:/typeB"));
    graph.addStatement(QUrl::fromLocalFile(fileA.fileName()), QUrl("prop:/int"), LiteralValue(12));
    graph.addStatement(QUrl::fromLocalFile(fileA.fileName()), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")));

    // write the test file
    QTemporaryFile tmp;
    tmp.open();
    QTextStream str(&tmp);
    Q_FOREACH(const Statement& s, graph.toList()) {
        str << s.subject().toN3() << " " << s.predicate().toN3() << " " << s.object().toN3() << " ." << endl;
    }
    tmp.close();


    // import the file
    m_dmModel->importResources(QUrl::fromLocalFile(tmp.fileName()), QLatin1String("A"), Soprano::SerializationNTriples);


    // make sure the data has been imported properly
    QVERIFY(m_model->containsAnyStatement(Node(), QUrl("prop:/int"), LiteralValue(42)));
    const QUrl res1Uri = m_model->listStatements(Node(), QUrl("prop:/int"), LiteralValue(42)).allStatements().first().subject().uri();
    QVERIFY(m_model->containsAnyStatement(res1Uri, RDF::type(), QUrl("class:/typeA")));
    QVERIFY(m_model->containsAnyStatement(res1Uri, QUrl("prop:/res"), Node()));
    const QUrl res2Uri = m_model->listStatements(res1Uri, QUrl("prop:/res"), Node()).allStatements().first().object().uri();
    QVERIFY(m_model->containsAnyStatement(res2Uri, RDF::type(), QUrl("class:/typeB")));
    QVERIFY(m_model->containsAnyStatement(Node(), NIE::url(), QUrl::fromLocalFile(fileA.fileName())));
    const QUrl res3Uri = m_model->listStatements(Node(), NIE::url(), QUrl::fromLocalFile(fileA.fileName())).allStatements().first().subject().uri();
    QVERIFY(m_model->containsAnyStatement(res3Uri, QUrl("prop:/int"), LiteralValue(12)));
    QVERIFY(m_model->containsAnyStatement(res3Uri, QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))));

    // make sure the metadata is there
    QVERIFY(m_model->containsAnyStatement(res1Uri, NAO::lastModified(), Node()));
    QVERIFY(m_model->containsAnyStatement(res1Uri, NAO::created(), Node()));
    QVERIFY(m_model->containsAnyStatement(res2Uri, NAO::lastModified(), Node()));
    QVERIFY(m_model->containsAnyStatement(res2Uri, NAO::created(), Node()));
    QVERIFY(m_model->containsAnyStatement(res3Uri, NAO::lastModified(), Node()));
    QVERIFY(m_model->containsAnyStatement(res3Uri, NAO::created(), Node()));
}

QTEST_KDEMAIN_CORE(DataManagementModelTest)

#include "datamanagementmodeltest.moc"
