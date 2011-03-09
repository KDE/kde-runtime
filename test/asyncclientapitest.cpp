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
#include "../lib/datamanagement.h"
#include "nepomuk_dms_test_config.h"

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
#include <Nepomuk/Variant>
#include <Nepomuk/ResourceManager>

using namespace Soprano;
using namespace Soprano::Vocabulary;
using namespace Nepomuk;
using namespace Nepomuk::Vocabulary;


void AsyncClientApiTest::initTestCase()
{
    kDebug() << "Starting fake DMS:" << FAKEDMS_BIN;

    // setup the service watcher so we know when the fake DMS is up
    QDBusServiceWatcher watcher(QLatin1String("org.kde.nepomuk.services.DataManagement"),
                                QDBusConnection::sessionBus(),
                                QDBusServiceWatcher::WatchForRegistration);

    // start the fake DMS
    m_fakeDms = new QProcess();
    m_fakeDms->setProcessChannelMode(QProcess::ForwardedChannels);
    m_fakeDms->start(QLatin1String(FAKEDMS_BIN));

    // wait for it to come up
    QTest::kWaitForSignal(&watcher, SIGNAL(serviceRegistered(QString)));

    // get us access to the fake DMS's model
    m_model = new Soprano::Client::DBusModel(QLatin1String("org.kde.nepomuk.services.DataManagement"), QLatin1String("/model"));
}

void AsyncClientApiTest::cleanupTestCase()
{
    kDebug() << "Shutting down fake DMS...";
    QDBusInterface(QLatin1String("org.kde.nepomuk.services.DataManagement"),
                   QLatin1String("/MainApplication"),
                   QLatin1String("org.kde.KApplication"),
                   QDBusConnection::sessionBus()).call(QLatin1String("quit"));
    m_fakeDms->waitForFinished();
    delete m_fakeDms;
    delete m_model;
}

void AsyncClientApiTest::init()
{
}


void AsyncClientApiTest::testAddProperty()
{
    KJob* job = Nepomuk::addProperty(QList<QUrl>() << QUrl("res:/A"), QUrl("prop:/int"), QVariantList() << 42);
    QTest::kWaitForSignal(job, SIGNAL(result(KJob*)), 5000);
    QVERIFY(!job->error());

    QVERIFY(m_model->containsAnyStatement(QUrl("res:/A"), QUrl("prop:/int"), LiteralValue(42)));
}

void AsyncClientApiTest::testRemoveProperties()
{
    Soprano::NRLModel nrlModel(m_model);

    QUrl mg1;
    const QUrl g1 = nrlModel.createGraph(NRL::InstanceBase(), &mg1);

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

QTEST_KDEMAIN_CORE(AsyncClientApiTest)

#include "asyncclientapitest.moc"
