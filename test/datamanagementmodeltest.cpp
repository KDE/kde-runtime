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

#include <QtTest>
#include "qtest_kde.h"

#include <Soprano/Soprano>
#define USING_SOPRANO_NRLMODEL_UNSTABLE_API
#include <Soprano/NRLModel>

#include <ktempdir.h>

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

QTEST_KDEMAIN_CORE(DataManagementModelTest)

#include "datamanagementmodeltest.moc"
