/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Serebriyskiy Artem <v.for.vandal@gmail.com>

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

#include "simpleresourceclassestest.h"


#include <QtDebug>
#include <QByteArray>
#include <QDataStream>
#include <QtTest/QTest>

using namespace Nepomuk;

void SimpleResourceSubsystemTest::initTestCase()
{
    resource1.setUri(QUrl("resource1"));
    resource1.addProperty(QUrl("blablabla"),QString("blablabla"));
    resource1.addProperty(QUrl("resprg"),SimpleResource(QUrl("another")));

    resource2.setUri(QUrl("resource2"));
    resource2.addProperty(QUrl("prop2"),QString("text"));
    resource2.addProperty(QUrl("prop3"),QVariant(1));
}


void SimpleResourceSubsystemTest::testSimpleResourceStream()
{

    QByteArray array;
    QDataStream write_stream(&array,QIODevice::WriteOnly);

    write_stream << resource1;

    // Now read
    QDataStream read_stream(array);
    SimpleResource answer;
    read_stream >> answer;

    QCOMPARE(resource1,answer);

}

void SimpleResourceSubsystemTest::testSimpleResourceGraphStream()
{
    SimpleResourceGraph refGraph;
    refGraph.insert(resource1);
    refGraph.insert(resource2);

    QByteArray array;
    QDataStream write_stream(&array,QIODevice::WriteOnly);

    write_stream << refGraph;

    // Now read
    QDataStream read_stream(array);
    SimpleResourceGraph answer;
    read_stream >> answer;

    QCOMPARE(refGraph,answer);

}

void SimpleResourceSubsystemTest::testSimpleResourceGraphAdd()
{
    SimpleResourceGraph refGraph;
    refGraph.insert(resource1);
    refGraph.insert(resource2);
    SimpleResourceGraph graph1;

    graph1 += refGraph;
    QCOMPARE(graph1,refGraph);

    SimpleResourceGraph graph2;
    graph2 = refGraph;
    graph2 += refGraph;
    qDebug() << refGraph;
    qDebug() << graph2;

    QCOMPARE(graph2,refGraph);
}
QTEST_MAIN(SimpleResourceSubsystemTest)
