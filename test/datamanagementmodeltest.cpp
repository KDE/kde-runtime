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

    m_model->addStatement( QUrl("prop:/res_c1"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/res_c1"), RDFS::range(), RDFS::Resource(), graph );
    m_model->addStatement( QUrl("prop:/res_c1"), NRL::maxCardinality(), LiteralValue(1), graph );

    m_model->addStatement( QUrl("class:/typeA"), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( QUrl("class:/typeB"), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( QUrl("class:/typeB"), RDFS::subClassOf(), QUrl("class:/typeA"), graph );


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

    // some ontology things we need in testStoreResources_realLife
    m_model->addStatement( NMM::season(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::season(), RDFS::range(), XMLSchema::xsdInt(), graph );
    m_model->addStatement( NMM::episodeNumber(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::episodeNumber(), RDFS::range(), XMLSchema::xsdInt(), graph );
    m_model->addStatement( NIE::description(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NIE::description(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NIE::title(), RDFS::subClassOf(), QUrl("http://www.semanticdesktop.org/ontologies/2007/08/15/nao#identifyingProperty"), graph );

    // rebuild the internals of the data management model
    m_dmModel->updateTypeCachesAndSoOn();
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

    m_dmModel = new Nepomuk::DataManagementModel( m_nrlModel );
}

void DataManagementModelTest::cleanupTestCase()
{
    delete m_dmModel;
    delete m_nrlModel;
    delete m_model;
    delete m_storageDir;
}

void DataManagementModelTest::init()
{
    resetModel();
}


void DataManagementModelTest::testAddProperty()
{
    // we start by simply adding a property
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/string"), QVariantList() << QVariant(QLatin1String("foobar")), QLatin1String("Testapp"));

    QVERIFY(!m_dmModel->lastError());

    // check that the actual data is there
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))));

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
                                                      "graph ?g { <res:/A> <prop:/string> %1 . } . "
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
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/string"), QVariantList() << QVariant(QLatin1String("hello world")), QLatin1String("Testapp"));

    // verify the values
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), QUrl("prop:/string"), Node()).allStatements().count(), 2);
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world"))));

    // check that we only have one agent instance
    QCOMPARE(m_model->listStatements(Node(), RDF::type(), NAO::Agent()).allStatements().count(), 1);

    //
    // rewrite the same property with the same app
    //
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/string"), QVariantList() << QVariant(QLatin1String("hello world")), QLatin1String("Testapp"));

    // nothing should have changed
    QCOMPARE(existingStatements, Soprano::Graph(m_model->listStatements().allStatements()));


    //
    // rewrite the same property with another app
    //
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/string"), QVariantList() << QVariant(QLatin1String("hello world")), QLatin1String("Otherapp"));

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
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/int"), QVariantList() << 42, QLatin1String("Testapp"));

    // now the newly created resource should have all the metadata a resource needs to have
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NAO::created(), Node()));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NAO::lastModified(), Node()));

    // and both created and last modification date should be similar
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), NAO::created(), Node()).iterateObjects().allNodes().first(),
             m_model->listStatements(QUrl("res:/A"), NAO::lastModified(), Node()).iterateObjects().allNodes().first());

    QVERIFY(!haveTrailingGraphs());
}


void DataManagementModelTest::testAddProperty_cardinality()
{
    // adding the same value twice in one call should result in one insert. This also includes the cardinality check
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/AA"), QUrl("prop:/res_c1"), QVariantList() << QVariant(QUrl("res:/B")) << QVariant(QUrl("res:/B")), QLatin1String("Testapp"));
    QVERIFY(!m_dmModel->lastError());
    QCOMPARE(m_model->listStatements(QUrl("res:/AA"), QUrl("prop:/res_c1"), QUrl("res:/B")).allStatements().count(), 1);

    // we now add two values for a property with cardinality 1
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/res_c1"), QVariantList() << QVariant(QUrl("res:/B")) << QVariant(QUrl("res:/C")), QLatin1String("Testapp"));
    QVERIFY(m_dmModel->lastError());

    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/res_c1"), QVariantList() << QVariant(QUrl("res:/B")), QLatin1String("Testapp"));
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/res_c1"), QVariantList() << QVariant(QUrl("res:/C")), QLatin1String("Testapp"));

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
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/res"), QVariantList() << QVariant(QUrl(fileA.fileName())) << QVariant(fileAUri), QLatin1String("Testapp"));

    QCOMPARE(m_model->listStatements(QUrl("res:/A"), QUrl("prop:/res"), fileAUri).allStatements().count(), 1);
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl::fromLocalFile(fileA.fileName())));
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

