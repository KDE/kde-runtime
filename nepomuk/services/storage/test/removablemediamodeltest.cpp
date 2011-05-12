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

#include "removablemediamodeltest.h"

#define private public
#include "../removablemediamodel.h"
#undef private

#include <QtTest>
#include "qtest_kde.h"
#include "qtest_dms.h"

#include <QtDBus>
#include <Soprano/Soprano>
#include <KDebug>

#include <Solid/DeviceNotifier>
#include <Solid/DeviceInterface>
#include <Solid/Block>
#include <Solid/Device>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>
#include <Solid/StorageAccess>
#include <Solid/Predicate>

#include <Nepomuk/Vocabulary/NIE>

#ifndef FAKE_COMPUTER_XML
    #error "FAKE_COMPUTER_XML not set. An XML file describing a computer is required for this test"
#endif

#define SOLID_FAKEHW_SERVICE QDBusConnection::sessionBus().baseService()
#define SOLID_FAKEHW_PATH "/org/kde/solid/fakehw"
#define SOLID_FAKEHW_INTERFACE "local.qttest.Solid.Backends.Fake.FakeManager"

// TODO: also test mounting a different device to the same mount path

using namespace Nepomuk;
using namespace Nepomuk::Vocabulary;
using namespace Soprano;

Q_DECLARE_METATYPE(Soprano::Node)
Q_DECLARE_METATYPE(Soprano::Statement)


namespace {
/// Plug a device in the fake Solid hw manager.
void plugDevice(const QString& udi) {
    QDBusInterface(SOLID_FAKEHW_SERVICE, SOLID_FAKEHW_PATH, SOLID_FAKEHW_INTERFACE, QDBusConnection::sessionBus())
            .call(QLatin1String("plug"), udi);
}

/// Unplug a device in the fake Solid hw manager.
void unplugDevice(const QString& udi) {
    QDBusInterface(SOLID_FAKEHW_SERVICE, SOLID_FAKEHW_PATH, SOLID_FAKEHW_INTERFACE, QDBusConnection::sessionBus())
            .call(QLatin1String("unplug"), udi);
}

const char* s_udiXyz123 = "/org/kde/solid/fakehw/volume_part1_size_993284096";
}

void RemovableMediaModelTest::initTestCase()
{
    // make sure Solid uses the fake manager
    setenv("SOLID_FAKEHW", FAKE_COMPUTER_XML, 1);

    // we simply need some memory model for now - nothing fancy
    m_model = Soprano::createModel();
    m_model->setParent(this);
    m_rmModel = new RemovableMediaModel(m_model, this);
}


void RemovableMediaModelTest::testConvertFileUrlsInStatement_data()
{
    QTest::addColumn<Statement>( "original" );
    QTest::addColumn<Statement>( "converted" );

    const Statement randomStatement(QUrl("nepomuk:/res/xyz"), QUrl("onto:someProp"), LiteralValue("foobar"));
    QTest::newRow("noFileUrls") << randomStatement << randomStatement;

    const Statement randomFileSubject(QUrl("file:///tmp/test"), QUrl("onto:someProp"), LiteralValue("foobar"));
    QTest::newRow("randomFileUrlInSubject") << randomFileSubject << randomFileSubject;

    const Statement convertableFileSubject(QUrl("file:///media/XO-Y4/test.txt"), QUrl("onto:someProp"), LiteralValue("foobar"));
    QTest::newRow("convertableFileUrlInSubject") << convertableFileSubject << convertableFileSubject;

    const Statement convertableFileObjectWithoutNieUrl(QUrl("nepomuk:/res/xyz"), QUrl("onto:someProp"), QUrl("file:///media/XO-Y4/test.txt"));
    QTest::newRow("convertableFileUrlInObjectWithoutNieUrl") << convertableFileObjectWithoutNieUrl << convertableFileObjectWithoutNieUrl;

    const Statement convertableFileObjectWithNieUrl1_original(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("file:///media/XO-Y4/test.txt"));
    const Statement convertableFileObjectWithNieUrl1_converted(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("filex://xyz-123/test.txt"));
    QTest::newRow("convertableFileUrlInObjectWithNieUrl1") << convertableFileObjectWithNieUrl1_original << convertableFileObjectWithNieUrl1_converted;

    const Statement convertableFileObjectWithNieUrl2_original(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("file:///media/XO-Y4"));
    const Statement convertableFileObjectWithNieUrl2_converted(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("filex://xyz-123"));
    QTest::newRow("convertableFileUrlInObjectWithNieUrl2") << convertableFileObjectWithNieUrl2_original << convertableFileObjectWithNieUrl2_converted;

    const Statement convertableFileObjectWithNieUrl3_original(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("file:///media/nfs/test.txt"));
    const Statement convertableFileObjectWithNieUrl3_converted(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("nfs://thehost/solid-path/test.txt"));
    QTest::newRow("convertableFileUrlInObjectWithNieUrl3") << convertableFileObjectWithNieUrl3_original << convertableFileObjectWithNieUrl3_converted;

    const Statement convertableFileObjectWithNieUrl4_original(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("file:///media/nfs"));
    const Statement convertableFileObjectWithNieUrl4_converted(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("nfs://thehost/solid-path"));
    QTest::newRow("convertableFileUrlInObjectWithNieUrl4") << convertableFileObjectWithNieUrl4_original << convertableFileObjectWithNieUrl4_converted;
}


