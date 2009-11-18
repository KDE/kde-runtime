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


    Some parts of this code are based on older drkonqi code (backtracegdb.cpp)
    that was under the following license:

    * Copyright (C) 2000-2003 Hans Petter Bieker <bieker@kde.org>
    * Copyright (C) 2008 Lubos Lunak <l.lunak@kde.org>
    *
    * Redistribution and use in source and binary forms, with or without
    * modification, are permitted provided that the following conditions
    * are met:
    *
    * 1. Redistributions of source code must retain the above copyright
    *    notice, this list of conditions and the following disclaimer.
    * 2. Redistributions in binary form must reproduce the above copyright
    *    notice, this list of conditions and the following disclaimer in the
    *    documentation and/or other materials provided with the distribution.
    *
    * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "backtraceparser.h"

#ifndef BACKTRACEPARSER_TEST
# include "backtracegenerator.h"
# include <KDebug>
#else
# include "tests/backtraceparsertest/backtraceparsertest.h"
# include <QDebug>
# define kDebug qDebug
#endif

#include <QtCore/QRegExp>
#include <QtCore/QSharedData>
#include <QtCore/QStack>
#include <QtCore/QMetaEnum> //used for a kDebug() in BacktraceParserGdb::backtraceUsefulness()
#include <KGlobal>

//BEGIN BacktraceParser

//factory
BacktraceParser *BacktraceParser::newParser(const QString & debuggerName, QObject *parent)
{
    if (debuggerName == "gdb") {
        return new BacktraceParserGdb(parent);
    } else {
        return new BacktraceParserNull(parent);
    }
}

BacktraceParser::BacktraceParser(QObject *parent) : QObject(parent) {}
BacktraceParser::~BacktraceParser() {}

void BacktraceParser::connectToGenerator(BacktraceGenerator *generator)
{
    connect(generator, SIGNAL(starting()), this, SLOT(resetState()));
    connect(generator, SIGNAL(newLine(QString)), this, SLOT(newLine(QString)));
}

//END BacktraceParser

//BEGIN BacktraceLineGdb

class BacktraceLineGdbData : public QSharedData
{
public:
    BacktraceLineGdbData()
            : m_stackFrameNumber(-1), m_functionName("??"),
            m_hasFileInfo(false), m_hasSourceFile(false),
            m_type(0), m_rating(-1) {}

    QString m_line;
    int m_stackFrameNumber;
    QString m_functionName;
    QString m_functionArguments;
    bool m_hasFileInfo;
    bool m_hasSourceFile;
    QString m_file;

    //these are enums defined below, in BacktraceLineGdb.
    int m_type; //LineType
    int m_rating; //LineRating
};

class BacktraceLineGdb
{
public:
    enum LineType {
        Unknown, //unknown type. the default
        EmptyLine, //line is empty
        Crap, //line is gdb's crap (like "(no debugging symbols found)",
              //"[New Thread 0x4275c950 (LWP 11931)]", etc...)
        KCrash, //line is "[KCrash Handler]"
        ThreadIndicator, //line indicates the current thread,
                         //ex. "[Current thread is 0 (process 11313)]"
        ThreadStart, //line indicates the start of a thread's stack.
        SignalHandlerStart, //line indicates the signal handler start
                            //(contains "<signal handler called>")
        StackFrame //line is a normal stack frame
    };

    enum LineRating {
        /* RATING           --          EXAMPLE */
        MissingEverything = 0, // #0 0x0000dead in ?? ()
        MissingFunction = 1, // #0 0x0000dead in ?? () from /usr/lib/libfoobar.so.4
        MissingLibrary = 2, // #0 0x0000dead in foobar()
        MissingSourceFile = 3, // #0 0x0000dead in FooBar::FooBar () from /usr/lib/libfoobar.so.4
        Good = 4, // #0 0x0000dead in FooBar::crash (this=0x0) at /home/user/foobar.cpp:204
        InvalidRating = -1 // (dummy invalid value)
    };

    static const LineRating BestRating = Good;

    BacktraceLineGdb() {} //needed for using BacktraceLineGdb in QStack
    BacktraceLineGdb(const QString & line);