void DataManagementModelTest::testSetProperty()
{
    // adding the most basic property
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/string"), QVariantList() << QVariant(QLatin1String("foobar")), QLatin1String("Testapp"));

    QVERIFY(!m_dmModel->lastError());

    // check that the actual data is there
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))));

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
                                                      "graph ?g { <res:/A> <prop:/string> %1 . } . "
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
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/int"), QVariantList() << 42, QLatin1String("Testapp"));

    // now the newly created resource should have all the metadata a resource needs to have
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NAO::created(), Node()));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NAO::lastModified(), Node()));

    // and both created and last modification date should be similar
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), NAO::created(), Node()).iterateObjects().allNodes().first(),
             m_model->listStatements(QUrl("res:/A"), NAO::lastModified(), Node()).iterateObjects().allNodes().first());

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
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/A"), QUrl(), QVariantList() << 42, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty value list
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/int"), QVariantList(), QLatin1String("testapp"));

    // the call should NOT have failed
    QVERIFY(!m_dmModel->lastError());

    // but nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty app
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/int"), QVariantList() << 42, QString());

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // invalid range
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/int"), QVariantList() << QLatin1String("foobar"), QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // protected property 1
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/A"), NAO::created(), QVariantList() << QDateTime::currentDateTime(), QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // protected property 1
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/A"), NAO::lastModified(), QVariantList() << QDateTime::currentDateTime(), QLatin1String("testapp"));

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
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/res"), QVariantList() << nonExistingFileUrl, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

