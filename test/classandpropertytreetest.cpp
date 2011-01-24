/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-2011 Sebastian Trueg <trueg@kde.org>

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

#include "classandpropertytreetest.h"
#include "../classandpropertytree.h"

#include <QtTest>
#include "qtest_kde.h"

#include <Soprano/Soprano>

#include <ktempdir.h>

using namespace Soprano;

void ClassAndPropertyTreeTest::initTestCase()
{
    // we need to use a Virtuoso model as tmp model since redland misses important SPARQL features
    // that are used by libnepomuk below
    const Soprano::Backend* backend = Soprano::PluginManager::instance()->discoverBackendByName( "virtuosobackend" );
    QVERIFY( backend );
    m_storageDir = new KTempDir();
    m_model = backend->createModel( Soprano::BackendSettings() << Soprano::BackendSetting(Soprano::BackendOptionStorageDir, m_storageDir->name()) );
    QVERIFY( m_model );

    m_typeTree = new Nepomuk::ClassAndPropertyTree( this );
}

void ClassAndPropertyTreeTest::cleanupTestCase()
{
    delete m_model;
    delete m_typeTree;
    delete m_storageDir;
}


void ClassAndPropertyTreeTest::init()
{
    m_model->removeAllStatements();

    // we create one fake ontology
    //
    // situations we need to test:
    // * class that is marked visible should stay visible
    // * class that is marked invisible should stay invisible
    // * non-marked subclass of visible should be visible, too
    // * non-marked subclass of invisible should be invisible, too
    // * marked subclass should keep its own visiblity and not inherit from parent
    // * whole branch should inherit from parent
    // * if one parent is visible the class is visible, too, even if N other parents are not
    // * if all parents are invisible, the class is invisible, even if higher up in the branch a class is visible
    // * properly handle loops (as in: do not run into an endless loop)
    //
    // A
    // |- B - invisible
    //    |- C
    //       |- D - visible
    //          |- E
    //             |- F
    //    |- G
    //
    // AA - invisible
    // | - F
    // | - G
    //
    // X
    // |- Y - invisible
    //    |- Z
    //       |- X

    QUrl graph("graph:/onto");
    m_model->addStatement( graph, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::Ontology(), graph );

    m_model->addStatement( QUrl("onto:/A"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_model->addStatement( QUrl("onto:/B"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_model->addStatement( QUrl("onto:/C"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_model->addStatement( QUrl("onto:/D"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_model->addStatement( QUrl("onto:/E"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_model->addStatement( QUrl("onto:/F"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_model->addStatement( QUrl("onto:/G"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_model->addStatement( QUrl("onto:/AA"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_model->addStatement( QUrl("onto:/X"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_model->addStatement( QUrl("onto:/Y"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );
    m_model->addStatement( QUrl("onto:/Z"), Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::RDFS::Class(), graph );

    m_model->addStatement( QUrl("onto:/B"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/A"), graph );
    m_model->addStatement( QUrl("onto:/C"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/B"), graph );
    m_model->addStatement( QUrl("onto:/D"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/C"), graph );
    m_model->addStatement( QUrl("onto:/E"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/D"), graph );
    m_model->addStatement( QUrl("onto:/F"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/E"), graph );
    m_model->addStatement( QUrl("onto:/G"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/B"), graph );
    m_model->addStatement( QUrl("onto:/F"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/AA"), graph );
    m_model->addStatement( QUrl("onto:/G"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/AA"), graph );
    m_model->addStatement( QUrl("onto:/Y"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/X"), graph );
    m_model->addStatement( QUrl("onto:/Z"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/Y"), graph );
    m_model->addStatement( QUrl("onto:/X"), Soprano::Vocabulary::RDFS::subClassOf(), QUrl("onto:/Z"), graph );

    m_model->addStatement( QUrl("onto:/B"), Soprano::Vocabulary::NAO::userVisible(), LiteralValue(false), graph );
    m_model->addStatement( QUrl("onto:/D"), Soprano::Vocabulary::NAO::userVisible(), LiteralValue(true), graph );
    m_model->addStatement( QUrl("onto:/AA"), Soprano::Vocabulary::NAO::userVisible(), LiteralValue(false), graph );
    m_model->addStatement( QUrl("onto:/Y"), Soprano::Vocabulary::NAO::userVisible(), LiteralValue(false), graph );

    m_typeTree->rebuildTree(m_model);
}

void ClassAndPropertyTreeTest::testVisibility()
{
    QVERIFY(m_typeTree->isUserVisible(QUrl("onto:/A")));
    QVERIFY(!m_typeTree->isUserVisible(QUrl("onto:/B")));
    QVERIFY(!m_typeTree->isUserVisible(QUrl("onto:/C")));
    QVERIFY(m_typeTree->isUserVisible(QUrl("onto:/D")));
    QVERIFY(m_typeTree->isUserVisible(QUrl("onto:/E")));
    QVERIFY(m_typeTree->isUserVisible(QUrl("onto:/F")));
    QVERIFY(!m_typeTree->isUserVisible(QUrl("onto:/G")));
    QVERIFY(!m_typeTree->isUserVisible(QUrl("onto:/AA")));
    QVERIFY(!m_typeTree->isUserVisible(QUrl("onto:/X"))); // because only top-level classes inherit from rdfs:Resource
    QVERIFY(!m_typeTree->isUserVisible(QUrl("onto:/Y")));
    QVERIFY(!m_typeTree->isUserVisible(QUrl("onto:/Z")));
}

QTEST_KDEMAIN_CORE(ClassAndPropertyTreeTest)

void ClassAndPropertyTreeTest::testParents()
{
    QCOMPARE(m_typeTree->allParents(QUrl("onto:/A")).count(), 1);
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/A"), Soprano::Vocabulary::RDFS::Resource()));

    QCOMPARE(m_typeTree->allParents(QUrl("onto:/B")).count(), 2);
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/B"), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/B"), QUrl("onto:/A")));

    QCOMPARE(m_typeTree->allParents(QUrl("onto:/C")).count(), 3);
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/C"), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/C"), QUrl("onto:/A")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/C"), QUrl("onto:/B")));

    QCOMPARE(m_typeTree->allParents(QUrl("onto:/D")).count(), 4);
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/D"), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/D"), QUrl("onto:/A")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/D"), QUrl("onto:/B")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/D"), QUrl("onto:/C")));

    QCOMPARE(m_typeTree->allParents(QUrl("onto:/E")).count(), 5);
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/E"), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/E"), QUrl("onto:/A")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/E"), QUrl("onto:/B")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/E"), QUrl("onto:/C")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/E"), QUrl("onto:/D")));

    QCOMPARE(m_typeTree->allParents(QUrl("onto:/F")).count(), 7);
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/F"), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/F"), QUrl("onto:/A")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/F"), QUrl("onto:/B")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/F"), QUrl("onto:/C")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/F"), QUrl("onto:/D")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/F"), QUrl("onto:/E")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/F"), QUrl("onto:/AA")));

    QCOMPARE(m_typeTree->allParents(QUrl("onto:/G")).count(), 4);
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/G"), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/G"), QUrl("onto:/AA")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/G"), QUrl("onto:/A")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/G"), QUrl("onto:/B")));

    QCOMPARE(m_typeTree->allParents(QUrl("onto:/X")).count(), 3);
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/X"), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/X"), QUrl("onto:/Y")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/X"), QUrl("onto:/Z")));

    QCOMPARE(m_typeTree->allParents(QUrl("onto:/Y")).count(), 3);
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/Y"), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/Y"), QUrl("onto:/X")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/Y"), QUrl("onto:/Z")));

    QCOMPARE(m_typeTree->allParents(QUrl("onto:/Z")).count(), 3);
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/Z"), Soprano::Vocabulary::RDFS::Resource()));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/Z"), QUrl("onto:/X")));
    QVERIFY(m_typeTree->isChildOf(QUrl("onto:/Z"), QUrl("onto:/Y")));
}

#include "classandpropertytreetest.moc"