    QString toString() const {
        return d->m_line;
    }
    LineType type() const {
        return (LineType) d->m_type;
    }
    LineRating rating() const {
        Q_ASSERT(d->m_rating >= 0); return (LineRating) d->m_rating;
    }

    int frameNumber() const {
        return d->m_stackFrameNumber;
    }
    QString functionName() const {
        return d->m_functionName;
    }
    QString functionArgs() const {
        return d->m_functionArguments;
    }
    QString fileName() const {
        return d->m_hasSourceFile ? d->m_file : QString();
    }
    QString libraryName() const {
        return d->m_hasSourceFile ? QString() : d->m_file;
    }

    bool operator==(const BacktraceLineGdb & other) const {
        return d == other.d;
    }

private:
    void parse();
    void rate();
    QExplicitlySharedDataPointer<BacktraceLineGdbData> d;
};

BacktraceLineGdb::BacktraceLineGdb(const QString & lineStr)
        : d(new BacktraceLineGdbData)
{
    d->m_line = lineStr;
    parse();
    if (d->m_type == StackFrame) {
        rate();
    }
}

void BacktraceLineGdb::parse()
{
    QRegExp regExp;

    if (d->m_line == "\n") {
        d->m_type = EmptyLine;
        return;
    } else if (d->m_line == "[KCrash Handler]\n") {
        d->m_type = KCrash;
        return;
    } else if (d->m_line.contains("<signal handler called>")) {
        d->m_type = SignalHandlerStart;
        return;
    }

    regExp.setPattern("^#([0-9]+)" //matches the stack frame number, ex. "#0"
                      "[\\s]+(0x[0-9a-f]+[\\s]+in[\\s]+)?" // matches " 0x0000dead in " (optionally)
                      "([^\\(]+)" //matches the function name (anything except left parenthesis,
                      // which is the start of the arguments section)
                      "(\\(.*\\))?" //matches the function arguments
                                    //(when the app doesn't have debugging symbols)
                      "[\\s]+\\(" //matches " ("
                      "(.*)" //matches the arguments of the function with their values
                             //(when the app has debugging symbols)
                      "\\)([\\s]+" //matches ") "
                      "(from|at)[\\s]+" //matches "from " or "at "
                      "(.+)" //matches the filename (source file or shared library file)
                      ")?\n$"); //matches trailing newline.
                    //the )? at the end closes the parenthesis before [\\s]+(from|at) and
                    //notes that the whole expression from there is optional.

    if (regExp.exactMatch(d->m_line)) {
        d->m_type = StackFrame;
        d->m_stackFrameNumber = regExp.cap(1).toInt();
        d->m_functionName = regExp.cap(3);
        //remove \n because arguments may be split in two lines
        d->m_functionArguments = regExp.cap(5).remove('\n');
        d->m_hasFileInfo = !regExp.cap(6).isEmpty();
        d->m_hasSourceFile = d->m_hasFileInfo ? (regExp.cap(7) == "at") : false;
        d->m_file = d->m_hasFileInfo ? regExp.cap(8) : QString();

        kDebug() << d->m_stackFrameNumber << d->m_functionName << d->m_functionArguments
                 << d->m_hasFileInfo << d->m_hasSourceFile << d->m_file;
        return;
    }

    regExp.setPattern(".*\\(no debugging symbols found\\).*|"
                      ".*\\[Thread debugging using libthread_db enabled\\].*|"
                      ".*\\[New .*|"
                      "0x[0-9a-f]+.*|"
                      "Current language:.*");
    if (regExp.exactMatch(d->m_line)) {
        kDebug() << "garbage detected:" << d->m_line;
        d->m_type = Crap;
        return;
    }

    regExp.setPattern("Thread [0-9]+\\s+\\(Thread [0-9a-fx]+\\s+\\(.*\\)\\):\n");
    if (regExp.exactMatch(d->m_line)) {
        kDebug() << "thread start detected:" << d->m_line;
        d->m_type = ThreadStart;
        return;
    }

    regExp.setPattern("\\[Current thread is [0-9]+ \\(.*\\)\\]\n");
    if (regExp.exactMatch(d->m_line)) {
        kDebug() << "thread indicator detected:" << d->m_line;
        d->m_type = ThreadIndicator;
        return;
    }

    kDebug() << "line" << d->m_line << "did not match";
}