void DataManagementModelTest::testSetProperty_nieUrl1()
{
    // setting nie:url if it is not there yet should result in a normal setProperty including graph creation
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/A"), NIE::url(), QVariantList() << QUrl("file:///tmp/A"), QLatin1String("testapp"));

    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NIE::url(), QUrl("file:///tmp/A")));

    // remember the graph since it should not change later on
    const QUrl nieUrlGraph = m_model->listStatements(QUrl("res:/A"), NIE::url(), QUrl("file:///tmp/A")).allStatements().first().context().uri();

    QVERIFY(!haveTrailingGraphs());


    // we reset the URL
    m_dmModel->setProperty(QList<QUrl>() << QUrl("res:/A"), NIE::url(), QVariantList() << QUrl("file:///tmp/B"), QLatin1String("testapp"));

    // the url should have changed
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), NIE::url(), QUrl("file:///tmp/A")));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NIE::url(), QUrl("file:///tmp/B")));

    // the graph should have been kept
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), NIE::url(), Node()).allStatements().first().context().uri(), nieUrlGraph);

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testSetProperty_nieUrl2()
{
    KTempDir* dir = createNieUrlTestData();

    // change the nie:url of one of the top level dirs
    const QUrl newDir1Url = QLatin1String("file://") + dir->name() + QLatin1String("dir1-new");

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
    const QUrl oldDir1Url = QLatin1String("file://") + dir->name() + QLatin1String("dir1");
    const QUrl newDir1Url = QLatin1String("file://") + dir->name() + QLatin1String("dir1-new");

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
    const QUrl oldDir121Url = QLatin1String("file://") + dir->name() + QLatin1String("dir1/dir12/dir121");
    const QUrl newDir121Url = QLatin1String("file://") + dir->name() + QLatin1String("dir1/dir12/dir121-new");

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
    const QUrl oldDir121Url = QLatin1String("file://") + dir->name() + QLatin1String("dir1/dir12/dir121");
    const QUrl newDir121Url = QLatin1String("file://") + dir->name() + QLatin1String("dir2/dir121");

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


    m_dmModel->removeResources(QList<QUrl>() << QUrl("res:/A"), DataManagementModel::NoRemovalFlags, QLatin1String("testapp"));

    // verify that the resource is gone
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));

    // verify that other resources were not touched
    QCOMPARE(m_model->listStatements(QUrl("res:/B"), Node(), Node()).allStatements().count(), 4);
    QCOMPARE(m_model->listStatements(QUrl("res:/C"), Node(), Node()).allStatements().count(), 2);

    // verify that removing resources by file URL works
    m_dmModel->removeResources(QList<QUrl>() << QUrl::fromLocalFile(fileB.fileName()), DataManagementModel::NoRemovalFlags, QLatin1String("testapp"));

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
    m_dmModel->removeResources(QList<QUrl>() << QUrl("res:/A"), DataManagementModel::RemoveSubResoures, QLatin1String("A"));

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
    m_dmModel->removeResources(QList<QUrl>(), DataManagementModel::NoRemovalFlags, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // resource list with empty URL
    m_dmModel->removeResources(QList<QUrl>() << QUrl("res:/A") << QUrl(), DataManagementModel::NoRemovalFlags, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // empty app
    m_dmModel->removeResources(QList<QUrl>() << QUrl("res:/A"), DataManagementModel::NoRemovalFlags, QString());

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // non-existing file
    const QUrl nonExistingFileUrl("file:///a/file/that/is/very/unlikely/to/exist");
    m_dmModel->removeResources(QList<QUrl>() << nonExistingFileUrl, DataManagementModel::NoRemovalFlags, QLatin1String("testapp"));

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
    m_dmModel->removeResources(QList<QUrl>() << QUrl("prop:/res"), DataManagementModel::NoRemovalFlags, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // class
    m_dmModel->removeResources(QList<QUrl>() << NRL::Graph(), DataManagementModel::NoRemovalFlags, QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // graph
    m_dmModel->removeResources(QList<QUrl>() << QUrl("graph:/onto"), DataManagementModel::NoRemovalFlags, QLatin1String("testapp"));

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

    // delete the resource
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), DataManagementModel::NoRemovalFlags, QLatin1String("A"));

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
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), DataManagementModel::NoRemovalFlags, QLatin1String("A"));

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
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), DataManagementModel::NoRemovalFlags, QLatin1String("A"));

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
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl::fromLocalFile(fileA.fileName()), DataManagementModel::NoRemovalFlags, QLatin1String("A"));

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
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), DataManagementModel::RemoveSubResoures, QLatin1String("A"));

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
    m_dmModel->removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), DataManagementModel::RemoveSubResoures, QLatin1String("A"));

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
void DataManagementModelTest::testStoreResources()
{
    Nepomuk::ResourceManager::instance()->setOverrideMainModel( m_model );
    
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
    res.addProperty( RDF::type(), NFO::FileDataObject() );
    res.addProperty( QUrl("nepomuk:/mergeTest/prop1"), QVariant(10) );

    Nepomuk::SimpleResourceGraph graph;
    graph << res;

    // Try merging it.
    m_dmModel->storeResources( graph, QLatin1String("Testapp") );
    
    // res2 shouldn't exists as it is the same as res1 and should have gotten merged.
    QVERIFY( !m_model->containsAnyStatement( QUrl("nepomuk:/mergeTest/res2"), QUrl(), QUrl() ) );

    // Try merging it as a blank uri
    res.setUri( QUrl("_:blah") );
    graph.clear();
    graph << res;

    m_dmModel->storeResources( graph, QLatin1String("Testapp") );
    QVERIFY( !m_model->containsAnyStatement( QUrl("_:blah"), QUrl(), QUrl() ) );

    QVERIFY(!haveTrailingGraphs());

    //
    // Test 2 -
    // This is for testing exactly how Strigi will use storeResources ie
    // have some blank nodes ( some of which may already exists ) and a
    // main resources which does not exist
    //
    {
        kDebug() << "Starting Strigi merge test";

        QUrl graphUri("nepomuk:/ctx/afd"); // TODO: Actually Create one

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

    //
    // Test 3 -
    // Test the graph rules. If a resource exists in a nrl:DiscardableInstanceBase
    // and it is merged with a non-discardable graph. Then the graph should be replaced
    // In the opposite case - nothing should be done
    {
        Nepomuk::SimpleResource res;
        res.addProperty( RDF::type(), NCO::Contact() );
        res.addProperty( NCO::fullname(), "Lion" );

        QUrl graphUri("nepomuk:/ctx/mergeRes/testGraph");
        QUrl graphMetadataGraph("nepomuk:/ctx/mergeRes/testGraph-metadata");
        int stCount = m_model->statementCount();
        m_model->addStatement( graphUri, RDF::type(), NRL::InstanceBase(), graphMetadataGraph );
        m_model->addStatement( graphUri, RDF::type(), NRL::DiscardableInstanceBase(), graphMetadataGraph );
        m_model->addStatement( graphMetadataGraph, NRL::coreGraphMetadataFor(), graphUri, graphMetadataGraph );
        QVERIFY( m_model->statementCount() == stCount+3 );

        res.setUri(QUrl("nepomuk:/res/Lion"));
        QVERIFY( push( m_model, res, graphUri ) == res.properties().size() );
        res.setUri(QUrl("_:lion"));

        Nepomuk::SimpleResourceGraph resGraph;
        resGraph << res;

        QHash<QUrl, QVariant> additionalMetadata;
        additionalMetadata.insert( RDF::type(), NRL::InstanceBase() );
        m_dmModel->storeResources( resGraph, "TestApp", additionalMetadata );
        
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

        QUrl graphUri("nepomuk:/ctx/mergeRes/testGraph2");
        QUrl graphMetadataGraph("nepomuk:/ctx/mergeRes/testGraph2-metadata");
        int stCount = m_model->statementCount();
        m_model->addStatement( graphUri, RDF::type(), NRL::InstanceBase(), graphMetadataGraph );
        m_model->addStatement( graphMetadataGraph, NRL::coreGraphMetadataFor(), graphUri, graphMetadataGraph );
        QVERIFY( m_model->statementCount() == stCount+2 );

        res.setUri(QUrl("nepomuk:/res/Tiger"));
        QVERIFY( push( m_model, res, graphUri ) == res.properties().size() );
        res.setUri(QUrl("_:tiger"));

        Nepomuk::SimpleResourceGraph resGraph;
        resGraph << res;

        QHash<QUrl, QVariant> additionalMetadata;
        additionalMetadata.insert( RDF::type(), NRL::InstanceBase() );
        m_dmModel->storeResources( resGraph, "TestApp", additionalMetadata );

        QList<Soprano::Statement> stList = m_model->listStatements( Soprano::Node(), NCO::fullname(),
                                                           Soprano::LiteralValue("Tiger") ).allStatements();
        kDebug() << stList;
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
#warning We need to get rid of the ResourceManager usage here!
    ResourceManager::instance()->setOverrideMainModel( m_model );

    //
    // Simple case: create a resource by merging it
    //
    SimpleResource res;
    res.setUri(QUrl("_:A"));
    res.addProperty(RDF::type(), NAO::Tag());
    res.addProperty(NAO::prefLabel(), QLatin1String("Foobar"));

    m_dmModel->storeResources(SimpleResourceGraph() << res, QLatin1String("testapp"));

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

    // nothing should have happened
    QCOMPARE(existingStatements, Soprano::Graph(m_model->listStatements().allStatements()));

    //
    // Now create the same resource with a different app
    //
    m_dmModel->storeResources(SimpleResourceGraph() << res, QLatin1String("testapp2"));

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

    QVERIFY(m_model->containsAnyStatement( res2.uri(), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))));

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testStoreResources_invalid_args()
{
    ResourceManager::instance()->setOverrideMainModel( m_model );

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
    res.setUri(QUrl("res:/A"));
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
    nonExistingFileRes.setUri(QUrl("res:/A"));
    nonExistingFileRes.addProperty(QUrl("prop:/res"), nonExistingFileUrl);
    m_dmModel->storeResources(SimpleResourceGraph() << nonExistingFileRes, QLatin1String("testapp"));

    // the call should have failed
    QVERIFY(m_dmModel->lastError());

    // nothing should have changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);
}

void DataManagementModelTest::testStoreResources_invalid_args_with_existing()
{
    ResourceManager::instance()->setOverrideMainModel( m_model );

    // create a test resource
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    // create the resource to delete
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/B"), g1);
    const QDateTime now = QDateTime::currentDateTime();
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(now), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::lastModified(), LiteralValue(now), g1);


    // create a resource to merge
    SimpleResource a(QUrl("res:/A"));
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

    ResourceManager::instance()->setOverrideMainModel( m_model );

    // merge a file URL
    SimpleResource r1;
    r1.setUri(QUrl::fromLocalFile(fileA.fileName()));
    r1.addProperty(RDF::type(), NAO::Tag());
    r1.addProperty(QUrl("prop:/string"), QLatin1String("Foobar"));

    m_dmModel->storeResources(SimpleResourceGraph() << r1, QLatin1String("testapp"));

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
    ResourceManager::instance()->setOverrideMainModel( m_model );

    // merge a property with non-existing file value
    QTemporaryFile fileA;
    fileA.open();
    const QUrl fileUrl = QUrl::fromLocalFile(fileA.fileName());

    SimpleResource r1;
    r1.setUri(QUrl("nepomuk:/res/A"));
    r1.addProperty(QUrl("prop:/res"), fileUrl);

    m_dmModel->storeResources(SimpleResourceGraph() << r1, QLatin1String("testapp"));

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

// metadata should be ignored when merging one resource into another
void DataManagementModelTest::testStoreResources_metadata()
{
    ResourceManager::instance()->setOverrideMainModel( m_model );

    // create our app
    const QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);

    // create a resource
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    const QDateTime now = QDateTime::currentDateTime();
    m_model->addStatement(QUrl("res:/A"), RDF::type(), NAO::Tag(), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("Foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(now), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::lastModified(), LiteralValue(now), g1);


    // now we merge the same resource (with differing metadata)
    SimpleResource a;
    a.addProperty(QUrl("prop:/int"), QVariant(42));
    a.addProperty(QUrl("prop:/string"), QVariant(QLatin1String("Foobar")));
    a.addProperty(NAO::created(), QVariant(QDateTime(QDate(2010, 12, 24), QTime::currentTime())));

    // merge the resource
    m_dmModel->storeResources(SimpleResourceGraph() << a, QLatin1String("B"));
    QVERIFY(!m_dmModel->lastError());

    // make sure no new resource has been created
    QCOMPARE(m_model->listStatements(Node(), RDF::type(), NAO::Tag()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/int"), Node()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/string"), Node()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), NAO::created(), Node()).allStatements().count(), 1);

    // make sure the new app has been created
    QueryResultIterator it = m_model->executeQuery(QString::fromLatin1("select ?a where { ?a a %1 . ?a %2 %3 . }")
                                                   .arg(Node::resourceToN3(NAO::Agent()),
                                                        Node::resourceToN3(NAO::identifier()),
                                                        Node::literalToN3(QLatin1String("B"))),
                                                   Soprano::Query::QueryLanguageSparql);
    QVERIFY(it.next());
    const QUrl appBRes = it[0].uri();

    // make sure the data is now maintained by both apps
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { graph ?g { <res:/A> <prop:/int> %1 . } . ?g %2 %3 . ?g %2 <app:/A> . }")
                                  .arg(Node::literalToN3(42),
                                       Node::resourceToN3(NAO::maintainedBy()),
                                       Node::resourceToN3(appBRes)),
                                  Soprano::Query::QueryLanguageSparql).boolValue());
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { graph ?g { <res:/A> <prop:/string> %1 . } . ?g %2 %3 . ?g %2 <app:/A> . }")
                                  .arg(Node::literalToN3(QLatin1String("Foobar")),
                                       Node::resourceToN3(NAO::maintainedBy()),
                                       Node::resourceToN3(appBRes)),
                                  Soprano::Query::QueryLanguageSparql).boolValue());
    QVERIFY(!m_model->executeQuery(QString::fromLatin1("ask where { graph ?g { <res:/A> a %1 . } . ?g %2 %3 . ?g %2 <app:/A> . }")
                                  .arg(Node::resourceToN3(NAO::Tag()),
                                       Node::resourceToN3(NAO::maintainedBy()),
                                       Node::resourceToN3(appBRes)),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    QVERIFY(!haveTrailingGraphs());


    // now merge the same resource with some new data - just to make sure the metadata is updated properly
    SimpleResource resA(QUrl("res:/A"));
    resA.addProperty(QUrl("prop:/int2"), 42);

    // merge the resource
    m_dmModel->storeResources(SimpleResourceGraph() << resA, QLatin1String("B"));
    QVERIFY(!m_dmModel->lastError());

    // make sure the new data is there
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(42)));

    // make sure creation date did not change
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), NAO::created(), Node()).allStatements().count(), 1);
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NAO::created(), LiteralValue(now)));

    // make sure mtime has changed - the resource has changed
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), NAO::lastModified(), Node()).allStatements().count(), 1);
    QVERIFY(m_model->listStatements(QUrl("res:/A"), NAO::lastModified(), Node()).iterateObjects().allNodes().first().literal().toDateTime() != now);
}

