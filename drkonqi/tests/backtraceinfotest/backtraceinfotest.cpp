/*
    Copyright (C) 2009  George Kiagiadakis <gkiagia@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "backtraceinfotest.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>
#include <QMetaEnum>
#include <QDebug>

#define DATA_DIR "backtraceinfotest_data"
#define TEST_PREFIX "test_"
#define MAP_FILE "usefulness_map"

void BacktraceInfoTest::readMap(QHash<QByteArray, BacktraceInfoTest::Usefulness> & map)
{
    QMetaEnum metaUsefulness = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("Usefulness"));

    QFile mapFile(DATA_DIR "/" MAP_FILE);
    if ( !mapFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        qFatal("Could not open \"" DATA_DIR "/" MAP_FILE "\".");
    }

    while ( !mapFile.atEnd() ) {
        QByteArray line = mapFile.readLine();
        if ( line.isEmpty() ) continue;
        QList<QByteArray> tokens = line.split(':');
        Q_ASSERT(tokens.size() == 2);
        map.insert( tokens[0].trimmed(), (Usefulness) metaUsefulness.keyToValue(tokens[1].trimmed().constData()) );
    }
}

void BacktraceInfoTest::btInfoTest_data()
{
    QTest::addColumn<QByteArray>("backtrace");
    QTest::addColumn<Usefulness>("result");

    if ( !QFileInfo(DATA_DIR).isDir() ) {
        qFatal("Could not find the \"" DATA_DIR "\" directory in the current directory.");
    }

    QHash<QByteArray, Usefulness> map;
    readMap(map);

    QDirIterator it(DATA_DIR, QStringList() << TEST_PREFIX"*", QDir::Files|QDir::Readable);
    while (it.hasNext()) {
        QFile file(it.next());
        file.open(QIODevice::ReadOnly);
        QByteArray fileName = it.fileName().toAscii();

        QTest::newRow(fileName.constData())
            << file.readAll()
            << map.value(fileName, InvalidUsefulnessValue);
    }
}

void BacktraceInfoTest::btInfoTest()
{
    QFETCH(QByteArray, backtrace);
    QFETCH(Usefulness, result);
    QVERIFY2(result != InvalidUsefulnessValue, "Invalid usefulness. There is an error in the " MAP_FILE " file");
    //qDebug() << runBacktraceInfo(backtrace); 
    //qDebug() << result;
    QCOMPARE(runBacktraceInfo(backtrace), result);
}

BacktraceInfoTest::Usefulness BacktraceInfoTest::runBacktraceInfo(const QByteArray & data)
{
    BacktraceInfo info;
    info.setBacktraceData(data);
    info.parse();
    return (Usefulness) info.getUsefulness();
}

void BacktraceInfoTest::btInfoBenchmark_data()
{
    btInfoTest_data(); //use the same data for the benchmark
}

void BacktraceInfoTest::btInfoBenchmark()
{
    QFETCH(QByteArray, backtrace);
    QFETCH(Usefulness, result);
    QVERIFY2(result != InvalidUsefulnessValue, "Invalid usefulness. There is an error in the " MAP_FILE " file");

    BacktraceInfo info;
    info.setBacktraceData(backtrace);
    QBENCHMARK {
        info.parse();
    }
}

QTEST_MAIN(BacktraceInfoTest);
#include "backtraceinfotest.moc"
