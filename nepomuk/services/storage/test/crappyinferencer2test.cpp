/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

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

#include "crappyinferencer2test.h"
#include "../crappyinferencer2.h"

#include <QtTest>
#include "qtest_kde.h"

#include <Soprano/Soprano>

#include <ktempdir.h>

using namespace Soprano;

Q_DECLARE_METATYPE(Soprano::Statement)

void CrappyInferencer2Test::initTestCase()
{
    qRegisterMetaType<Soprano::Statement>();

    // we need to use a Virtuoso model as tmp model since redland misses important SPARQL features
    // that are used by libnepomuk below
    const Soprano::Backend* backend = Soprano::PluginManager::instance()->discoverBackendByName( "virtuosobackend" );
    QVERIFY( backend );
    m_storageDir = new KTempDir();
    m_baseModel = backend->createModel( Soprano::BackendSettings() << Soprano::BackendSetting(Soprano::BackendOptionStorageDir, m_storageDir->name()) );
    QVERIFY( m_baseModel );

    m_model = new CrappyInferencer2( m_baseModel );
}

void CrappyInferencer2Test::cleanupTestCase()
{
    delete m_model;
    delete m_baseModel;
    delete m_storageDir;
}


void CrappyInferencer2Test::init()
{
    m_baseModel->removeAllStatements();

    //
    // we create one fake ontology
    //
    // A
    // |- B
    //    |- C - invisible
    //       |- D
    //       |- E
    //
    // F
    // |- E
    //
    // L1
    // |- L2
    //    |- L1
    //
    QUrl graph("graph:/onto");
    m_baseModel->addStatement( graph, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::Ontology(), graph );
    m_baseModel->addStatement( QUrl("nepomuk:/G"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::InstanceBase(), graph );

    m_baseModel->addStatement( QUrl("onto:/A"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_baseModel->addStatement( QUrl("onto:/B"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_baseModel->addStatement( QUrl("onto:/C"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_baseModel->addStatement( QUrl("onto:/D"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_baseModel->addStatement( QUrl("onto:/E"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_baseModel->addStatement( QUrl("onto:/F"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );

    m_baseModel->addStatement( QUrl("onto:/E"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/C"), graph );
    m_baseModel->addStatement( QUrl("onto:/B"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/A"), graph );
    m_baseModel->addStatement( QUrl("onto:/C"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/B"), graph );
    m_baseModel->addStatement( QUrl("onto:/D"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/C"), graph );
    m_baseModel->addStatement( QUrl("onto:/E"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/F"), graph );

    m_model->addStatement( QUrl("onto:/C"), Soprano::Vocabulary::NAO::userVisible(), LiteralValue(false), graph );

    // the loopy classes
    m_baseModel->addStatement( QUrl("onto:/L1"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_baseModel->addStatement( QUrl("onto:/L1"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/L2"), graph );
    m_baseModel->addStatement( QUrl("onto:/L2"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_baseModel->addStatement( QUrl("onto:/L2"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/L1"), graph );

    m_model->updateInferenceIndex();
}

// make sure only the one statement is added
void CrappyInferencer2Test::testAddAnyStatement()
{
    const int cnt = m_model->statementCount();
    Statement s( QUrl("nepomuk:/A"), Soprano::Vocabulary::NAO::hasTag(), QUrl("nepomuk:/T"), QUrl("nepomuk:/G") );
    QVERIFY(!m_model->addStatement(s));
    QCOMPARE(m_model->statementCount(), cnt+1);
    QVERIFY(m_model->containsStatement(s));
}

// make sure all superclasses are added as type, too
void CrappyInferencer2Test::testAddTypeStatement()
{
    int cnt = m_model->statementCount();
    QUrl resA("nepomuk:/A");
    m_model->addStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/A"), QUrl("nepomuk:/G"));
    QCOMPARE(m_model->statementCount(), cnt+3);
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::NAO::userVisible(), LiteralValue(1)));

    cnt = m_model->statementCount();
    QUrl resB("nepomuk:/B");
    m_model->addStatement(resB, Soprano::Vocabulary::RDF::type(), QUrl("onto:/B"), QUrl("nepomuk:/G"));
    QCOMPARE(m_model->statementCount(), cnt+4);
    QVERIFY(m_model->containsAnyStatement(resB, Vocabulary::RDF::type(), QUrl("onto:/B")));
    QVERIFY(m_model->containsAnyStatement(resB, Vocabulary::RDF::type(), QUrl("onto:/A")));
    QVERIFY(m_model->containsAnyStatement(resB, Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_model->containsAnyStatement(resB, Vocabulary::NAO::userVisible(), LiteralValue(1)));

    cnt = m_model->statementCount();
    QUrl resC("nepomuk:/C");
    m_model->addStatement(resC, Soprano::Vocabulary::RDF::type(), QUrl("onto:/C"), QUrl("nepomuk:/G"));
    QCOMPARE(m_model->statementCount(), cnt+4);
    QVERIFY(m_model->containsAnyStatement(resC, Vocabulary::RDF::type(), QUrl("onto:/C")));
    QVERIFY(m_model->containsAnyStatement(resC, Vocabulary::RDF::type(), QUrl("onto:/B")));
    QVERIFY(m_model->containsAnyStatement(resC, Vocabulary::RDF::type(), QUrl("onto:/A")));
    QVERIFY(m_model->containsAnyStatement(resC, Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Resource()));

    cnt = m_model->statementCount();
    QUrl resD("nepomuk:/D");
    m_model->addStatement(resD, Soprano::Vocabulary::RDF::type(), QUrl("onto:/E"), QUrl("nepomuk:/G"));
    QCOMPARE(m_model->statementCount(), cnt+7);
    QVERIFY(m_model->containsAnyStatement(resD, Vocabulary::RDF::type(), QUrl("onto:/F")));
    QVERIFY(m_model->containsAnyStatement(resD, Vocabulary::RDF::type(), QUrl("onto:/E")));
    QVERIFY(m_model->containsAnyStatement(resD, Vocabulary::RDF::type(), QUrl("onto:/C")));
    QVERIFY(m_model->containsAnyStatement(resD, Vocabulary::RDF::type(), QUrl("onto:/B")));
    QVERIFY(m_model->containsAnyStatement(resD, Vocabulary::RDF::type(), QUrl("onto:/A")));
    QVERIFY(m_model->containsAnyStatement(resD, Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_model->containsAnyStatement(resD, Vocabulary::NAO::userVisible(), LiteralValue(1)));
}

// make sure the new relation is taken into account with the next addStatement
void CrappyInferencer2Test::testAddSubClassOfStatement()
{
    QEXPECT_FAIL("", "Not implemented yet", Abort);
}

// make sure only the one statement is removed
void CrappyInferencer2Test::testRemoveAnyStatement()
{
}

// make sure the appropriate types are removed, too
// check 2 cases: 1. only the one type, 2. two types with intersecting superclasses
void CrappyInferencer2Test::testRemoveTypeStatement()
{
    int cnt = m_model->statementCount();
    QUrl resA("nepomuk:/A");
    m_model->addStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/A"), QUrl("nepomuk:/G"));
    m_model->removeStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/A"), QUrl("nepomuk:/G"));
    QCOMPARE(m_model->statementCount(), cnt);
    QVERIFY(!m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), QUrl("onto:/A")));
    QVERIFY(!m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(!m_model->containsAnyStatement(resA, Vocabulary::NAO::userVisible(), LiteralValue(1)));

    // add two types with intersecting super types
    m_model->addStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/B"), QUrl("nepomuk:/G"));
    m_model->addStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/E"), QUrl("nepomuk:/G"));
    // remove one of them
    m_model->removeStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/B"), QUrl("nepomuk:/G"));
    // make sure all is well
    QCOMPARE(m_model->statementCount(), cnt+7);
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), QUrl("onto:/F")));
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), QUrl("onto:/E")));
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), QUrl("onto:/C")));
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), QUrl("onto:/B")));
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), QUrl("onto:/A")));
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::NAO::userVisible(), LiteralValue(1)));
}

// make sure the removed subclass relation is taken into account with the next addStatement
// make sure type statements that are no longer valid are removed
void CrappyInferencer2Test::testRemoveSubClassOfStatement()
{
    QEXPECT_FAIL("", "Not implemented yet", Abort);
}

// make sure the corresponing statements are removed and nothing else
// test all combinations of invalid and valid nodes if possible
void CrappyInferencer2Test::testRemoveAllStatements()
{
    // 1. remove the whole resource
    int cnt = m_model->statementCount();
    QUrl resA("nepomuk:/A");
    m_model->addStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/A"), QUrl("nepomuk:/G"));
    m_model->addStatement(resA, Soprano::Vocabulary::RDFS::label(), LiteralValue(QLatin1String("A")), QUrl("nepomuk:/G"));
    m_model->removeAllStatements(resA, Node(), Node());
    QCOMPARE(m_model->statementCount(), cnt);
    QVERIFY(!m_model->containsAnyStatement(resA, Node(), Node(), Node()));

    // 2. remove a type from several resources
    QUrl resB("nepomuk:/B");
    m_model->addStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/A"), QUrl("nepomuk:/G"));
    m_model->addStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/F"), QUrl("nepomuk:/G"));
    m_model->addStatement(resB, Soprano::Vocabulary::RDF::type(), QUrl("onto:/B"), QUrl("nepomuk:/G"));
    m_model->addStatement(resB, Soprano::Vocabulary::RDF::type(), QUrl("onto:/F"), QUrl("nepomuk:/G"));
    m_model->removeAllStatements(Node(), Soprano::Vocabulary::RDF::type(), QUrl("onto:/F"));
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), QUrl("onto:/A")));
    QVERIFY(!m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), QUrl("onto:/F")));
    QVERIFY(m_model->containsAnyStatement(resB, Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_model->containsAnyStatement(resB, Vocabulary::RDF::type(), QUrl("onto:/A")));
    QVERIFY(m_model->containsAnyStatement(resB, Vocabulary::RDF::type(), QUrl("onto:/B")));
    QVERIFY(!m_model->containsAnyStatement(resB, Vocabulary::RDF::type(), QUrl("onto:/F")));
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::NAO::userVisible(), LiteralValue(1)));
    QVERIFY(m_model->containsAnyStatement(resB, Vocabulary::NAO::userVisible(), LiteralValue(1)));
    QCOMPARE(m_model->statementCount(), cnt+7);

    // 3. remove all types from one resource
    m_model->addStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/E"), QUrl("nepomuk:/G"));
    m_model->removeAllStatements(resA, Soprano::Vocabulary::RDF::type(), Node(), QUrl("nepomuk:/G"));
    QVERIFY(!m_model->containsAnyStatement(resA, Soprano::Vocabulary::RDF::type(), Node()));
    QVERIFY(!m_model->containsAnyStatement(resA, Vocabulary::NAO::userVisible(), LiteralValue(1)));

    // 4. remove a graph
    m_model->addStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/E"), QUrl("nepomuk:/G"));
    m_model->addStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/A"), QUrl("nepomuk:/G2"));
    m_model->removeContext(QUrl("nepomuk:/G"));
    QVERIFY(!m_model->containsAnyStatement(resB, Node(), Node()));
    QVERIFY(m_model->containsAnyStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/A")));
    QVERIFY(!m_model->containsAnyStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/E")));
    QVERIFY(!m_model->containsAnyStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/D")));
    QVERIFY(!m_model->containsAnyStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/C")));
    QVERIFY(!m_model->containsAnyStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/B")));
    QVERIFY(m_model->containsAnyStatement(resA, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(!m_model->containsContext(QUrl("nepomuk:/G")));
}

// make sure we do not run into an endless loop
void CrappyInferencer2Test::testCyclicSubClassRelation()
{
    int cnt = m_model->statementCount();
    QUrl resA("nepomuk:/A");
    m_model->addStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/L1"), QUrl("nepomuk:/G"));
    QCOMPARE(m_model->statementCount(), cnt+4); // +4 since L1 is there twice: the one we added and the superclass of L2
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), QUrl("onto:/L1")));
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), QUrl("onto:/L2")));
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Resource()));
}

void CrappyInferencer2Test::testUpdateAllResources()
{
    // we add a bunch of statements to the base model instead of going through the crappy inference model
    QUrl resA("nepomuk:/A");
    QUrl resB("nepomuk:/B");
    QUrl resC("nepomuk:/C");
    QUrl resD("nepomuk:/D");

    m_baseModel->addStatement(resA, Soprano::Vocabulary::RDF::type(), QUrl("onto:/A"), QUrl("nepomuk:/G"));
    m_baseModel->addStatement(resB, Soprano::Vocabulary::RDF::type(), QUrl("onto:/B"), QUrl("nepomuk:/G"));
    m_baseModel->addStatement(resC, Soprano::Vocabulary::RDF::type(), QUrl("onto:/C"), QUrl("nepomuk:/G"));
    m_baseModel->addStatement(resD, Soprano::Vocabulary::RDF::type(), QUrl("onto:/E"), QUrl("nepomuk:/G"));

    m_model->updateAllResources();

    // wait for the above call to finish
    QEventLoop loop;
    connect(m_model, SIGNAL(allResourcesUpdated()), &loop, SLOT(quit()));
    loop.exec();

    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), QUrl("onto:/A")));
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_model->containsAnyStatement(resA, Vocabulary::NAO::userVisible(), LiteralValue(1)));

    QVERIFY(m_model->containsAnyStatement(resB, Vocabulary::RDF::type(), QUrl("onto:/B")));
    QVERIFY(m_model->containsAnyStatement(resB, Vocabulary::RDF::type(), QUrl("onto:/A")));
    QVERIFY(m_model->containsAnyStatement(resB, Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_model->containsAnyStatement(resB, Vocabulary::NAO::userVisible(), LiteralValue(1)));

    QVERIFY(m_model->containsAnyStatement(resC, Vocabulary::RDF::type(), QUrl("onto:/C")));
    QVERIFY(m_model->containsAnyStatement(resC, Vocabulary::RDF::type(), QUrl("onto:/B")));
    QVERIFY(m_model->containsAnyStatement(resC, Vocabulary::RDF::type(), QUrl("onto:/A")));
    QVERIFY(m_model->containsAnyStatement(resC, Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(!m_model->containsAnyStatement(resC, Vocabulary::NAO::userVisible(), LiteralValue(1)));

    QVERIFY(m_model->containsAnyStatement(resD, Vocabulary::RDF::type(), QUrl("onto:/F")));
    QVERIFY(m_model->containsAnyStatement(resD, Vocabulary::RDF::type(), QUrl("onto:/E")));
    QVERIFY(m_model->containsAnyStatement(resD, Vocabulary::RDF::type(), QUrl("onto:/C")));
    QVERIFY(m_model->containsAnyStatement(resD, Vocabulary::RDF::type(), QUrl("onto:/B")));
    QVERIFY(m_model->containsAnyStatement(resD, Vocabulary::RDF::type(), QUrl("onto:/A")));
    QVERIFY(m_model->containsAnyStatement(resD, Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_model->containsAnyStatement(resD, Vocabulary::NAO::userVisible(), LiteralValue(1)));
}

QTEST_KDEMAIN_CORE(CrappyInferencer2Test)

#include "crappyinferencer2test.moc"