void DataManagementModelTest::testStoreResources_protectedTypes()
{
    ResourceManager::instance()->setOverrideMainModel( m_model );

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
    ResourceManager::instance()->setOverrideMainModel( m_model );

    // 1. create a resource to merge
    QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/A"), RDF::type(), QUrl("class:/typeA"), g1);
    m_model->addStatement(QUrl("res:/A"), RDF::type(), QUrl("class:/typeB"), g1);
    const QDateTime now = QDateTime::currentDateTime();
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(now), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::lastModified(), LiteralValue(now), g1);


    // now merge the same resource (excluding the super-type A)
    SimpleResource a;
    a.addProperty(RDF::type(), QUrl("class:/typeB"));
    a.addProperty(QUrl("prop:/string"), QLatin1String("hello world"));

    m_dmModel->storeResources(SimpleResourceGraph() << a, QLatin1String("A"));


    // make sure the existing resource was reused
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world"))).allElements().count(), 1);
}

// make sure merging even works with missing metadata in store
void DataManagementModelTest::testStoreResources_missingMetadata()
{
    ResourceManager::instance()->setOverrideMainModel( m_model );

    // create our app
    const QUrl appG = m_nrlModel->createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(QLatin1String("A")), appG);

    // create a resource (without creation date)
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    const QDateTime now = QDateTime::currentDateTime();
    m_model->addStatement(QUrl("res:/A"), RDF::type(), NAO::Tag(), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("Foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::lastModified(), LiteralValue(now), g1);


    // now we merge the same resource
    SimpleResource a;
    a.addProperty(QUrl("prop:/int"), QVariant(42));
    a.addProperty(QUrl("prop:/string"), QVariant(QLatin1String("Foobar")));

    // merge the resource
    m_dmModel->storeResources(SimpleResourceGraph() << a, QLatin1String("B"));

    // make sure no new resource has been created
    QCOMPARE(m_model->listStatements(Node(), RDF::type(), NAO::Tag()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/int"), Node()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/string"), Node()).allStatements().count(), 1);

    QVERIFY(!haveTrailingGraphs());


    // now merge the same resource with some new data - just to make sure the metadata is updated properly
    SimpleResource resA(QUrl("res:/A"));
    resA.addProperty(QUrl("prop:/int2"), 42);

    // merge the resource
    m_dmModel->storeResources(SimpleResourceGraph() << resA, QLatin1String("B"));

    // make sure the new data is there
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(42)));

    // make sure creation date did not change, ie. it was not created as that would be wrong
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), NAO::created(), Node()));

    // make sure the last mtime has been updated
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), NAO::lastModified(), Node()).allStatements().count(), 1);
    QVERIFY(m_model->listStatements(QUrl("res:/A"), NAO::lastModified(), Node()).iterateObjects().allNodes().first().literal().toDateTime() > now);
}

