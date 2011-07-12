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

#include "resourcewatchertest.h"
#include "../datamanagementmodel.h"
#include "../classandpropertytree.h"
#include "../resourcewatcherconnection.h"
#include "../resourcewatchermanager.h"

#include "simpleresource.h"
#include "simpleresourcegraph.h"

#include <QtTest>
#include "qtest_kde.h"
#include "qtest_dms.h"
#include <QStringList>
#include <Soprano/Soprano>
#include <Soprano/Graph>
#define USING_SOPRANO_NRLMODEL_UNSTABLE_API
#include <Soprano/NRLModel>

#include <KTemporaryFile>
#include <KTempDir>
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


void ResourceWatcherTest::resetModel()
{
    // remove all the junk from previous tests
    m_model->removeAllStatements();

    // add some classes and properties
    QUrl graph("graph:/onto");
    Nepomuk::insertOntologies( m_model, graph );

    // rebuild the internals of the data management model
    m_classAndPropertyTree->rebuildTree(m_dmModel);
}


void ResourceWatcherTest::initTestCase()
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

void ResourceWatcherTest::cleanupTestCase()
{
    delete m_dmModel;
    delete m_nrlModel;
    delete m_model;
    delete m_storageDir;
    delete m_classAndPropertyTree;
}

void ResourceWatcherTest::init()
{
    resetModel();
}

void ResourceWatcherTest::testPropertyAddedSignal()
{
    // create a dummy resource which we will use
    const QUrl resA = m_dmModel->createResource(QList<QUrl>() << QUrl("class:/typeA"), QString(), QString(), QLatin1String("A"));

    // no error should be generated after the above method is executed.
    QVERIFY(!m_dmModel->lastError());

    // create a connection which listens to changes in res:/A
    Nepomuk::ResourceWatcherConnection* con = m_dmModel->resourceWatcherManager()->createConnection(QList<QUrl>() << resA, QList<QUrl>(), QList<QUrl>());
    QVERIFY(!m_dmModel->lastError());

    // spy for the propertyAdded signal
    QSignalSpy spy(con, SIGNAL(propertyAdded(QString, QString, QVariant)));

    // change the resource
    m_dmModel->setProperty(QList<QUrl>() << resA, NAO::prefLabel(), QVariantList() << QLatin1String("foobar"), QLatin1String("A"));
    QVERIFY(!m_dmModel->lastError());

    // check that we actually got one signal
    QCOMPARE( spy.count(), 1 );

    // check that we got the correct values
    QList<QVariant> args = spy.takeFirst();

    // 1 param: the resource
    QCOMPARE(args[0].toString(), resA.toString());

    // 2 param: the property
    QCOMPARE(args[1].toString(), NAO::prefLabel().toString());

    // 3 param: the value
    QCOMPARE(args[2].value<QVariant>(), QVariant(QString(QLatin1String("foobar"))));

    // cleanup
    con->deleteLater();
}

void ResourceWatcherTest::testPropertyRemovedSignal()
{
    const QUrl resA = m_dmModel->createResource(QList<QUrl>() << QUrl("class:/typeA"), QLatin1String("foobar"), QString(), QLatin1String("A"));
    QVERIFY(!m_dmModel->lastError());

    Nepomuk::ResourceWatcherConnection* con = m_dmModel->resourceWatcherManager()->createConnection(QList<QUrl>() << resA, QList<QUrl>(), QList<QUrl>());

    QSignalSpy spy(con, SIGNAL(propertyRemoved(QString, QString, QVariant)));

    m_dmModel->removeProperty(QList<QUrl>() << resA, NAO::prefLabel(), QVariantList() << QLatin1String("foobar"), QLatin1String("A"));
    QVERIFY(!m_dmModel->lastError());

    QCOMPARE( spy.count(), 1 );

    QList<QVariant> args = spy.takeFirst();

    QCOMPARE(args[0].toString(), resA.toString());
    QCOMPARE(args[1].toString(), NAO::prefLabel().toString());
    QCOMPARE(args[2].value<QVariant>(), QVariant(QString(QLatin1String("foobar"))));

    con->deleteLater();
}

void ResourceWatcherTest::testResourceRemovedSignal()
{
    const QUrl resA = m_dmModel->createResource(QList<QUrl>() << QUrl("class:/typeA"), QString(), QString(), QLatin1String("A"));
    QVERIFY(!m_dmModel->lastError());

    Nepomuk::ResourceWatcherConnection* con = m_dmModel->resourceWatcherManager()->createConnection(QList<QUrl>() << resA, QList<QUrl>(), QList<QUrl>());
    QVERIFY(!m_dmModel->lastError());

    QSignalSpy spy(con, SIGNAL(resourceRemoved(QString, QStringList)));

    m_dmModel->removeResources(QList<QUrl>() << resA, Nepomuk::RemovalFlags() , QLatin1String("A"));
    QVERIFY(!m_dmModel->lastError());

    QCOMPARE( spy.count(), 1 );

    QList<QVariant> args = spy.takeFirst();

    QCOMPARE(args[0].toString(), resA.toString());

    con->deleteLater();
}

QTEST_KDEMAIN_CORE(ResourceWatcherTest)

#include "resourcewatchertest.moc"
