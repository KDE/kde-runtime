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

#include "datamanagementadaptortest.h"
#include "../datamanagementmodel.h"
#include "../datamanagementadaptor.h"
#include "../classandpropertytree.h"
#include "simpleresource.h"

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


void DataManagementAdaptorTest::resetModel()
{
    // remove all the junk from previous tests
    m_model->removeAllStatements();

    // add some classes and properties
    QUrl graph1("graph:/onto1/");
    m_model->addStatement( graph1, RDF::type(), NRL::Ontology(), graph1 );
    m_model->addStatement( graph1, NAO::hasDefaultNamespace(), QUrl("graph:/onto1/"), graph1 );
    m_model->addStatement( graph1, NAO::hasDefaultNamespaceAbbreviation(), LiteralValue(QLatin1String("itda")), graph1 );

    QUrl graph2("graph:/onto2#");
    m_model->addStatement( graph2, RDF::type(), NRL::Ontology(), graph2 );
    m_model->addStatement( graph2, NAO::hasDefaultNamespace(), QUrl("graph:/onto2#"), graph1 );
    m_model->addStatement( graph2, NAO::hasDefaultNamespaceAbbreviation(), LiteralValue(QLatin1String("wbzo")), graph1 );


    m_model->addStatement( QUrl("graph:/onto1/P1"), RDF::type(), RDF::Property(), graph1 );
    m_model->addStatement( QUrl("graph:/onto1/P2"), RDF::type(), RDF::Property(), graph1 );
    m_model->addStatement( QUrl("graph:/onto1/T1"), RDFS::Class(), RDF::Property(), graph1 );

    m_model->addStatement( QUrl("graph:/onto2#P1"), RDF::type(), RDF::Property(), graph2 );
    m_model->addStatement( QUrl("graph:/onto2#P2"), RDF::type(), RDF::Property(), graph2 );
    m_model->addStatement( QUrl("graph:/onto2#T1"), RDFS::Class(), RDF::Property(), graph2 );


    // rebuild the internals of the data management model
    m_dmModel->updateTypeCachesAndSoOn();
}

void DataManagementAdaptorTest::initTestCase()
{
    const Soprano::Backend* backend = Soprano::PluginManager::instance()->discoverBackendByName( "virtuosobackend" );
    QVERIFY( backend );
    m_storageDir = new KTempDir();
    m_model = backend->createModel( Soprano::BackendSettings() << Soprano::BackendSetting(Soprano::BackendOptionStorageDir, m_storageDir->name()) );
    QVERIFY( m_model );

    // DataManagementModel relies on the ussage of a NRLModel in the storage service
    m_nrlModel = new Soprano::NRLModel(m_model);

    m_dmModel = new Nepomuk::DataManagementModel(m_nrlModel);

    m_dmAdaptor = new Nepomuk::DataManagementAdaptor(m_dmModel);
}

void DataManagementAdaptorTest::cleanupTestCase()
{
    delete m_dmAdaptor;
    delete m_dmModel;
    delete m_nrlModel;
    delete m_model;
    delete m_storageDir;
}

void DataManagementAdaptorTest::init()
{
    resetModel();
}

void DataManagementAdaptorTest::testNamespaceExpansion()
{
    QCOMPARE(m_dmAdaptor->decodeUri(QLatin1String("itda:P1"), true), QUrl("graph:/onto1/P1"));
    QCOMPARE(m_dmAdaptor->decodeUri(QLatin1String("itda:T1"), true), QUrl("graph:/onto1/T1"));
    QCOMPARE(m_dmAdaptor->decodeUri(QLatin1String("wbzo:P1"), true), QUrl("graph:/onto2#P1"));
    QCOMPARE(m_dmAdaptor->decodeUri(QLatin1String("wbzo:T1"), true), QUrl("graph:/onto2#T1"));
}


QTEST_KDEMAIN_CORE(DataManagementAdaptorTest)

#include "datamanagementadaptortest.moc"