// test merging when there is more than one candidate resource to merge with
void DataManagementModelTest::testStoreResources_multiMerge()
{
    ResourceManager::instance()->setOverrideMainModel( m_model );

    // create two resource which could be matches for the one we will store
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase());

    // the resource in which we want to merge
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::lastModified(), LiteralValue(QDateTime::currentDateTime()), g1);

    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/B"), NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);
    m_model->addStatement(QUrl("res:/B"), NAO::lastModified(), LiteralValue(QDateTime::currentDateTime()), g1);


    // now store the exact same resource
    SimpleResource res;
    res.addProperty(QUrl("prop:/int"), 42);
    res.addProperty(QUrl("prop:/string"), QLatin1String("foobar"));

    m_dmModel->storeResources(SimpleResourceGraph() << res, QLatin1String("A"));
    QVERIFY(!m_dmModel->lastError());

    // make sure no new resource was created
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/int"), LiteralValue(42)).allElements().count(), 2);
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))).allElements().count(), 2);

    // make sure both resources still exist
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/B"), Node(), Node()));
}

// an example from real-life which made an early version of DMS fail
void DataManagementModelTest::testStoreResources_realLife()
{
    ResourceManager::instance()->setOverrideMainModel( m_model );

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

    SimpleResource tvSeriesRes(graph.createBlankNode());
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
    ResourceManager::instance()->setOverrideMainModel( m_model );

    // we create a resource with some properties
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase());

    m_model->addStatement(QUrl("res:/A"), RDF::type(), QUrl("class:/typeA"), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::lastModified(), LiteralValue(QDateTime::currentDateTime()), g1);


    // now we store a trivial resource
    SimpleResource res;
    res.addProperty(RDF::type(), QUrl("class:/typeA"));

    m_dmModel->storeResources(SimpleResourceGraph() << res, QLatin1String("A"));

    // the two resources should NOT have been merged
    QCOMPARE(m_model->listStatements(Node(), RDF::type(), QUrl("class:/typeA")).allElements().count(), 2);
}

