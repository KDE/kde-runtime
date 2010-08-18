/*
    Copyright (C) 2009-2010  George Kiagiadakis <gkiagia@users.sourceforge.net>
    Copyright (C) 2010  Milian Wolff <mail@milianw.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "backtraceparsergdb.h"
#include "backtraceline.h"
#include <QtCore/QRegExp>
#include <QtCore/QStack>
#include <QtCore/QMetaEnum> //used for a kDebug() in BacktraceParserGdb::backtraceUsefulness()
#include <QtGui/QTextDocument>
#include <QtGui/QSyntaxHighlighter>
#include <KColorScheme>
#include <KDebug>

//BEGIN BacktraceLineGdb

class BacktraceLineGdb : public BacktraceLine
{
public:
    BacktraceLineGdb(const QString & line);

private:
    void parse();
    void rate();
};

BacktraceLineGdb::BacktraceLineGdb(const QString & lineStr)
        : BacktraceLine()
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
        d->m_functionName = regExp.cap(3).trimmed();

        if (!regExp.cap(6).isEmpty()) { //we have file information (stuff after from|at)
            if (regExp.cap(7) == "at") { //'at' means we have a source file
                d->m_file = regExp.cap(8);
            } else { //'from' means we have a library
                d->m_library == regExp.cap(8);
            }
        }

        kDebug() << d->m_stackFrameNumber << d->m_functionName << d->m_file << d->m_library;
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

    //for explanations, see the LineRating enum definition
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
    QString m_simplifiedBacktrace;
    QSet<QString> m_librariesWithMissingDebugSymbols;
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
        if (!d->m_isBelowSignalHandler) {
            //replace the stack frames of KCrash with a nice message
            d->m_linesList.erase(d->m_linesList.begin() + d->m_possibleKCrashStart, d->m_linesList.end());
            d->m_linesList.insert(d->m_possibleKCrashStart, BacktraceLineGdb("[KCrash Handler]\n"));
            d->m_isBelowSignalHandler = true; //next line is the first below the signal handler
        } else {
            //this is not the first time we see a crash handler frame on the same thread,
            //so we just add it to the list
            d->m_linesList.append(line);
        }
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
        || line.libraryName().contains("libpthread.so")
        || line.libraryName().contains("libglib-2.0.so") )
        return true;

    return false;
}

static bool isFunctionUseful(const BacktraceLineGdb & line)
{
    //We need the function name
    if ( line.rating() == BacktraceLineGdb::MissingEverything
        || line.rating() == BacktraceLineGdb::MissingFunction ) {
        return false;
    }

    //Misc ignores
    if ( line.functionName() == "__kernel_vsyscall"
         || line.functionName() == "raise"
         || line.functionName() == "abort" ) {
        return false;
    }

    //Ignore core Qt functions
    //(QObject can be useful in some cases)
    if ( line.functionName().startsWith(QLatin1String("QBasicAtomicInt::"))
        || line.functionName().startsWith(QLatin1String("QBasicAtomicPointer::"))
        || line.functionName().startsWith(QLatin1String("QAtomicInt::"))
        || line.functionName().startsWith(QLatin1String("QAtomicPointer::"))
        || line.functionName().startsWith(QLatin1String("QMetaObject::"))
        || line.functionName().startsWith(QLatin1String("QPointer::"))
        || line.functionName().startsWith(QLatin1String("QWeakPointer::"))
        || line.functionName().startsWith(QLatin1String("QSharedPointer::"))
        || line.functionName().startsWith(QLatin1String("QScopedPointer::"))
        || line.functionName().startsWith(QLatin1String("QMetaCallEvent::")) ) {
        return false;
    }

    //Ignore core Qt containers misc functions
    if ( line.functionName().endsWith(QLatin1String("::detach"))
        || line.functionName().endsWith(QLatin1String("::detach_helper"))
        || line.functionName().endsWith(QLatin1String("::node_create")) ) {
        return false;
    }

    //Misc Qt stuff
    if ( line.functionName() == "qt_message_output"
        || line.functionName() == "qt_message"
        || line.functionName() == "qFatal"
        || line.functionName().startsWith(QLatin1String("qGetPtrHelper"))
        || line.functionName().startsWith(QLatin1String("qt_meta_")) ) {
        return false;
    }

    return true;
}

static bool isFunctionUsefulForSearch(const BacktraceLineGdb & line)
{
    //Ignore Qt containers (and iterators Q*Iterator)
    if ( line.functionName().startsWith(QLatin1String("QList"))
        || line.functionName().startsWith(QLatin1String("QLinkedList"))
        || line.functionName().startsWith(QLatin1String("QVector"))
        || line.functionName().startsWith(QLatin1String("QStack"))
        || line.functionName().startsWith(QLatin1String("QQueue"))
        || line.functionName().startsWith(QLatin1String("QSet"))
        || line.functionName().startsWith(QLatin1String("QMap"))
        || line.functionName().startsWith(QLatin1String("QMultiMap"))
        || line.functionName().startsWith(QLatin1String("QHash"))
        || line.functionName().startsWith(QLatin1String("QMultiHash")) ) {
        return false;
    }

    return true;
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

        if ( line.rating() == BacktraceLineGdb::MissingFunction
            || line.rating() == BacktraceLineGdb::MissingSourceFile) {
            d->m_librariesWithMissingDebugSymbols.insert(line.libraryName().trimmed());
        }

        uint multiplier = ++counter; //give weight to the first lines
        rating += static_cast<uint>(line.rating()) * multiplier;
        bestPossibleRating += static_cast<uint>(BacktraceLineGdb::BestRating) * multiplier;

        kDebug() << line.rating() << line.toString();
    }

    //Generate a simplified backtrace
    //- Starts from the first useful function
    //- Max of 5 lines
    //- Replaces garbage with [...]
    //At the same time, grab the first three useful functions for search queries

    i.toFront(); //Reuse the list iterator
    int functionIndex = 0;
    int usefulFunctionsCount = 0;
    bool firstUsefulFound = false;
    while( i.hasNext() && functionIndex < 5 ) {
        const BacktraceLineGdb & line = i.next();
        if ( !lineShouldBeIgnored(line) && isFunctionUseful(line) ) { //Line is not garbage to use
            if (!firstUsefulFound) {
                firstUsefulFound = true;
            }
            //Save simplified backtrace line
            d->m_simplifiedBacktrace += line.toString();

            //Fetch three useful functions (only functionName) for search queries
            if (usefulFunctionsCount < 3 && isFunctionUsefulForSearch(line) &&
                !d->m_firstUsefulFunctions.contains(line.functionName())) {
                d->m_firstUsefulFunctions.append(line.functionName());
                usefulFunctionsCount++;
            }

            functionIndex++;
        } else if (firstUsefulFound) {
            //Add "[...]" if there are invalid functions in the middle
            d->m_simplifiedBacktrace += QLatin1String("[...]\n");
        }
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

QString BacktraceParserGdb::simplifiedBacktrace() const
{
    //if there is no cached usefulness, the usefulness calculation function has not run yet.
    //in this case, we need to run it because the simplified backtrace is also saved from there.
    if (d && d->m_cachedUsefulness == InvalidUsefulness) {
        kDebug() << "backtrace usefulness has not been calculated yet. calculating...";
        backtraceUsefulness();
    }

    //if there is no d, the debugger has not run, so we have backtrace.
    return d ? d->m_simplifiedBacktrace : QString();
}

QSet<QString> BacktraceParserGdb::librariesWithMissingDebugSymbols() const
{
    return d->m_librariesWithMissingDebugSymbols;
}

//BEGIN SUB GdbHighlighter

class GdbHighlighter : public QSyntaxHighlighter
{
public:
    GdbHighlighter(QTextDocument* parent, QList<BacktraceLineGdb> gdbLines)
        : QSyntaxHighlighter(parent)
    {
        // setup line lookup
        int l = 0;
        foreach(const BacktraceLineGdb& line, gdbLines) {
            lines.insert(l, line);
            l += line.toString().count('\n');
        }

        // setup formates
        KColorScheme scheme(QPalette::Active);

        crashFormat.setForeground(scheme.foreground(KColorScheme::NegativeText));
        threadFormat.setForeground(scheme.foreground(KColorScheme::NeutralText));
        urlFormat.setForeground(scheme.foreground(KColorScheme::LinkText));
        funcFormat.setForeground(scheme.foreground(KColorScheme::VisitedText));
        funcFormat.setFontWeight(QFont::Bold);
        otheridFormat.setForeground(scheme.foreground(KColorScheme::PositiveText));
        crapFormat.setForeground(scheme.foreground(KColorScheme::InactiveText));
    }

protected:
    virtual void highlightBlock(const QString& text)
    {
        int cur = 0;
        int next;
        int diff;
        const QRegExp hexptrPattern("0x[0-9a-f]+", Qt::CaseSensitive, QRegExp::RegExp2);
        int lineNr = currentBlock().firstLineNumber();
        while ( cur < text.length() ) {
            next = text.indexOf('\n', cur);
            if (next == -1) {
                next = text.length();
            }
            if (lineNr == 0) {
                // line that contains 'Application: ...'
                ++lineNr;
                cur = next;
                continue;
            }

            diff = next - cur;

            QString lineStr = text.mid(cur, diff).append('\n');
            // -1 since we skip the first line
            QMap< int, BacktraceLineGdb >::iterator it = lines.lowerBound(lineNr - 1);
            // lowerbound would return the next higher item, even though we want the former one
            if (it.key() > lineNr - 1) {
                --it;
            }
            const BacktraceLineGdb& line = it.value();

            if (line.type() == BacktraceLineGdb::KCrash) {
                setFormat(cur, diff, crashFormat);
            } else if (line.type() == BacktraceLineGdb::ThreadStart || line.type() == BacktraceLineGdb::ThreadIndicator) {
                setFormat(cur, diff, threadFormat);
            } else if (line.type() == BacktraceLineGdb::StackFrame) {
                if (!line.fileName().isEmpty()) {
                    int colonPos = line.fileName().lastIndexOf(':');
                    setFormat(lineStr.indexOf(line.fileName()), colonPos == -1 ? line.fileName().length() : colonPos, urlFormat);
                }
                if (!line.libraryName().isEmpty()) {
                    setFormat(lineStr.indexOf(line.libraryName()), line.libraryName().length(), urlFormat);
                }
                if (!line.functionName().isEmpty()) {
                    int idx = lineStr.indexOf(line.functionName());
                    if (idx != -1) {
                        // highlight Id::Id::Id::Func
                        // Id should have otheridFormat, :: no format and Func funcFormat
                        int i = idx;
                        int from = idx;
                        while (i < idx + line.functionName().length()) {
                            if (lineStr.at(i) == ':') {
                                setFormat(from, i - from, otheridFormat);
                                // skip ::
                                i += 2;
                                from = i;
                                continue;
                            } else if (lineStr.at(i) == '<' || lineStr.at(i) == '>') {
                                setFormat(from, i - from, otheridFormat);
                                ++i;
                                from = i;
                                continue;
                            }
                            ++i;
                        }
                        setFormat(from, i - from, funcFormat);
                    }
                }
                // highlight hexadecimal ptrs
                int idx = 0;
                while ((idx = hexptrPattern.indexIn(lineStr, idx)) != -1) {
                    if (hexptrPattern.cap() == "0x0") {
                        setFormat(idx, hexptrPattern.matchedLength(), crashFormat);
                    }
                    idx += hexptrPattern.matchedLength();
                }
            } else if (line.type() == BacktraceLineGdb::Crap) {
                setFormat(cur, diff, crapFormat);
            }

            cur = next;
            ++lineNr;
        }
    }

private:
    QMap<int, BacktraceLineGdb> lines;
    QTextCharFormat crashFormat;
    QTextCharFormat threadFormat;
    QTextCharFormat urlFormat;
    QTextCharFormat funcFormat;
    QTextCharFormat otheridFormat;
    QTextCharFormat crapFormat;
};

//END SUB GdbHighlighter

QSyntaxHighlighter* BacktraceParserGdb::highlightBacktrace(QTextDocument* document) const
{
    return new GdbHighlighter(document, d->m_linesList);
}

//END BacktraceParserGdb

#include "backtraceparsergdb.moc"
