/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
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
 *****************************************************************/

#include "backtracegdb.h"

#include <QRegExp>

#include <KProcess>
#include <KDebug>
#include <KStandardDirs>
#include <KMessageBox>
#include <KLocale>
#include <ktemporaryfile.h>
#include <KShell>

#include <signal.h>

#include "krashconf.h"
#include "backtracegdb.moc"

BackTraceGdb::BackTraceGdb(const KrashConfig *krashconf, QObject *parent)
  : BackTrace(krashconf,parent)
  , backtraceWithCrash( 0 )
{
}

BackTraceGdb::~BackTraceGdb()
{
}

QString BackTraceGdb::processDebuggerOutput( QString bt )
{
    QStringList lines = bt.split( '\n' );
    // read the common part until backtraces are found (either line starting with
    // something like 'Thread 1 (Thread 0xb5d3f8e0 (LWP 25088)):' or directly the first
    // backtrace line like '#0  0xffffe430 in __kernel_vsyscall ()'
    QRegExp threadheading( "Thread [0-9]+\\s+\\(Thread 0x[0-9a-f]+\\s+\\(LWP [0-9]+\\)\\):" );
    QRegExp backtracestart( "#0 .*" );
    while( !lines.isEmpty()
        && !threadheading.exactMatch( lines.first()) && !backtracestart.exactMatch( lines.first()))
    {
        common.append( lines.takeFirst());
    }
    // current gdb seems to have a bug in 'thread apply all bt' in that the backtrace
    // has the '#0 ...' frame again appended, so when the backtraces have thread headings,
    // do not use '#0 ...' for detecting the start of a backtrace at all
    bool hasthreadheadings = threadheading.exactMatch( lines.first());
    // read backtraces per thread
    while( !lines.isEmpty())
    {
        QStringList backtrace;
        backtrace.append( lines.takeFirst());
        while( !lines.isEmpty() && !threadheading.exactMatch( lines.first())
            && (hasthreadheadings || !backtracestart.exactMatch( lines.first())))
        {
            backtrace.append( lines.takeFirst());
        }
        backtraces.append( backtrace );
    }
    // remove some not very useful lines like '(no debugging symbols)', etc.
    common = removeLines( common, QRegExp( ".*\\(no debugging symbols found\\).*" ));
    common = removeLines( common, QRegExp( "\\[Thread debugging using libthread_db enabled\\]" ));
    common = removeLines( common, QRegExp( "\\[New Thread 0x[0-9a-f]+\\s+\\(LWP [0-9]+\\)\\]" ));
    // remove the line that shows where the process is at the moment '0xffffe430 in __kernel_vsyscall ()',
    // gdb prints that automatically, but it will be later visible again in the backtrace
    common = removeFirstLine( common, QRegExp( "0x[0-9a-f]+\\s+in .*\\(\\)" ));
    // it is obvious which thread is current when there is only one
    if( backtraces.count() == 1 )
        common = removeLines( common, QRegExp( "\\[Current thread is [0-9]+\\s+\\(process [0-9]+\\)\\]" ));
    for( int i = 0;
         i < backtraces.count();
         ++i )
        processBacktrace( i );

    QString ret = common.join( "\n" );
    foreach( const QStringList& bt, backtraces )
        ret += '\n' + bt.join( "\n" );
    ret += '\n';  
    return ret;
}

void BackTraceGdb::processBacktrace( int index )
{
    QStringList& bt = backtraces[ index ];
    if( prettyKcrashHandler( index ))
        backtraceWithCrash = index; // this is the important backtrace
    // gdb workaround - (v6.8 at least) - 'thread apply all bt' writes
    // the #0 stack frame again at the end
    for( int i = bt.count() - 1;
         i >= 0;
         --i )
    {
        if( bt[ i ].isEmpty())
            continue;
        QRegExp fakebacktracestart( "#0 .*" );
        if( fakebacktracestart.exactMatch( bt[ i ] ))
            bt.removeAt( i );
        break;
    }
}

bool BackTraceGdb::usefulDebuggerOutput( QString /*bt - use already done parsing*/ )
{
    if( common.isEmpty() || backtraces.isEmpty())
        return false;
    if( !usefulBacktrace( backtraceWithCrash ))
        return false;
    // ignore other backtraces from other threads as hopefully irrelevant
    return true;
}

bool BackTraceGdb::usefulBacktrace( int index )
{
    QStringList& bt = backtraces[ index ];
    QRegExp backtracestart( "#[0-9]+ .*" ); // not #0, probably replaces by [KCrash...]
    int start = bt.indexOf( backtracestart );
    if( start < 0 )
        return false;
    int end = bt.count() - 1;
    while( end >= 0 && bt[ end ].isEmpty())
        --end;
    int frames = end - start + 1;
    // topmost items must mean something, otherwise the backtrace is not very usable,
    // which means ok is:
    // - no ?? at all in first 3 items (or all of them, if there are less than 3 frames)
    QRegExp unknown( "#[0-9]+\\s+0x[0-9a-f]+\\s+in\\s+\\?\\?\\s+.*" );
    bool ok = false;
    if( frames >= 3 && !unknown.exactMatch( bt[ start ] ) && !unknown.exactMatch( bt[ start + 1 ] )
        && !unknown.exactMatch( bt[ start + 2 ] ))
    {
        ok = true;
    }
    // - only one frame, must be valid
    if( frames == 1 && !unknown.exactMatch( bt[ start ] ))
        ok = true;
    // - only two frames, must be both valid
    if( frames == 2 && !unknown.exactMatch( bt[ start ] ) && !unknown.exactMatch( bt[ start + 1 ] ))
        ok = true;
    // - 1st item is ?? but 4 following ones are okay (can be e.g. with a jump to NULL function pointer)
    if( frames >= 5 && unknown.exactMatch( bt[ start ] ) && !unknown.exactMatch( bt[ start + 1 ] )
        && !unknown.exactMatch( bt[ start + 2 ] ) && !unknown.exactMatch( bt[ start + 3 ]))
    {
        ok = true;
    }
    if( !ok )
        return false;
    return true;
}

QStringList BackTraceGdb::removeLines( QStringList list, const QRegExp& regexp )
{
    for( QStringList::Iterator it = list.begin();
         it != list.end();
        )
    {
    if( regexp.exactMatch( *it ))
        it = list.erase( it );
    else
        ++it;
    }
    return list;
}

QStringList BackTraceGdb::removeFirstLine( QStringList list, const QRegExp& regexp )
{
    for( int i = 0;
         i < list.count();
         ++i )
    {
        if( regexp.exactMatch( list[ i ] ))
        {
            list.removeAt( i );
            break;
        }
    }
    return list;
}

// replace the stack frames from KCrash with '[KCrash handler]'
bool BackTraceGdb::prettyKcrashHandler( int index )
{
    QStringList& bt = backtraces[ index ];
    QRegExp backtracestart( "#[0-9]+ .*" );
    QRegExp sighandler( "#[0-9]+\\s+<signal handler called>.*" );
    int start = bt.indexOf( backtracestart );
    int sigpos = bt.indexOf( sighandler );
    if( sigpos >= 0 && start >= 0 )
    {
        bt.erase( bt.begin() + start, bt.begin() + sigpos + 1 );
        bt.insert( start, "[KCrash Handler]" );
        return true;
    }
    return false;
}