void DataManagementModelTest::testMergeResources()
{
    // first we need to create the two resources we want to merge as well as one that should not be touched
    // for this simple test we put everything into one graph
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);

    // the resource in which we want to merge
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int_c1"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);

    // the resource that is going to be merged
    // one duplicate property and one that differs, one backlink to ignore,
    // one property with cardinality 1 to ignore
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/int_c1"), LiteralValue(12), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/B"), g1);

    // resource C to ignore (except the backlink which needs to be updated)
    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/res"), QUrl("res:/B"), g1);


    // now merge the resources
    m_dmModel->mergeResources(QUrl("res:/A"), QUrl("res:/B"), QLatin1String("A"));

    // make sure B is gone
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/B"), Node(), Node()));
    QVERIFY(!m_model->containsAnyStatement(Node(), Node(), QUrl("res:/B")));

    // make sure A has all the required properties
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42)));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int_c1"), LiteralValue(42)));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello"))));

    // make sure A has no superfluous properties
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int_c1"), LiteralValue(12)));
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), QUrl("prop:/int"), Node()).allElements().count(), 1);

    // make sure the backlink was updated
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/C"), QUrl("prop:/res"), QUrl("res:/A")));

    // make sure C was not touched apart from the backlink
    QVERIFY(m_model->containsStatement(QUrl("res:/C"), QUrl("prop:/int"), LiteralValue(42), g1));
    QVERIFY(m_model->containsStatement(QUrl("res:/C"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1));

    QVERIFY(!haveTrailingGraphs());
}