void RemovableMediaModelTest::testConvertFileUrlsInStatement()
{
    QFETCH(Statement, original);
    QFETCH(Statement, converted);

    QCOMPARE(m_rmModel->convertFileUrls(original), converted);
}

void RemovableMediaModelTest::testConvertFileUrlsInQuery_data()
{
    QTest::addColumn<QString>( "original" );
    QTest::addColumn<QString>( "converted" );

    QString query = QString::fromLatin1("select ?r where { ?r ?p <file:///media/foobar/test.txt> . }");
    QTest::newRow("queryWithNonConvertableFileUrl") << query << query;

    QTest::newRow("queryWithConvertableFileUrl1")
            << QString::fromLatin1("select ?r where { ?r ?p <file:///media/XO-Y4/test.txt> . }")
            << QString::fromLatin1("select ?r where { ?r ?p <filex://xyz-123/test.txt> . }");

    QTest::newRow("queryWithConvertableFileUrl2")
            << QString::fromLatin1("select ?r where { ?r ?p <file:///media/nfs/test.txt> . }")
            << QString::fromLatin1("select ?r where { ?r ?p <nfs://thehost/solid-path/test.txt> . }");

    QTest::newRow("queryWithConvertableRegex1")
            << QString::fromLatin1("select ?r where { ?r nie:url ?u . FILTER(REGEX(?u, '^file:///media/XO-Y4/test')) . }")
            << QString::fromLatin1("select ?r where { ?r nie:url ?u . FILTER(REGEX(?u, '^filex://xyz-123/test')) . }");

    QTest::newRow("queryWithConvertableRegex2")
            << QString::fromLatin1("select ?r where { ?r nie:url ?u . FILTER(REGEX(?u, '^file:///media/nfs/')) . }")
            << QString::fromLatin1("select ?r where { ?r nie:url ?u . FILTER(REGEX(?u, '^nfs://thehost/solid-path/')) . }");
}

void RemovableMediaModelTest::testConvertFileUrlsInQuery()
{
    QFETCH(QString, original);
    QFETCH(QString, converted);

    QCOMPARE(m_rmModel->convertFileUrls(original), converted);
}

void RemovableMediaModelTest::testConvertFilxUrl_data()
{
    QTest::addColumn<Node>( "original" );
    QTest::addColumn<Node>( "converted" );

    const Node nothingToConvertFilex(QUrl("filex://abc-789/hello/world"));
    QTest::newRow("nothingToConvertFilex") << nothingToConvertFilex << nothingToConvertFilex;

    const Node convertFilex1(QUrl("filex://xyz-123/hello/world"));
    QTest::newRow("convertFilex1") << convertFilex1 << Node(QUrl("file:///media/XO-Y4/hello/world"));

    const Node convertFilex2(QUrl("filex://xyz-123"));
    QTest::newRow("convertFilex2") << convertFilex2 << Node(QUrl("file:///media/XO-Y4"));

    const Node convertnfs(QUrl("nfs://thehost/solid-path"));
    QTest::newRow("convertnfs") << convertnfs << Node(QUrl("file:///media/nfs"));
}

void RemovableMediaModelTest::testConvertFilxUrl()
{
    QFETCH(Node, original);
    QFETCH(Node, converted);

    QCOMPARE(m_rmModel->convertFilexUrl(original), converted);
}

