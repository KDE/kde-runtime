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

#include "activefilequeuetest.h"
#include "../activefilequeue.h"

#include <QtTest>
#include <qtest_kde.h>


namespace {
    void loopWait(int msecs) {
        QEventLoop loop;
        QTimer::singleShot(msecs, &loop, SLOT(quit()));
        loop.exec();
    }
}

ActiveFileQueueTest::ActiveFileQueueTest()
{
    qRegisterMetaType<KUrl>();
}

void ActiveFileQueueTest::testTimeout()
{
    KUrl myUrl("/tmp");

    // enqueue one url and then make sure it is not emitted before the timeout
    ActiveFileQueue queue;
    queue.setTimeout(3);
    queue.enqueueUrl(myUrl);

    QSignalSpy spy( &queue, SIGNAL(urlTimeout(KUrl)) );

    // wait for 1 seconds
    loopWait(1000);

    // the signal should not have been emitted yet
    QVERIFY(spy.isEmpty());

    // wait another 2 seconds
    loopWait(2000);

    // now the signal should have been emitted
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().first().value<KUrl>(), myUrl);
}

void ActiveFileQueueTest::testRequeue()
{
    KUrl myUrl("/tmp");

    // enqueue one url and then make sure it is not emitted before the timeout
    ActiveFileQueue queue;
    queue.setTimeout(3);
    queue.enqueueUrl(myUrl);

    QSignalSpy spy( &queue, SIGNAL(urlTimeout(KUrl)) );

    // wait for 2 seconds
    loopWait(1000);

    // re-queue the url
    queue.enqueueUrl(myUrl);

    // wait another 2 seconds
    loopWait(2000);

    // the signal should not have been emitted yet
    QVERIFY(spy.isEmpty());

    // wait another 2 seconds
    loopWait(2000);

    // now the signal should have been emitted
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().first().value<KUrl>(), myUrl);
}

QTEST_MAIN(ActiveFileQueueTest)

#include "activefilequeuetest.moc"
