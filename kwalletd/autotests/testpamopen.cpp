/*************************************************************************************
 *  Copyright (C) 2013 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/

#include "kwalletexecuter.h"
#include "qtest_kwallet.h"
#include <backendpersisthandler.h>

#include <kstandarddirs.h>
#include <QtTest/QtTest>
#include <QtCore/QObject>
#include <QtCore/QDebug>
#include <QtCore/QProcess>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusServiceWatcher>

class testPamOpen : public KWalletExecuter
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testOpen();
    void testRead();
    void testWrite();
};

void testPamOpen::initTestCase()
{
    QString from(TEST_DATA);
    from.append("kdewallet.kwl");
    QString to = KStandardDirs::locateLocal("data", "kwallet/kdewallet.kwl");
    QFile::remove(to);
    QFile::copy(from, to);
    execute();
}

void testPamOpen::testOpen()
{
    pamOpen();
}

void testPamOpen::testRead()
{
    pamRead(QLatin1String("bar"));
}

void testPamOpen::testWrite()
{
    const QString value("bar2");
    pamWrite(value);
    pamRead(value);
}

QTEST_KDEMAIN_CORE_WITH_DBUS_DAEMON(testPamOpen)
#include "testpamopen.moc"