void RemovableMediaModelTest::testConvertFilxUrls_data()
{
    QTest::addColumn<Statement>( "original" );
    QTest::addColumn<Statement>( "converted" );

    const Statement randomStatement(QUrl("nepomuk:/res/xyz"), QUrl("onto:someProp"), LiteralValue("foobar"));
    QTest::newRow("noFileUrls") << randomStatement << randomStatement;

    const Statement randomFilexSubject(QUrl("filex://123-123/tmp/test"), QUrl("onto:someProp"), LiteralValue("foobar"));
    QTest::newRow("randomFilexUrlInSubject") << randomFilexSubject << randomFilexSubject;

    const Statement convertableFilexSubject(QUrl("filex://xyz-123/test.txt"), QUrl("onto:someProp"), LiteralValue("foobar"));
    QTest::newRow("convertableFilexUrlInSubject") << convertableFilexSubject << convertableFilexSubject;

    const Statement convertableFilexObjectWithoutNieUrl(QUrl("nepomuk:/res/xyz"), QUrl("onto:someProp"), QUrl("filex://xyz-123/test.txt"));
    QTest::newRow("convertableFilexUrlInObjectWithoutNieUrl") << convertableFilexObjectWithoutNieUrl << convertableFilexObjectWithoutNieUrl;

    const Statement convertableFilexObjectWithNieUrl1_original(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("filex://xyz-123/test.txt"));
    const Statement convertableFilexObjectWithNieUrl1_converted(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("file:///media/XO-Y4/test.txt"));
    QTest::newRow("convertableFilexUrlInObjectWithNieUrl1") << convertableFilexObjectWithNieUrl1_original << convertableFilexObjectWithNieUrl1_converted;

    const Statement convertableFilexObjectWithNieUrl2_original(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("filex://xyz-123"));
    const Statement convertableFilexObjectWithNieUrl2_converted(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("file:///media/XO-Y4"));
    QTest::newRow("convertableFilexUrlInObjectWithNieUrl2") << convertableFilexObjectWithNieUrl2_original << convertableFilexObjectWithNieUrl2_converted;

    const Statement convertableFilexObjectWithNieUrl3_original(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("nfs://thehost/solid-path/test.txt"));
    const Statement convertableFilexObjectWithNieUrl3_converted(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("file:///media/nfs/test.txt"));
    QTest::newRow("convertableFileUrlInObjectWithNieUrl3") << convertableFilexObjectWithNieUrl3_original << convertableFilexObjectWithNieUrl3_converted;

    const Statement convertableFilexObjectWithNieUrl4_original(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("nfs://thehost/solid-path"));
    const Statement convertableFilexObjectWithNieUrl4_converted(QUrl("nepomuk:/res/xyz"), NIE::url(), QUrl("file:///media/nfs"));
    QTest::newRow("convertableFileUrlInObjectWithNieUrl4") << convertableFilexObjectWithNieUrl4_original << convertableFilexObjectWithNieUrl4_converted;
}

void RemovableMediaModelTest::testConvertFilxUrls()
{
    QFETCH(Statement, original);
    QFETCH(Statement, converted);

    QCOMPARE(m_rmModel->convertFilexUrls(original), converted);
}

void RemovableMediaModelTest::testConversionWithUnmount()
{
    KUrl fileXUrl("filex://xyz-123/hello/world");
    KUrl fileUrl("file:///media/XO-Y4/hello/world");

    // device mounted
    plugDevice(QLatin1String(s_udiXyz123));

    // conversion should work
    QCOMPARE(m_rmModel->convertFileUrl(fileUrl), Soprano::Node(fileXUrl));
    QCOMPARE(m_rmModel->convertFilexUrl(fileXUrl), Soprano::Node(fileUrl));


    // add some data to query
    m_rmModel->addStatement(QUrl("nepomuk:/res/foobar"), NIE::url(), fileUrl);

    // make sure it is converted when queried
    QCOMPARE(m_rmModel->listStatements(QUrl("nepomuk:/res/foobar"), NIE::url(), Soprano::Node()).allElements().count(), 1);
    QCOMPARE(m_rmModel->listStatements(QUrl("nepomuk:/res/foobar"), NIE::url(), Soprano::Node()).allElements().first().object(),
             Soprano::Node(fileUrl));
    QCOMPARE(m_rmModel->executeQuery(QString::fromLatin1("select ?u where { <nepomuk:/res/foobar> ?p ?u . }"),
                                     Soprano::Query::QueryLanguageSparql).iterateBindings(0).allElements().count(), 1);
    QCOMPARE(m_rmModel->executeQuery(QString::fromLatin1("select ?u where { <nepomuk:/res/foobar> ?p ?u . }"),
                                     Soprano::Query::QueryLanguageSparql).iterateBindings(0).allElements().first(),
             Soprano::Node(fileUrl));


    // unmount device
    unplugDevice(s_udiXyz123);

    // now conversion should do noting
    QCOMPARE(m_rmModel->convertFileUrl(fileUrl), Soprano::Node(fileUrl));
    QCOMPARE(m_rmModel->convertFilexUrl(fileXUrl), Soprano::Node(fileXUrl));

    // make sure nothing is converted anymore
    QCOMPARE(m_rmModel->listStatements(QUrl("nepomuk:/res/foobar"), NIE::url(), Soprano::Node()).allElements().count(), 1);
    QCOMPARE(m_rmModel->listStatements(QUrl("nepomuk:/res/foobar"), NIE::url(), Soprano::Node()).allElements().first().object(),
             Soprano::Node(fileXUrl));
    QCOMPARE(m_rmModel->executeQuery(QString::fromLatin1("select ?u where { <nepomuk:/res/foobar> ?p ?u . }"),
                                     Soprano::Query::QueryLanguageSparql).iterateBindings(0).allElements().count(), 1);
    QCOMPARE(m_rmModel->executeQuery(QString::fromLatin1("select ?u where { <nepomuk:/res/foobar> ?p ?u . }"),
                                     Soprano::Query::QueryLanguageSparql).iterateBindings(0).allElements().first(),
             Soprano::Node(fileXUrl));


    // re-plug device for other tests
    plugDevice(QLatin1String(s_udiXyz123));
}

QTEST_KDEMAIN_CORE(RemovableMediaModelTest)

#include "removablemediamodeltest.moc"
