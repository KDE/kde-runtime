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

#include "asyncclientapitest.h"
#include "../datamanagementmodel.h"
#include "../datamanagementadaptor.h"
#include "../classandpropertytree.h"
#include "simpleresource.h"
#include "simpleresourcegraph.h"
#include "datamanagement.h"
#include "createresourcejob.h"
#include "describeresourcesjob.h"
#include "storeresourcesjob.h"
#include "nepomuk_dms_test_config.h"
#include "qtest_dms.h"

#include <QtTest>
#include "qtest_kde.h"

#include <QtDBus>
#include <QProcess>
#include <Soprano/Soprano>
#include <Soprano/Client/DBusModel>

#include <Soprano/Graph>
#define USING_SOPRANO_NRLMODEL_UNSTABLE_API
#include <Soprano/NRLModel>

#include <ktempdir.h>
#include <KDebug>
#include <KJob>

#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NMM>
#include <Nepomuk/Vocabulary/NCO>
#include <Nepomuk/Vocabulary/NIE>

using namespace Soprano;
using namespace Soprano::Vocabulary;
using namespace Nepomuk;
using namespace Nepomuk::Vocabulary;


void AsyncClientApiTest::initTestCase()
{
    kDebug() << "Starting fake DMS:" << FAKEDMS_BIN;

    // setup the service watcher so we know when the fake DMS is up
    QDBusServiceWatcher watcher(QLatin1String("org.kde.nepomuk.FakeDataManagement"),
                                QDBusConnection::sessionBus(),
                                QDBusServiceWatcher::WatchForRegistration);

    // start the fake DMS
    m_fakeDms = new QProcess();
    m_fakeDms->setProcessChannelMode(QProcess::ForwardedChannels);
    m_fakeDms->start(QLatin1String(FAKEDMS_BIN));

    // wait for it to come up
    QTest::kWaitForSignal(&watcher, SIGNAL(serviceRegistered(QString)));

    // get us access to the fake DMS's model
    m_model = new Soprano::Client::DBusModel(QLatin1String("org.kde.nepomuk.FakeDataManagement"), QLatin1String("/model"));

    qputenv("NEPOMUK_FAKE_DMS_DBUS_SERVICE", "org.kde.nepomuk.FakeDataManagement");
}

void AsyncClientApiTest::cleanupTestCase()
{
    kDebug() << "Shutting down fake DMS...";
    QDBusInterface(QLatin1String("org.kde.nepomuk.FakeDataManagement"),
                   QLatin1String("/MainApplication"),
                   QLatin1String("org.kde.KApplication"),
                   QDBusConnection::sessionBus()).call(QLatin1String("quit"));
    m_fakeDms->waitForFinished();
    delete m_fakeDms;
    delete m_model;
}

void AsyncClientApiTest::resetModel()
{
    // remove all the junk from previous tests
    m_model->removeAllStatements();

    // add some classes and properties
    QUrl graph("graph:/onto");
    Nepomuk::insertOntologies( m_model, graph );
    
    // rebuild the internals of the data management model
    QDBusInterface(QLatin1String("org.kde.nepomuk.FakeDataManagement"),
                   QLatin1String("/fakedms"),
                   QLatin1String("org.kde.nepomuk.FakeDataManagement"),
                   QDBusConnection::sessionBus()).call(QLatin1String("updateClassAndPropertyTree"));
}

void AsyncClientApiTest::init()
{
    resetModel();
}


void AsyncClientApiTest::testAddProperty()
{
    Soprano::NRLModel nrlModel(m_model);
    const QUrl g1 = nrlModel.createGraph(NRL::InstanceBase());

    m_model->addStatement(QUrl("res:/A"), RDF::type(), QUrl("class:/typeA"), g1);
    
    KJob* job = Nepomuk::addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/int"), QVariantList() << 42);
    QTest::kWaitForSignal(job, SIGNAL(result(KJob*)), 5000);
    QVERIFY(!job->error());

    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42)));
}

void AsyncClientApiTest::testSetProperty()
{
    Soprano::NRLModel nrlModel(m_model);
    const QUrl g1 = nrlModel.createGraph(NRL::InstanceBase());

    m_model->addStatement(QUrl("res:/A"), RDF::type(), QUrl("class:/typeA"), g1);

    KJob* job = Nepomuk::setProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/int"), QVariantList() << 42);
    QTest::kWaitForSignal(job, SIGNAL(result(KJob*)), 5000);
    QVERIFY(!job->error());

    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42)));
}

void AsyncClientApiTest::testRemoveProperties()
{
    Soprano::NRLModel nrlModel(m_model);
    const QUrl g1 = nrlModel.createGraph(NRL::InstanceBase());

    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(12), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(2), g1);

    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(12), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/int2"), LiteralValue(2), g1);

    KJob* job = Nepomuk::removeProperties(QList<QUrl>() << QUrl("res:/A"), QList<QUrl>() << QUrl("prop:/int") << QUrl("prop:/int2"));
    QTest::kWaitForSignal(job, SIGNAL(result(KJob*)), 5000);
    QVERIFY(!job->error());

    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int"), Node()));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int2"), Node()));
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));
}