void BacktraceLineGdb::rate()
{
    LineRating r;

    //for explanations, see the LineRating enum definition above
    if (!fileName().isEmpty()) {
        r = Good;
    } else if (!libraryName().isEmpty()) {
        if (functionName() == "??") {
            r = MissingFunction;
        } else {
            r = MissingSourceFile;
        }
    } else {
        if (functionName() == "??") {
            r = MissingEverything;
        } else {
            r = MissingLibrary;
        }
    }

    d->m_rating = r;
}

//END BacktraceLineGdb

//BEGIN BacktraceParserGdb

struct BacktraceParserGdb::Private {
    Private() : m_possibleKCrashStart(0), m_threadsCount(0),
            m_isBelowSignalHandler(false), m_frameZeroAppeared(false),
            m_cachedUsefulness(BacktraceParser::InvalidUsefulness) {}

    QString m_lineInputBuffer;
    QList<BacktraceLineGdb> m_linesList;
    QList<BacktraceLineGdb> m_linesToRate;
    QStringList m_firstUsefulFunctions;
    int m_possibleKCrashStart;
    int m_threadsCount;
    bool m_isBelowSignalHandler;
    bool m_frameZeroAppeared;

    mutable BacktraceParser::Usefulness m_cachedUsefulness;
};

BacktraceParserGdb::BacktraceParserGdb(QObject *parent)
        : BacktraceParser(parent), d(NULL)
{
}

BacktraceParserGdb::~BacktraceParserGdb()
{
    delete d;
}

void BacktraceParserGdb::resetState()
{
    //reset the state of the parser by getting a new instance of Private
    delete d;
    d = new Private;
}

void BacktraceParserGdb::newLine(const QString & lineStr)
{
    //when the line is too long, gdb splits it into two lines.
    //This breaks parsing and results in two Unknown lines instead of a StackFrame one.
    //Here we workaround this by joining the two lines when such a scenario is detected.
    if (d->m_lineInputBuffer.isEmpty()) {
        d->m_lineInputBuffer = lineStr;
    } else if (lineStr.startsWith(QLatin1Char(' ')) || lineStr.startsWith(QLatin1Char('\t'))) {
        //gdb always adds some whitespace at the beginning of the second line
        d->m_lineInputBuffer.append(lineStr);
    } else {
        parseLine(d->m_lineInputBuffer);
        d->m_lineInputBuffer = lineStr;
    }
}

void BacktraceParserGdb::parseLine(const QString & lineStr)
{
    BacktraceLineGdb line(lineStr);
    switch (line.type()) {
    case BacktraceLineGdb::Crap:
        break; //we don't want crap in the backtrace ;)
    case BacktraceLineGdb::ThreadStart:
        d->m_linesList.append(line);
        d->m_possibleKCrashStart = d->m_linesList.size();
        d->m_threadsCount++;
        //reset the state of the flags that need to be per-thread
        d->m_isBelowSignalHandler = false;
        d->m_frameZeroAppeared = false; // gdb bug workaround flag, see below
        break;
    case BacktraceLineGdb::SignalHandlerStart:
        //replace the stack frames of KCrash with a nice message
        d->m_linesList.erase(d->m_linesList.begin() + d->m_possibleKCrashStart, d->m_linesList.end());
        d->m_linesList.insert(d->m_possibleKCrashStart, BacktraceLineGdb("[KCrash Handler]\n"));
        d->m_isBelowSignalHandler = true; //next line is the first below the signal handler
        break;
    case BacktraceLineGdb::StackFrame:
        // gdb workaround - (v6.8 at least) - 'thread apply all bt' writes
        // the #0 stack frame again at the end.
        // Here we ignore this frame by using a flag that tells us whether
        // this is the first or the second time that the #0 frame appears in this thread.
        // The flag is cleared on each thread start.
        if (line.frameNumber() == 0) {
            if (d->m_frameZeroAppeared) {
                break; //break from the switch so that the frame is not added to the list.
            } else {
                d->m_frameZeroAppeared = true;
            }
        }

        //rate the stack frame if we are below the signal handler
        if (d->m_isBelowSignalHandler) {
            d->m_linesToRate.append(line);
        }

        //fall through and append the line to the list
    default:
        d->m_linesList.append(line);
        break;
    }
}

