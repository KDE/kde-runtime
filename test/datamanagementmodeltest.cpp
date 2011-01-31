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


void DataManagementModelTest::testMergeResources()
{
    //Nepomuk::ResourceManager::instance()->setOverrideMainModel( m_model );
    using namespace Soprano::Vocabulary;
    using namespace Nepomuk::Vocabulary;
    
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
    
    m_dmModel->mergeResources( graph, QLatin1String("Testapp"), QHash<QUrl, QVariant>() );

    QVERIFY( !m_model->containsAnyStatement( QUrl("nepomuk:/mergeTest/res2"), QUrl(), QUrl() ) );
    
}

QTEST_KDEMAIN_CORE(DataManagementModelTest)

#include "datamanagementmodeltest.moc"