void AsyncClientApiTest::testCreateResource()
{
    CreateResourceJob* job = Nepomuk::createResource(QList<QUrl>() << QUrl("class:/typeA") << QUrl("class:/typeB"), QLatin1String("label"), QLatin1String("desc"));
    QTest::kWaitForSignal(job, SIGNAL(result(KJob*)), 5000);
    QVERIFY(!job->error());

    const QUrl uri = job->resourceUri();
    QVERIFY(!uri.isEmpty());

    QVERIFY(m_model->containsAnyStatement(uri, RDF::type(), QUrl("class:/typeA")));
    QVERIFY(m_model->containsAnyStatement(uri, RDF::type(), QUrl("class:/typeB")));
    QVERIFY(m_model->containsAnyStatement(uri, NAO::prefLabel(), LiteralValue::createPlainLiteral(QLatin1String("label"))));
    QVERIFY(m_model->containsAnyStatement(uri, NAO::description(), LiteralValue::createPlainLiteral(QLatin1String("desc"))));
}

void AsyncClientApiTest::testRemoveProperty()
{
    Soprano::NRLModel nrlModel(m_model);
    const QUrl g1 = nrlModel.createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::lastModified(), LiteralValue(QDateTime::currentDateTime()), g1);

    KJob* job = Nepomuk::removeProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/string"), QVariantList() << QLatin1String("hello world"));
    QTest::kWaitForSignal(job, SIGNAL(result(KJob*)), 5000);
    QVERIFY(!job->error());

    // test that the data has been removed
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world"))));
}

void AsyncClientApiTest::testRemoveResources()
{
    Soprano::NRLModel nrlModel(m_model);
    const QUrl g1 = nrlModel.createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("res:/A"), RDF::type(), NAO::Tag(), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g1);

    KJob* job = Nepomuk::removeResources(QList<QUrl>() << QUrl("res:/A"), NoRemovalFlags);
    QTest::kWaitForSignal(job, SIGNAL(result(KJob*)), 5000);
    QVERIFY(!job->error());

    // verify that the resource is gone
    QVERIFY(!m_model->containsAnyStatement(QUrl("res:/A"), Node(), Node()));
}

void AsyncClientApiTest::testRemoveDataByApplication()
{
    Soprano::NRLModel nrlModel(m_model);

    // create our apps (we need to use the component name for the first one as that will be reused in the call below)
    QUrl appG = nrlModel.createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/A"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/A"), NAO::identifier(), LiteralValue(KGlobal::mainComponent().componentName()), appG);
    appG = nrlModel.createGraph(NRL::InstanceBase());
    m_model->addStatement(QUrl("app:/B"), RDF::type(), NAO::Agent(), appG);
    m_model->addStatement(QUrl("app:/B"), NAO::identifier(), LiteralValue(QLatin1String("B")), appG);

    // create the resource to delete
    QUrl mg1;
    const QUrl g1 = nrlModel.createGraph(NRL::InstanceBase(), &mg1);
    m_model->addStatement(g1, NAO::maintainedBy(), QUrl("app:/A"), mg1);

    QUrl mg2;
    const QUrl g2 = nrlModel.createGraph(NRL::InstanceBase(), &mg2);
    m_model->addStatement(g2, NAO::maintainedBy(), QUrl("app:/B"), mg2);

    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g2);

    // delete the resource
    KJob* job = Nepomuk::removeDataByApplication(QList<QUrl>() << QUrl("res:/A"), NoRemovalFlags);
    QTest::kWaitForSignal(job, SIGNAL(result(KJob*)), 5000);
    QVERIFY(!job->error());

    // verify that graph1 is gone completely
    QVERIFY(!m_model->containsAnyStatement(Node(), Node(), Node(), g1));

    // only two statements left: the one in the second graph and the last modification date
    QCOMPARE(m_model->listStatements(QUrl("res:/A"), Node(), Node()).allStatements().count(), 2);
    QVERIFY(m_model->containsStatement(QUrl("res:/A"), QUrl("prop:/string"), LiteralValue(QLatin1String("hello world")), g2));
    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), NAO::lastModified(), Node()));

    // four graphs: g2, the 2 app graphs, and the mtime graph
    QCOMPARE(m_model->listStatements(Node(), RDF::type(), NRL::InstanceBase()).allStatements().count(), 4);
}