QString BacktraceParserGdb::parsedBacktrace() const
{
    QString result;

    //if there is no d, the debugger has not run, so we return an empty string.
    if (!d) {
        return result;
    }

    QList<BacktraceLineGdb>::const_iterator i;
    for (i = d->m_linesList.constBegin(); i != d->m_linesList.constEnd(); ++i) {
        //if there is only one thread, we can omit the thread indicator,
        //the thread header and all the empty lines.
        if (d->m_threadsCount == 1 && ((*i).type() == BacktraceLineGdb::ThreadIndicator
                                       || (*i).type() == BacktraceLineGdb::ThreadStart
                                       || (*i).type() == BacktraceLineGdb::EmptyLine))
            continue;
        result += (*i).toString();
    }
    return result;
}


/* This function returns true if the given stack frame line is the base of the backtrace
   and thus the parser should not rate any frames below that one. */
static bool lineIsStackBase(const BacktraceLineGdb & line)
{
    //optimization. if there is no function name, do not bother to check it
    if ( line.rating() == BacktraceLineGdb::MissingEverything
        || line.rating() == BacktraceLineGdb::MissingFunction )
        return false;

    //this is the base frame for all threads except the main thread
    //FIXME that probably works only on linux
    if ( line.functionName() == "start_thread" )
        return true;

    QRegExp regExp;
    regExp.setPattern("(kde)?main"); //main() or kdemain() is the base for the main thread
    if ( regExp.exactMatch(line.functionName()) )
        return true;

    //HACK for better rating. we ignore all stack frames below any function that matches
    //the following regular expression. The functions that match this expression are usually
    //"QApplicationPrivate::notify_helper", "QApplication::notify" and similar, which
    //are used to send any kind of event to the Qt application. All stack frames below this,
    //with or without debug symbols, are useless to KDE developers, so we ignore them.
    regExp.setPattern("(Q|K)(Core)?Application(Private)?::notify.*");
    if ( regExp.exactMatch(line.functionName()) )
        return true;

    return false;
}

/* This function returns true if the given stack frame line is the top of the bactrace
   and thus the parser should not rate any frames above that one. This is used to avoid
   rating the stack frames of abort(), assert(), Q_ASSERT() and qFatal() */
static bool lineIsStackTop(const BacktraceLineGdb & line)
{
    //optimization. if there is no function name, do not bother to check it
    if ( line.rating() == BacktraceLineGdb::MissingEverything
        || line.rating() == BacktraceLineGdb::MissingFunction )
        return false;

    if ( line.functionName().startsWith(QLatin1String("qt_assert")) //qt_assert and qt_assert_x
        || line.functionName() == "qFatal"
        || line.functionName() == "abort"
        || line.functionName() == "*__GI_abort"
        || line.functionName() == "*__GI___assert_fail" )
        return true;

    return false;
}

/* This function returns true if the given stack frame line should be ignored from rating
   for some reason. Currently it ignores all libc/libstdc++/libpthread functions. */
static bool lineShouldBeIgnored(const BacktraceLineGdb & line)
{
    if ( line.libraryName().contains("libc.so")
        || line.libraryName().contains("libstdc++.so")
        || line.functionName().startsWith(QLatin1String("*__GI_")) //glibc2.9 uses *__GI_ as prefix
        || line.libraryName().contains("libpthread.so") )
        return true;

    return false;
}