void DataManagementModelTest::testMergeResources_protectedTypes()
{
    // create one resource to be merged with something else
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);

    m_model->addStatement(QUrl("res:/A"), RDF::type(), NAO::Tag(), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("Foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::created(), LiteralValue(QDateTime::currentDateTime()), g1);


    // remember current state to compare later on
    Soprano::Graph existingStatements = m_model->listStatements().allStatements();


    // property 1
    m_dmModel->mergeResources(QUrl("res:/A"), QUrl("prop:/int"), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // property 2
    m_dmModel->mergeResources(QUrl("prop:/int"), QUrl("res:/A"), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // class 1
    m_dmModel->mergeResources(QUrl("res:/A"), NRL::Graph(), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // property 2
    m_dmModel->mergeResources(NRL::Graph(), QUrl("res:/A"), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // graph 1
    m_dmModel->mergeResources(QUrl("res:/A"), QUrl("graph:/onto"), QLatin1String("testapp"));

    // this call should fail
    QVERIFY(m_dmModel->lastError());

    // no data should have been changed
    QCOMPARE(Graph(m_model->listStatements().allStatements()), existingStatements);


    // graph 2
    m_dmModel->mergeResources(QUrl("graph:/onto"), QUrl("res:/A"), QLatin1String("testapp"));

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