void AsyncClientApiTest::testStoreResources()
{
    // store a resource just to check if the method is called properly
    // and all types are property handled
    SimpleResource res;
    res.setUri(QUrl("_:A"));
    res.addProperty(RDF::type(), NAO::Tag());
    res.addProperty(QUrl("prop:/string"), QLatin1String("Foobar"));
    res.addProperty(QUrl("prop:/int"), 42);
    res.addProperty(QUrl("prop:/date"), QDate::currentDate());
    res.addProperty(QUrl("prop:/time"), QTime::currentTime());
    res.addProperty(QUrl("prop:/dateTime"), QDateTime::currentDateTime());
    
    KJob* job = Nepomuk::storeResources(SimpleResourceGraph() << res);
    QTest::kWaitForSignal(job, SIGNAL(result(KJob*)), 5000);
    QVERIFY(!job->error());

    // check if the resource exists
    QVERIFY(m_model->containsAnyStatement(Soprano::Node(), RDF::type(), NAO::Tag()));
    QVERIFY(m_model->containsAnyStatement(Soprano::Node(), QUrl("prop:/string"), Soprano::LiteralValue(QLatin1String("Foobar"))));
    QVERIFY(m_model->containsAnyStatement(Soprano::Node(), QUrl("prop:/int"), Soprano::LiteralValue(42)));
    QVERIFY(m_model->containsAnyStatement(Soprano::Node(), QUrl("prop:/date"), Soprano::LiteralValue(res.property(QUrl("prop:/date")).first().toDate())));
    QVERIFY(m_model->containsAnyStatement(Soprano::Node(), QUrl("prop:/time"), Soprano::LiteralValue(res.property(QUrl("prop:/time")).first().toTime())));
    QVERIFY(m_model->containsAnyStatement(Soprano::Node(), QUrl("prop:/dateTime"), Soprano::LiteralValue(res.property(QUrl("prop:/dateTime")).first().toDateTime())));
}

void AsyncClientApiTest::testMergeResources()
{
    // create some resources
    Soprano::NRLModel nrlModel(m_model);
    const QUrl g1 = nrlModel.createGraph(NRL::InstanceBase());

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

    KJob* job = Nepomuk::mergeResources(QUrl("res:/A"), QUrl("res:/B"));
    QTest::kWaitForSignal(job, SIGNAL(result(KJob*)), 5000);
    QVERIFY(!job->error());

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
}

void AsyncClientApiTest::testDescribeResources()
{
    // create some resources
    Soprano::NRLModel nrlModel(m_model);
    const QUrl g1 = nrlModel.createGraph(NRL::InstanceBase());

    m_model->addStatement(QUrl("res:/A"), RDF::type(), NAO::Tag(), g1);
    m_model->addStatement(QUrl("res:/A"), QUrl("prop:/res"), QUrl("res:/B"), g1);
    m_model->addStatement(QUrl("res:/A"), NAO::hasSubResource(), QUrl("res:/B"), g1);

    m_model->addStatement(QUrl("res:/B"), QUrl("prop:/string"), LiteralValue(QLatin1String("foobar")), g1);

    m_model->addStatement(QUrl("res:/C"), QUrl("prop:/int"), LiteralValue(42), g1);
    m_model->addStatement(QUrl("res:/C"), NAO::hasSubResource(), QUrl("res:/D"), g1);

    m_model->addStatement(QUrl("res:/D"), QUrl("prop:/string"), LiteralValue(QLatin1String("Hello")), g1);


    // we only use one of the test cases from the dms test: get two resources with subresoruces
    DescribeResourcesJob* job = Nepomuk::describeResources(QList<QUrl>() << QUrl("res:/A") << QUrl("res:/C"));
    QTest::kWaitForSignal(job, SIGNAL(result(KJob*)), 5000);
    QVERIFY(!job->error());

    QList<SimpleResource> g = job->resources().toList();

    // only one resource in the result
    QCOMPARE(g.count(), 4);

    // the results are res:/A, res:/B, res:/C and res:/D
    QList<SimpleResource>::const_iterator it = g.constBegin();
    SimpleResource r1 = *it;
    ++it;
    SimpleResource r2 = *it;
    ++it;
    SimpleResource r3 = *it;
    ++it;
    SimpleResource r4 = *it;
    QVERIFY(r1.uri() == QUrl("res:/A") || r2.uri() == QUrl("res:/A") || r3.uri() == QUrl("res:/A") || r4.uri() == QUrl("res:/A"));
    QVERIFY(r1.uri() == QUrl("res:/B") || r2.uri() == QUrl("res:/B") || r3.uri() == QUrl("res:/B") || r4.uri() == QUrl("res:/B"));
    QVERIFY(r1.uri() == QUrl("res:/C") || r2.uri() == QUrl("res:/C") || r3.uri() == QUrl("res:/C") || r4.uri() == QUrl("res:/C"));
    QVERIFY(r1.uri() == QUrl("res:/D") || r2.uri() == QUrl("res:/D") || r3.uri() == QUrl("res:/D") || r4.uri() == QUrl("res:/D"));
}

void AsyncClientApiTest::testImportResources()
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
    KJob* job = Nepomuk::importResources(QUrl::fromLocalFile(tmp.fileName()), Soprano::SerializationNTriples);
    QTest::kWaitForSignal(job, SIGNAL(result(KJob*)), 5000);
    QVERIFY(!job->error());


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

QTEST_KDEMAIN_CORE(AsyncClientApiTest)

#include "asyncclientapitest.moc"
