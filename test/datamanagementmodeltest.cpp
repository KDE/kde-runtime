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
using namespace Soprano::Vocabulary;
using namespace Nepomuk;
using namespace Nepomuk::Vocabulary;


// TODO: test error conditions: provide invalid parameters in all variations and then check for lastError() and ensure that nothing was changed in the model.

void DataManagementModelTest::resetModel()
{
    // remove all the junk from previous tests
    m_model->removeAllStatements();

    // add some classes and properties
    QUrl graph("graph:/onto");
    m_model->addStatement( graph, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::Ontology(), graph );

    m_model->addStatement( QUrl("prop:/int"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/int"), Soprano::Vocabulary::RDFS::range(), Soprano::Vocabulary::XMLSchema::xsdInt(), graph );

    m_model->addStatement( QUrl("prop:/int2"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/int2"), Soprano::Vocabulary::RDFS::range(), Soprano::Vocabulary::XMLSchema::xsdInt(), graph );

    m_model->addStatement( QUrl("prop:/int3"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/int3"), Soprano::Vocabulary::RDFS::range(), Soprano::Vocabulary::XMLSchema::xsdInt(), graph );

    m_model->addStatement( QUrl("prop:/string"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/string"), Soprano::Vocabulary::RDFS::range(), Soprano::Vocabulary::XMLSchema::string(), graph );

    m_model->addStatement( QUrl("prop:/res"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/res"), Soprano::Vocabulary::RDFS::range(), Soprano::Vocabulary::RDFS::Resource(), graph );

    m_model->addStatement( QUrl("prop:/res_c1"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/res_c1"), Soprano::Vocabulary::RDFS::range(), Soprano::Vocabulary::RDFS::Resource(), graph );
    m_model->addStatement( QUrl("prop:/res_c1"), Soprano::Vocabulary::NRL::maxCardinality(), LiteralValue(1), graph );


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


// TODO: 1. test file URLs both as resource and as property value
//       2. test file URLs encoded as strings in property values

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
                                  .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::Agent()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::identifier()),
                                       Soprano::Node::literalToN3(QLatin1String("Testapp")),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::InstanceBase()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::GraphMetadata()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::coreGraphMetadataFor())),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    // check that we have an InstanceBase with a GraphMetadata graph
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { <res:/A> <prop:/string> %1 . } . "
                                                      "graph ?mg { ?g a %2 . ?mg a %3 . ?mg %4 ?g . } . "
                                                      "}")
                                  .arg(Soprano::Node::literalToN3(QLatin1String("foobar")),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::InstanceBase()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::GraphMetadata()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::coreGraphMetadataFor())),
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
                                      .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::Agent()),
                                           Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::identifier()),
                                           Soprano::Node::literalToN3(QLatin1String("Otherapp"))),
                                      Soprano::Query::QueryLanguageSparql)
                .iterateStatementsFromBindings(QLatin1String("s"), QLatin1String("p"), QLatin1String("o"), QLatin1String("g"))
                .allStatements()
                + m_model->executeQuery(QString::fromLatin1("select ?g ?s ?p ?o where { graph ?g { ?s ?p ?o . } . filter(bif:exists((select (1) where { graph ?gg { ?a a %1 . ?a %2 %3 . } . ?g %4 ?gg . })))}")
                                        .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::Agent()),
                                             Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::identifier()),
                                             Soprano::Node::literalToN3(QLatin1String("Otherapp")),
                                             Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::coreGraphMetadataFor())),
                                        Soprano::Query::QueryLanguageSparql)
                .iterateStatementsFromBindings(QLatin1String("s"), QLatin1String("p"), QLatin1String("o"), QLatin1String("g"))
                .allStatements()
                + m_model->listStatements(Node(), NAO::maintainedBy(), Node(), Node()).allStatements()
                );

    QCOMPARE(existingStatements, Soprano::Graph(m_model->listStatements().allStatements()));
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
}


