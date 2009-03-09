/*
    Copyright (C) 2009  George Kiagiadakis <gkiagia@users.sourceforge.net>
    Copyright (C) 2009  Dario Andres Rodriguez <andresbajotierra@gmail.com>

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
# include "tests/backtraceinfotest/backtraceinfotest.h"
# include <QDebug>
# define kDebug qDebug
#endif

#include <QtCore/QRegExp>
#include <QtCore/QSharedData>
#include <QtCore/QSet>
#include <KGlobal>

//BEGIN BacktraceParser

//factory
BacktraceParser *BacktraceParser::newParser(const QString & debuggerName, QObject *parent)
{
    if ( debuggerName == "gdb" )
        return new BacktraceParserGdb(parent);
    else
        return new BacktraceParserNull(parent);
}

BacktraceParser::BacktraceParser(QObject *parent) : QObject(parent) {}
BacktraceParser::~BacktraceParser() {}

void BacktraceParser::connectToGenerator(BacktraceGenerator *generator)
{
    connect(generator, SIGNAL(starting()), this, SLOT(resetState()) );
    connect(generator, SIGNAL(newLine(QString)), this, SLOT(parseLine(QString)) );
}

//END BacktraceParser

//BEGIN BacktraceLineGdb

class BacktraceLineGdbData : public QSharedData
{
public:
    BacktraceLineGdbData()
        : m_parsed(false), m_stackFrameNumber(-1), m_functionName("??"),
          m_hasFileInfo(false), m_hasSourceFile(false), m_type(0), m_rating(-1) {}

    QString m_line;
    bool m_parsed;
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
        Crap, //line is gdb's crap (like "(no debugging symbols found)", "[New Thread 0x4275c950 (LWP 11931)]", etc...
        KCrash, //line is "[KCrash Handler]"
        ThreadIndicator, //line indicates the current thread, ex. "[Current thread is 0 (process 11313)]"
        ThreadStart, //line indicates the start of a thread's stack.
        SignalHandlerStart, //line indicates the signal handler start (contains "<signal handler called>")
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

    BacktraceLineGdb(const QString & line);

    QString toString() const { return d->m_line; }
    LineType type() const { return (LineType) d->m_type; }
    LineRating rating() const { Q_ASSERT(d->m_rating >= 0); return (LineRating) d->m_rating; }

    int frameNumber() const { return d->m_stackFrameNumber; }
    QString functionName() const { return d->m_functionName; }
    QString functionArgs() const { return d->m_functionArguments; }
    QString fileName() const { return d->m_hasSourceFile ? d->m_file : QString(); }
    QString libraryName() const { return d->m_hasSourceFile ? QString() : d->m_file; }

    bool operator==(const BacktraceLineGdb & other) const { return d == other.d; }

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
    if ( d->m_type == StackFrame )
        rate();
}

void BacktraceLineGdb::parse()
{
    QRegExp regExp;

    Q_ASSERT(!d->m_parsed);
    d->m_parsed = true;

    if ( d->m_line == "\n" ) {
        d->m_type = EmptyLine;
        return;
    } else if ( d->m_line == "[KCrash Handler]\n") {
        d->m_type = KCrash;
        return;
    } else if ( d->m_line.contains("<signal handler called>") ) {
        d->m_type = SignalHandlerStart;
        return;
    }

    regExp.setPattern(".*\\(no debugging symbols found\\).*|"
                      ".*\\[Thread debugging using libthread_db enabled\\].*|"
                      ".*\\[New Thread 0x[0-9a-f]+\\s+\\(.*\\)\\].*|"
                // remove the line that shows where the process is at the moment '0xffffe430 in __kernel_vsyscall ()',
                // gdb prints that automatically, but it will be later visible again in the backtrace
                      "0x[0-9a-f]+\\s+in .*");
    if ( regExp.exactMatch(d->m_line) ) {
        kDebug() << "crap detected:" << d->m_line;
        d->m_type = Crap;
        return;
    }

    regExp.setPattern( "Thread [0-9]+\\s+\\(Thread 0x[0-9a-f]+\\s+\\(.*\\)\\):\n" );
    if ( regExp.exactMatch(d->m_line) ) {
        kDebug() << "thread start detected:" << d->m_line;
        d->m_type = ThreadStart;
        return;
    }

    regExp.setPattern( "\\[Current thread is [0-9]+ \\(.*\\)\\]\n" );
    if ( regExp.exactMatch(d->m_line) ) {
        kDebug() << "thread indicator detected:" << d->m_line;
        d->m_type = ThreadIndicator;
        return;
    }

    regExp.setPattern("^#([0-9]+)" //matches the stack frame number, ex. "#0"
                     "[\\s]+(0x[0-9a-fA-F]+[\\s]+in[\\s]+)?" // matches " 0x0000dead in " (optionally)
                     "([\\w\\W]+)" //matches the function name
                     "[\\s]+\\(" //matches " ("
                     "(.*)" //matches the arguments of the function
                     "\\)([\\s]+" //matches ") "
                     "(from|at)[\\s]+" //matches "from " or "at "
                     "([\\w\\W]+)" //matches the filename (source file or shared library file)
                     ")?\n$"); //matches trailing newline.
                                //the )? at the end closes the parenthesis before [\\s]+(from|at) and
                                //notes that the whole expression from there is optional.
    if ( regExp.exactMatch(d->m_line) ) {
        d->m_type = StackFrame;
        d->m_stackFrameNumber = regExp.cap(1).toInt();
        d->m_functionName = regExp.cap(3);
        d->m_functionArguments = regExp.cap(4).remove('\n'); //remove \n because arguments may be split in two lines
        d->m_hasFileInfo = (regExp.pos(5) != -1);
        d->m_hasSourceFile = d->m_hasFileInfo ? (regExp.cap(6) == "at") : false;
        d->m_file = d->m_hasFileInfo ? regExp.cap(7) : QString();

        kDebug() << d->m_stackFrameNumber << d->m_functionName << d->m_functionArguments
                 << d->m_hasFileInfo << d->m_hasSourceFile << d->m_file;
        return;
    }

    kDebug() << "line" << d->m_line << "did not match";
}

void BacktraceLineGdb::rate()
{
    LineRating r;

    //for explanations, see the LineRating enum definition above
    if ( !fileName().isEmpty() ) {
        r = Good;
    } else if( !libraryName().isEmpty() ) {
        if( functionName() == "??" )
            r = MissingFunction;
        else
            r = MissingSourceFile;
    } else {
        if ( functionName() == "??" )
            r = MissingEverything;
        else
            r = MissingLibrary;
    }

    d->m_rating = r;
}

//END BacktraceLineGdb

//BEGIN BacktraceParserGdb

struct BacktraceParserGdb::Private
{
    Private() : m_possibleKCrashStart(0), m_threadsCount(0),
                m_isBelowSignalHandler(false), m_frameZeroAppeared(false),
                m_cachedUsefulness(BacktraceParser::InvalidUsefulness) {}

    QList<BacktraceLineGdb> m_linesList;
    QList<BacktraceLineGdb> m_usefulLinesList;
    int m_possibleKCrashStart;
    int m_threadsCount;
    bool m_isBelowSignalHandler;
    bool m_frameZeroAppeared;

    mutable BacktraceParser::Usefulness m_cachedUsefulness;
};

K_GLOBAL_STATIC_WITH_ARGS(const QSet<QString>, blacklistedFunctions, (
    QSet<QString>() << "raise" << "abort" << "_start" << "__libc_start_main"
                    << "__assert_fail" << "do_assert" << "qt_message_output"
                    << "qFatal" << "clone" << "start_thread"
));

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

void BacktraceParserGdb::parseLine(const QString & lineStr)
{
    BacktraceLineGdb line(lineStr);
    switch (line.type())
    {
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
            d->m_linesList.erase( d->m_linesList.begin() + d->m_possibleKCrashStart, d->m_linesList.end() );
            d->m_linesList.insert( d->m_possibleKCrashStart, BacktraceLineGdb("[KCrash Handler]\n") );
            d->m_isBelowSignalHandler = true; //next line is the first below the signal handler
            break;
        case BacktraceLineGdb::StackFrame:
            // gdb workaround - (v6.8 at least) - 'thread apply all bt' writes
            // the #0 stack frame again at the end.
            // Here we ignore this frame by using a flag that tells us whether
            // this is the first or the second time that the #0 frame appears in this thread.
            // The flag is cleared on each thread start.
            if ( line.frameNumber() == 0 ) {
                if ( d->m_frameZeroAppeared )
                    break; //break from the switch so that the frame is not added to the list.
                else
                    d->m_frameZeroAppeared = true;
            }

            //rate the stack frame if we are below the signal handler and the function is not blacklisted.
            if ( d->m_isBelowSignalHandler && !blacklistedFunctions->contains(line.functionName()) )
                d->m_usefulLinesList.append(line);

            d->m_linesList.append(line);
            break;
        case BacktraceLineGdb::Unknown:
            if ( !d->m_linesList.isEmpty() )
            {
                //when the line is too long, gdb splits it into two lines.
                //This breaks parsing and results in two Unknown lines instead of a StackFrame one.
                //Here we workaround this by joining the two lines when such a scenario is detected.
                BacktraceLineGdb lastLine = d->m_linesList.at(d->m_linesList.size() - 1);
                if ( lineStr.startsWith(' ') &&    //gdb always adds some whitespace to the second line
                    lastLine.toString().startsWith('#') && //last line must be a stack frame
                    //the previous line either is Unknown or is a StackFrame that misses
                    //the whole part after (from|at), so it has MissingLibrary or MissingEverything rating.
                    (lastLine.type() == BacktraceLineGdb::Unknown ||
                     lastLine.rating() == BacktraceLineGdb::MissingLibrary ||
                     lastLine.rating() == BacktraceLineGdb::MissingEverything)
                   )
                {
                    //remove the last line from the cache.
                    d->m_linesList.removeLast();
                    //if m_usefulLinesList also contains this line, remove it from there too.
                    if ( !d->m_usefulLinesList.isEmpty() && d->m_usefulLinesList.last() == lastLine)
                        d->m_usefulLinesList.removeLast();
                    //parse again the two lines joined together.
                    parseLine(lastLine.toString() + lineStr);
                    break;
                }
            }
            //in all other cases of Unknown lines, just append them to the backtrace.
            d->m_linesList.append(line);
            break;
        default:
            d->m_linesList.append(line);
            break;
    }
}

QString BacktraceParserGdb::parsedBacktrace() const
{
    QString result;

    //if there is no d, the debugger has not run, so we return an empty string.
    if ( !d )
        return result;

    QList<BacktraceLineGdb>::const_iterator i;
    for(i = d->m_linesList.constBegin(); i != d->m_linesList.constEnd(); ++i)
    {
        //if there is only one thread, we can omit the thread indicator, the thread header and all the empty lines.
        if ( d->m_threadsCount == 1 && ((*i).type() == BacktraceLineGdb::ThreadIndicator
                || (*i).type() == BacktraceLineGdb::ThreadStart || (*i).type() == BacktraceLineGdb::EmptyLine) )
            continue;
        result += (*i).toString();
    }
    return result;
}

BacktraceParser::Usefulness BacktraceParserGdb::backtraceUsefulness() const
{
    //if there is no d, the debugger has not run,
    //so we can say that the (inexistent) backtrace Useless.
    if ( !d )
        return Useless;

    //cache the usefulness value because this function will probably be called many times
    if ( d->m_cachedUsefulness != InvalidUsefulness )
        return d->m_cachedUsefulness;

    uint rating = 0, bestPossibleRating = 0, counter = 0;
    QList<BacktraceLineGdb>::const_iterator i;
    for(i = d->m_usefulLinesList.constBegin(); i != d->m_usefulLinesList.constEnd(); ++i)
    {
       uint multiplier = d->m_usefulLinesList.size() - counter; //give weight to the first lines
       rating += static_cast<uint>((*i).rating()) * multiplier;
       bestPossibleRating += static_cast<uint>(BacktraceLineGdb::BestRating) * multiplier;
       counter++;

       kDebug() << (*i).rating() << (*i).toString();
    }

    kDebug() << "Rating:" << rating << "out of" << bestPossibleRating;

    uint usefulRating = bestPossibleRating*0.90;
    uint maybeUsefulRating = bestPossibleRating*0.70;
    uint probablyUselessRating = bestPossibleRating*0.35;
    Usefulness usefulness = Useless;

    if(rating >= usefulRating)
    {
        usefulness = ReallyUseful;
    }
    else if (rating >= maybeUsefulRating)
    {
        usefulness = MayBeUseful;
    }
    else if (rating >= probablyUselessRating)
    {
        usefulness = ProbablyUseless;
    }

    kDebug() << bestPossibleRating << usefulRating << maybeUsefulRating << probablyUselessRating << usefulness;

    d->m_cachedUsefulness = usefulness;
    return usefulness;
}

QStringList BacktraceParserGdb::firstValidFunctions() const
{
    //if there is no d, the debugger has not run, so we have no functions to return.
    if ( !d )
        return QStringList();

    QStringList result;
    QList<BacktraceLineGdb>::const_iterator i;

    //get only the first three valid functions that are encountered
    for(i = d->m_usefulLinesList.constBegin(); i != d->m_usefulLinesList.constEnd() || result.size() < 3; ++i)
    {
        if ( (*i).functionName() != "??" )
            result.append((*i).functionName());
    }

    return result;
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

void BacktraceParserNull::parseLine(const QString & lineStr)
{
    m_lines.append(lineStr);
}

//END BacktraceParserNull

#include "backtraceparser.moc"
