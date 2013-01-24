/*
    Copyright (C) 2010  George Kiagiadakis <kiagiadakis.george@gmail.com>

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
#include "fakebacktracegenerator.h"
#include "../../parser/backtraceparser.h"
#include <QtCore/QFile>
#include <QtCore/QSharedPointer>
#include <QtCore/QTextStream>
#include <QtCore/QMetaEnum>
#include <QtCore/QCoreApplication>
#include <KAboutData>
#include <KCmdLineOptions>
#include <KLocalizedString>

int main(int argc, char **argv)
{
    KAboutData aboutData("backtraceparsertest_manual", 0,
                         ki18n("backtraceparsertest_manual"), "1.0");
    KCmdLineArgs::init(argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add("debugger <name>", ki18n("The debugger name passed to the parser factory"), "gdb");
    options.add("+file", ki18n("A file containing the backtrace."));
    KCmdLineArgs::addCmdLineOptions(options);

    QCoreApplication app(KCmdLineArgs::qtArgc(), KCmdLineArgs::qtArgv());
    QCoreApplication::setApplicationName(QLatin1String("backtraceparsertest_manual"));

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    QString debugger = args->getOption("debugger");
    QString file = args->arg(0);

    if (!QFile::exists(file)) {
        QTextStream(stderr) << "The specified file does not exist" << endl;
        return 1;
    }

    FakeBacktraceGenerator generator;
    QSharedPointer<BacktraceParser> parser(BacktraceParser::newParser(debugger));
    parser->connectToGenerator(&generator);
    generator.sendData(file);

    QMetaEnum metaUsefulness = BacktraceParser::staticMetaObject.enumerator(
                                BacktraceParser::staticMetaObject.indexOfEnumerator("Usefulness"));
    QTextStream(stdout) << "Usefulness: " << metaUsefulness.valueToKey(parser->backtraceUsefulness()) << endl;
    QTextStream(stdout) << "First valid functions: " << parser->firstValidFunctions().join(" ") << endl;
    QTextStream(stdout) << "Simplified backtrace:\n" << parser->simplifiedBacktrace() << endl;
    QStringList l = static_cast<QStringList>(parser->librariesWithMissingDebugSymbols().toList());
    QTextStream(stdout) << "Missing dbgsym libs: " << l.join(" ") << endl;

    return 0;
}