void DataManagementModelTest::testAddProperty_file()
{
    m_dmModel->addProperty(QList<QUrl>() << QUrl("file:/A"), QUrl("prop:/string"), QVariantList() << QVariant(QLatin1String("foobar")), QLatin1String("Testapp"));

    // make sure the nie:url relation has been created
    QVERIFY(m_model->containsAnyStatement(Node(), NIE::url(), QUrl("file:/A")));
    QVERIFY(!m_model->containsAnyStatement(QUrl("file:/A"), Node(), Node()));

    // make sure the actual value is there
    QVERIFY(m_model->containsAnyStatement(m_model->listStatements(Node(), NIE::url(), QUrl("file:/A")).allStatements().first().subject(), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar"))));


    // add relation from file to file
    m_dmModel->addProperty(QList<QUrl>() << QUrl("file:/A"), QUrl("prop:/res"), QVariantList() << QVariant(QUrl("file:/B")), QLatin1String("Testapp"));

    // make sure the nie:url relation has been created
    QVERIFY(m_model->containsAnyStatement(Node(), NIE::url(), QUrl("file:/B")));
    QVERIFY(!m_model->containsAnyStatement(QUrl("file:/B"), Node(), Node()));

    // make sure the actual value is there
    QVERIFY(m_model->containsAnyStatement(m_model->listStatements(Node(), NIE::url(), QUrl("file:/A")).allStatements().first().subject(),
                                          QUrl("prop:/res"),
                                          m_model->listStatements(Node(), NIE::url(), QUrl("file:/B")).allStatements().first().subject()));


    // add the same relation but with another app
    m_dmModel->addProperty(QList<QUrl>() << QUrl("file:/A"), QUrl("prop:/res"), QVariantList() << QVariant(QUrl("file:/B")), QLatin1String("Otherapp"));

    // there is only one prop:/res relation defined
    QCOMPARE(m_model->listStatements(Node(), QUrl("prop:/res"), Node()).allStatements().count(), 1);

    // we now add two values for a property with cardinality 1
    m_dmModel->addProperty(QList<QUrl>() << QUrl("file:/A"), QUrl("prop:/res_c1"), QVariantList() << QVariant(QUrl("file:/B")), QLatin1String("Testapp"));
    m_dmModel->addProperty(QList<QUrl>() << QUrl("file:/A"), QUrl("prop:/res_c1"), QVariantList() << QVariant(QUrl("file:/C")), QLatin1String("Testapp"));

    // the second call needs to fail
    QVERIFY(m_dmModel->lastError());


    // get the res URI for file:/A
    const QUrl fileAUri = m_model->listStatements(Soprano::Node(), NIE::url(), QUrl("file:/A")).allStatements().first().subject().uri();

    // test adding a property to both the file and the resource URI. The result should be the exact same as doing it with only one of them
    m_dmModel->addProperty(QList<QUrl>() << fileAUri << QUrl("file:/A"), QUrl("prop:/string"), QVariantList() << QVariant(QLatin1String("Whatever")), QLatin1String("Testapp"));

    QCOMPARE(m_model->listStatements(fileAUri, QUrl("prop:/string"), LiteralValue(QLatin1String("Whatever"))).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(Node(), NIE::url(), QUrl("file:/A")).allStatements().count(), 1);

    // test the same with the file as object
    m_dmModel->addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/res"), QVariantList() << QVariant(QUrl(QLatin1String("file:/A"))) << QVariant(fileAUri), QLatin1String("Testapp"));

    QCOMPARE(m_model->listStatements(QUrl("res:/A"), QUrl("prop:/res"), fileAUri).allStatements().count(), 1);
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("file:/A")));
    QCOMPARE(m_model->listStatements(Node(), NIE::url(), QUrl("file:/A")).allStatements().count(), 1);
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
                                  .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::Agent()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::identifier()),
                                       Soprano::Node::literalToN3(QLatin1String("Testapp")),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::InstanceBase()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::GraphMetadata()),
                                       Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::coreGraphMetadataFor())),
                                  Soprano::Query::QueryLanguageSparql).boolValue());

    // check that we have an InstanceBase with a GraphMetadata graph
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { <res:/A> <prop:/string> %1 . } . "
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
}

void DataManagementModelTest::testRemoveProperty()
{
    const int cleanCount = m_model->statementCount();

    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/A"), Soprano::Vocabulary::NAO::lastModified(), LiteralValue(QDateTime::currentDateTime()), g1);

    m_dmModel->removeProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/string"), QVariantList() << QLatin1String("hello world"), QLatin1String("Testapp"));

    QVERIFY(!m_dmModel->lastError());

    // test that the data has been removed
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world"))));

    // test that the mtime has been updated (and is thus in another graph)
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Soprano::Vocabulary::NAO::lastModified(), Soprano::Node(), g1));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), Soprano::Vocabulary::NAO::lastModified(), Soprano::Node()));

    // test that the other property value is still valid
    QVERIFY(m_model->containsStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1));


    // step 2: remove the second value
    m_dmModel->removeProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/string"), QVariantList() << QLatin1String("foobar"), QLatin1String("Testapp"));

    QVERIFY(!m_dmModel->lastError());

    // the property should be gone entirely
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), Soprano::Node()));

    // even the resource should be gone since the NAO mtime does not count as a "real" property
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Soprano::Node(), Soprano::Node()));

    QTextStream s(stderr);
    m_model->write(s);

    // nothing except the ontology and the Testapp Agent should be left
    QCOMPARE(m_model->statementCount(), cleanCount+6);
}

