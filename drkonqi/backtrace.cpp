/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * Copyright (C) 2000-2003 Hans Petter Bieker <bieker@kde.org>
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

#include <qfile.h>
#include <qregexp.h>

#include <kprocess.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <ktempfile.h>

#include "krashconf.h"
#include "backtrace.h"
#include "backtrace.moc"

BackTrace::BackTrace(const KrashConfig *krashconf, QObject *parent,
                     const char *name)
  : QObject(parent, name),
    m_krashconf(krashconf), m_temp(0)
{
  m_proc = new KProcess;
}

BackTrace::~BackTrace()
{
  pid_t pid = m_proc ? m_proc->pid() : 0;
  // we don't want the gdb process to hang around
  delete m_proc; // this will kill gdb (SIGKILL, signal 9)

  // continue the process we ran backtrace on. Gdb sends SIGSTOP to the
  // process. For some reason it doesn't work if we send the signal before
  // gdb has exited, so we better wait for it.
  // Do not touch it if we never ran backtrace.
  if (pid)
  {
    waitpid(pid, NULL, 0);
    kill(m_krashconf->pid(), SIGCONT);
  }

  delete m_temp;
}

void BackTrace::start()
{
  QString exec = m_krashconf->tryExec();
  if ( !exec.isEmpty() && KStandardDirs::findExe(exec).isEmpty() )
  {
    QObject * o = parent();

    if (o && !o->inherits("QWidget"))
    {
      o = NULL;
    }

    KMessageBox::error(
                        (QWidget *)o,
			i18n("Could not generate a backtrace as the debugger '%1' was not found.").arg(exec));
    return;
  }
  m_temp = new KTempFile;
  m_temp->setAutoDelete(TRUE);
  int handle = m_temp->handle();
  QString backtraceCommand = m_krashconf->backtraceCommand();
  const char* bt = backtraceCommand.latin1();
  ::write(handle, bt, strlen(bt)); // the command for a backtrace
  ::write(handle, "\n", 1);
  ::fsync(handle);

  // start the debugger
  m_proc = new KProcess;
  m_proc->setUseShell(true);

  QString str = m_krashconf->debuggerBatch();
  m_krashconf->expandString(str, true, m_temp->name());

  *m_proc << str;

  connect(m_proc, SIGNAL(receivedStdout(KProcess*, char*, int)),
          SLOT(slotReadInput(KProcess*, char*, int)));
  connect(m_proc, SIGNAL(processExited(KProcess*)),
          SLOT(slotProcessExited(KProcess*)));

  m_proc->start ( KProcess::NotifyOnExit, KProcess::All );
}

void BackTrace::slotReadInput(KProcess *, char* buf, int buflen)
{
  QString newstr = QString::fromLocal8Bit(buf, buflen);
  m_strBt.append(newstr);

  emit append(newstr);
}

void BackTrace::slotProcessExited(KProcess *proc)
{
  // start it again
  kill(m_krashconf->pid(), SIGCONT);

  if (proc->normalExit() && (proc->exitStatus() == 0) &&
      usefulBacktrace())
  {
    processBacktrace();
    emit done(m_strBt);
  }
  else
    emit someError();
}

// analyze backtrace for usefulness
bool BackTrace::usefulBacktrace()
{
  // remove crap
  if( !m_krashconf->removeFromBacktraceRegExp().isEmpty())
    m_strBt.replace(QRegExp( m_krashconf->removeFromBacktraceRegExp()), QString::null);

  if( m_krashconf->disableChecks())
      return true;
  // prepend and append newline, so that regexps like '\nwhatever\n' work on all lines
  QString strBt = '\n' + m_strBt + '\n';
  // how many " ?? " in the bt ?
  int unknown = 0;
  if( !m_krashconf->invalidStackFrameRegExp().isEmpty())
    unknown = strBt.contains( QRegExp( m_krashconf->invalidStackFrameRegExp()));
  // how many stack frames in the bt ?
  int frames = 0;
  if( !m_krashconf->frameRegExp().isEmpty())
    frames = strBt.contains( QRegExp( m_krashconf->frameRegExp()));
  else
    frames = strBt.contains('\n');
  bool tooShort = false;
  if( !m_krashconf->neededInValidBacktraceRegExp().isEmpty())
    tooShort = ( strBt.find( QRegExp( m_krashconf->neededInValidBacktraceRegExp())) == -1 );
  return !m_strBt.isNull() && !tooShort && (unknown < frames);
}

// remove stack frames added because of KCrash
void BackTrace::processBacktrace()
{
  if( !m_krashconf->kcrashRegExp().isEmpty())
    {
    QRegExp kcrashregexp( m_krashconf->kcrashRegExp());
    int pos = kcrashregexp.search( m_strBt );
    if( pos >= 0 )
      {
      int len = kcrashregexp.matchedLength();
      if( m_strBt[ pos ] == '\n' )
        {
        ++pos;
        --len;
        }
      m_strBt.remove( pos, len );
      m_strBt.insert( pos, QString::fromLatin1( "[KCrash handler]\n" ));
      }
    }
}