BacktraceParser::Usefulness BacktraceParserGdb::backtraceUsefulness() const
{
    //if there is no d, the debugger has not run,
    //so we can say that the (inexistent) backtrace is Useless.
    if (!d || d->m_linesToRate.isEmpty()) {
        return Useless;
    }

    //cache the usefulness value because this function will probably be called many times
    if (d->m_cachedUsefulness != InvalidUsefulness) {
        return d->m_cachedUsefulness;
    }

    uint rating = 0, bestPossibleRating = 0, counter = 0;
    bool haveSeenStackBase = false;
    QStack<BacktraceLineGdb> usefulFunctionsStack;

    QListIterator<BacktraceLineGdb> i(d->m_linesToRate);
    i.toBack(); //start from the end of the list

    while( i.hasPrevious() ) {
        const BacktraceLineGdb & line = i.previous();

        if ( !i.hasPrevious() && line.rating() == BacktraceLineGdb::MissingEverything ) {
            //Under some circumstances, the very first stack frame is invalid (ex, calling a function
            //at an invalid address could result in a stack frame like "0x00000000 in ?? ()"),
            //which however does not necessarily mean that the backtrace has a missing symbol on
            //the first line. Here we make sure to ignore this line from rating. (bug 190882)
            break; //there are no more items anyway, just break the loop
        }

        if ( lineIsStackBase(line) ) {
            rating = bestPossibleRating = counter = 0; //restart rating ignoring any previous frames
            haveSeenStackBase = true;
        } else if ( lineIsStackTop(line) ) {
            break; //we have reached the top, no need to inspect any more frames
        }

        if ( lineShouldBeIgnored(line) ) {
            continue;
        }

        if ( line.rating() != BacktraceLineGdb::MissingEverything
            && line.rating() != BacktraceLineGdb::MissingFunction ) {
            usefulFunctionsStack.push(line);
        }

        uint multiplier = ++counter; //give weight to the first lines
        rating += static_cast<uint>(line.rating()) * multiplier;
        bestPossibleRating += static_cast<uint>(BacktraceLineGdb::BestRating) * multiplier;

        kDebug() << line.rating() << line.toString();
    }

    //Save the first 3 useful functions that were encountered in the backtrace.
    //This is used later to search for duplicate reports.
    for (int i = 0; i<3 && !usefulFunctionsStack.isEmpty(); i++) {
        d->m_firstUsefulFunctions.append(usefulFunctionsStack.pop().functionName());
    }

    //calculate rating
    Usefulness usefulness = Useless;
    if (rating >= (bestPossibleRating*0.90)) {
        usefulness = ReallyUseful;
    } else if (rating >= (bestPossibleRating*0.70)) {
        usefulness = MayBeUseful;
    } else if (rating >= (bestPossibleRating*0.40)) {
        usefulness = ProbablyUseless;
    }

    //if there is no stack base, the executable is probably stripped,
    //so we need to be more strict with rating
    if ( !haveSeenStackBase ) {
        //less than 4 stack frames is useless
        if ( counter < 4 ) {
            usefulness = Useless;
        //more than 4 stack frames might have some value, so let's not be so strict, just lower the rating
        } else if ( usefulness > Useless ) {
            usefulness = (Usefulness) (usefulness - 1);
        }
    }

    kDebug() << "Rating:" << rating << "out of" << bestPossibleRating << "Usefulness:"
             << staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("Usefulness")).valueToKey(usefulness);
    kDebug() << "90%:" << (bestPossibleRating*0.90) << "70%:" << (bestPossibleRating*0.70)
             << "40%:" << (bestPossibleRating*0.40);
    kDebug() << "Have seen stack base:" << haveSeenStackBase << "Lines counted:" << counter;

    d->m_cachedUsefulness = usefulness;
    return usefulness;
}

QStringList BacktraceParserGdb::firstValidFunctions() const
{
    //if there is no cached usefulness, the usefulness calculation function has not run yet.
    //in this case, we need to run it because the useful functions are also saved from there.
    if (d && d->m_cachedUsefulness == InvalidUsefulness) {
        kDebug() << "backtrace usefulness has not been calculated yet. calculating...";
        backtraceUsefulness();
    }

    //if there is no d, the debugger has not run, so we have no functions to return.
    return d ? d->m_firstUsefulFunctions : QStringList();
}

//END BacktraceParserGdb

//BEGIN BacktraceParserNull

BacktraceParserNull::BacktraceParserNull(QObject *parent) : BacktraceParser(parent) {}
BacktraceParserNull::~BacktraceParserNull() {}

QString BacktraceParserNull::parsedBacktrace() const
{
    return m_lines.join("");
}

BacktraceParser::Usefulness BacktraceParserNull::backtraceUsefulness() const
{
    return MayBeUseful;
}

QStringList BacktraceParserNull::firstValidFunctions() const
{
    return QStringList();
}

void BacktraceParserNull::resetState()
{
    m_lines.clear();
}

void BacktraceParserNull::newLine(const QString & lineStr)
{
    m_lines.append(lineStr);
}

//END BacktraceParserNull

#include "backtraceparser.moc"
