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
#include "backtraceparsertest.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>
#include <QMetaEnum>
#include <QDebug>
#include <QSharedPointer>

#define DATA_DIR "backtraceparsertest_data"
#define TEST_PREFIX "test_"
#define MAP_FILE "usefulness_map"

BacktraceParserTest::BacktraceParserTest(QObject *parent)
    : QObject(parent), m_generator(new FakeBacktraceGenerator(this))
{
}

void BacktraceParserTest::readMap(QHash<QByteArray, BacktraceParser::Usefulness> & map)
{
    QMetaEnum metaUsefulness = BacktraceParser::staticMetaObject.enumerator(
                                    BacktraceParser::staticMetaObject.indexOfEnumerator("Usefulness"));

    QFile mapFile(DATA_DIR "/" MAP_FILE);
    if ( !mapFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        qFatal("Could not open \"" DATA_DIR "/" MAP_FILE "\".");
    }

    while ( !mapFile.atEnd() ) {
        QByteArray line = mapFile.readLine();
        if ( line.isEmpty() ) continue;
        QList<QByteArray> tokens = line.split(':');
        Q_ASSERT(tokens.size() == 2);
        map.insert( tokens[0].trimmed(), (BacktraceParser::Usefulness) metaUsefulness.keyToValue(tokens[1].trimmed().constData()) );
    }
}

void BacktraceParserTest::btParserTest_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<BacktraceParser::Usefulness>("result");

    if ( !QFileInfo(DATA_DIR).isDir() ) {
        qFatal("Could not find the \"" DATA_DIR "\" directory in the current directory.");
    }

    QHash<QByteArray, BacktraceParser::Usefulness> map;
    readMap(map);

    QDirIterator it(DATA_DIR, QStringList() << TEST_PREFIX"*", QDir::Files|QDir::Readable);
    while (it.hasNext()) {
        it.next();
        QTest::newRow(it.fileName().toLocal8Bit().constData())
            << it.filePath()
            << map.value(it.fileName().toLocal8Bit(), BacktraceParser::InvalidUsefulness);
    }
}

void BacktraceParserTest::btParserTest()
{
    QFETCH(QString, filename);
    QFETCH(BacktraceParser::Usefulness, result);
    QVERIFY2(result != BacktraceParser::InvalidUsefulness, "Invalid usefulness. There is an error in the " MAP_FILE " file");

    QMetaEnum metaUsefulness = BacktraceParser::staticMetaObject.enumerator(
                                    BacktraceParser::staticMetaObject.indexOfEnumerator("Usefulness"));

    QSharedPointer<BacktraceParser> parser(BacktraceParser::newParser("gdb"));
    parser->connectToGenerator(m_generator);
    m_generator->sendData(filename);

    QTextStream(stdout) << "\n(" << QFileInfo(filename).fileName() << "): Received: "
                        << metaUsefulness.valueToKey(parser->backtraceUsefulness())
                        << " Expected: " << metaUsefulness.valueToKey(result) << endl;

    QEXPECT_FAIL("test_e", "Working on it", Continue);
    QCOMPARE(parser->backtraceUsefulness(), result);
}

void BacktraceParserTest::btParserBenchmark_data()
{
    btParserTest_data(); //use the same data for the benchmark
}

void BacktraceParserTest::btParserBenchmark()
{
    QFETCH(QString, filename);
    QFETCH(BacktraceParser::Usefulness, result);
    QVERIFY2(result != BacktraceParser::InvalidUsefulness, "Invalid usefulness. There is an error in the " MAP_FILE " file");

    QSharedPointer<BacktraceParser> parser(BacktraceParser::newParser("gdb"));
    parser->connectToGenerator(m_generator);

    QBENCHMARK {
        m_generator->sendData(filename);
    }
}

QTEST_MAIN(BacktraceParserTest)
#include "backtraceparsertest.moc"
