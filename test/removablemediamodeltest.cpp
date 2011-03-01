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
}

void RemovableMediaModelTest::initTestCase()
{
    // make sure Solid uses the fake manager
    setenv("SOLID_FAKEHW", FAKE_COMPUTER_XML, 1);

    // we simply need some memory model for now - nothing fancy
    m_model = Soprano::createModel();
    m_model->setParent(this);
    m_rmModel = new RemovableMediaModel(m_model, this);

    // list the removable devices we have
    QList<Solid::Device> devices
        = Solid::Device::listFromQuery( QLatin1String( "StorageVolume.usage=='FileSystem'" ) );
    foreach( const Solid::Device& dev, devices ) {
        if ( dev.is<Solid::StorageVolume>() &&
             dev.is<Solid::StorageAccess>() &&
             dev.parent().is<Solid::StorageDrive>() &&
             ( dev.parent().as<Solid::StorageDrive>()->isRemovable() ||
               dev.parent().as<Solid::StorageDrive>()->isHotpluggable() ) ) {
            const Solid::StorageVolume* volume = dev.as<Solid::StorageVolume>();
            if ( !volume->isIgnored() && volume->usage() == Solid::StorageVolume::FileSystem ) {
                m_removableDevices << dev.udi();
            }
        }
    }
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
}


void RemovableMediaModelTest::testConvertFileUrlsInStatement()
{
    QFETCH(Statement, original);
    QFETCH(Statement, converted);

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
}

void RemovableMediaModelTest::testConvertFilxUrls()
{
    QFETCH(Statement, original);
    QFETCH(Statement, converted);

    QCOMPARE(m_rmModel->convertFilexUrls(original), converted);
}

QTEST_KDEMAIN_CORE(RemovableMediaModelTest)

#include "removablemediamodeltest.moc"