void DataManagementModelTest::testRemoveProperty_file()
{
    // prepare some test data
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);

    m_model->addStatement(QUrl("res:/A"), NIE::url(), QUrl("file:/A"), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("whatever")), g1);

    m_model->addStatement(QUrl("res:/B"), NIE::url(), QUrl("file:/B"), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/B"), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/C"), g1);



    // now we remove one value via the file URL
    m_dmModel->removeProperty(QList<QUrl>() << QUrl("file:/A"), QUrl("prop:/string"), QVariantList() << QLatin1String("hello world"), QLatin1String("Testapp"));

    QCOMPARE(m_model->listStatements(QUrl("res:/A"), QUrl("prop:/string"), Node()).allStatements().count(), 2);
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), NIE::url(), Node()).allStatements().count(), 1);
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), NIE::url(), QUrl("file:/A")).allStatements().count(), 1);


    // test the same with a file URL value
    m_dmModel->removeProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/res"), QVariantList() << QUrl("file:/B"), QLatin1String("Testapp"));

    QCOMPARE(m_model->listStatements(QUrl("res:/A"), QUrl("prop:/res"), Node()).allStatements().count(), 1);
}

void DataManagementModelTest::testRemoveProperties()
{
    // prepare some test data
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);

    m_model->addStatement(QUrl("res:/A"), NIE::url(), QUrl("file:/A"), g1);
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

    m_model->addStatement(QUrl("res:/B"), NIE::url(), QUrl("file:/B"), g1);
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
    m_dmModel->removeProperties(QList<QUrl>() << QUrl("file:/A"), QList<QUrl>() << QUrl("prop:/int2"), QLatin1String("testapp"));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int2"), Node()));

    // TODO: verify graphs
}


void DataManagementModelTest::testRemoveResources()
{
    // prepare some test data
    QUrl mg1;
    const QUrl g1 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg1);

    m_model->addStatement(QUrl("res:/A"), NIE::url(), QUrl("file:/A"), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);

    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("whatever")), g1);

    QUrl mg2;
    const QUrl g2 = m_nrlModel->createGraph(NRL::InstanceBase(), &mg2);

    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/int"), LiteralValue(6), g2);
    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/int"), LiteralValue(12), g2);
    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/int"), LiteralValue(2), g2);
    m_model->addStatement(QUrl("res:/B"), NIE::url(), QUrl("file:/B"), g2);


    m_dmModel->removeResources(QList<QUrl>() << QUrl("res:/A"), QLatin1String("testapp"), false);

    // verify that the resource is gone
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));

    // verify that other resources were not touched
    QCOMPARE(m_model->listStatements(QUrl("res:/B"), Node(), Node()).allStatements().count(), 4);
    QCOMPARE(m_model->listStatements(QUrl("res:/C"), Node(), Node()).allStatements().count(), 2);

    // verify that removing resources by file URL works
    m_dmModel->removeResources(QList<QUrl>() << QUrl("file:/B"), QLatin1String("testapp"), false);

    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/B"), Node(), Node()));

    // verify that other resources were not touched
    QCOMPARE(m_model->listStatements(QUrl("res:/C"), Node(), Node()).allStatements().count(), 2);
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
    ResourceManager::instance()->setOverrideMainModel( m_model );

    //
    // Simple case: create a resource by merging it
    //
    SimpleResource res;
    res.setUri(QUrl("_:A"));
    res.m_properties.insert(RDF::type(), NAO::Tag());
    res.m_properties.insert(NAO::prefLabel(), QLatin1String("Foobar"));

    m_dmModel->mergeResources(SimpleResourceGraph() << res, QLatin1String("testapp"));

    // check if the resource exists
    m_model->containsAnyStatement(Soprano::Node(), RDF::type(), NAO::Tag());
    m_model->containsAnyStatement(Soprano::Node(), NAO::prefLabel(), Soprano::LiteralValue(QLatin1String("Foobar")));

    // check if all the correct metadata graphs exist
    QVERIFY(m_model->executeQuery(QString::fromLatin1("ask where { "
                                                      "graph ?g { ?r a %1 . ?r %2 %3 . } . "
                                                      "graph ?mg { ?g a %4 . ?mg a %5 . ?mg %6 ?g . } . "
                                                      "?g %7 ?a . ?a %8 %9 . "
                                                      "}")
                                  .arg(Soprano::Node::resourceToN3(NAO::Tag()),
                                       Soprano::Node::resourceToN3(NAO::prefLabel()),
                                       Soprano::Node::literalToN3(QLatin1String("Foobar")),
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
                                  .arg(Soprano::Node::resourceToN3(NAO::Tag()),
                                       Soprano::Node::resourceToN3(NAO::prefLabel()),
                                       Soprano::Node::literalToN3(QLatin1String("Foobar")),
                                       Soprano::Node::resourceToN3(NAO::maintainedBy()),
                                       Soprano::Node::resourceToN3(NAO::identifier()),
                                       Soprano::Node::literalToN3(QLatin1String("testapp")),
                                       Soprano::Node::literalToN3(QLatin1String("testapp2"))),
                                  Soprano::Query::QueryLanguageSparql).boolValue());
}

QTEST_KDEMAIN_CORE(DataManagementModelTest)


#include "datamanagementmodeltest.moc"
