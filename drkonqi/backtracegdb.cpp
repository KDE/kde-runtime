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
{
}

BackTraceGdb::~BackTraceGdb()
{
}

// remove stack frames added because of KCrash
QString BackTraceGdb::processDebuggerOutput( QString bt )
{
  // remove crap
  if( !m_krashconf->removeFromBacktraceRegExp().isEmpty())
    bt.replace(QRegExp( m_krashconf->removeFromBacktraceRegExp()), QString());

  if( !m_krashconf->kcrashRegExp().isEmpty())
    {
    QRegExp kcrashregexp( m_krashconf->kcrashRegExp());
    int pos = kcrashregexp.indexIn( bt );
    if( pos >= 0 )
      {
      int len = kcrashregexp.matchedLength();
      if( bt[ pos ] == '\n' )
        {
        ++pos;
        --len;
        }
      bt.remove( pos, len );
      bt.insert( pos, QLatin1String( "[KCrash handler]\n" ));
      }
    }
  return bt;
}

// analyze backtrace for usefulness
bool BackTraceGdb::usefulDebuggerOutput( QString bt )
{
  if( m_krashconf->disableChecks())
      return true;
  // prepend and append newline, so that regexps like '\nwhatever\n' work on all lines
  QString strBt = '\n' + bt + '\n';
  // how many " ?? " in the bt ?
  int unknown = 0;
  if( !m_krashconf->invalidStackFrameRegExp().isEmpty())
    unknown = strBt.count( QRegExp( m_krashconf->invalidStackFrameRegExp()));
  // how many stack frames in the bt ?
  int frames = 0;
  if( !m_krashconf->frameRegExp().isEmpty())
    frames = strBt.count( QRegExp( m_krashconf->frameRegExp()));
  else
    frames = strBt.count('\n');
  bool tooShort = false;
  if( !m_krashconf->neededInValidBacktraceRegExp().isEmpty())
    tooShort = ( strBt.indexOf( QRegExp( m_krashconf->neededInValidBacktraceRegExp())) == -1 );
  return !bt.isNull() && !tooShort && (unknown < frames);
}

