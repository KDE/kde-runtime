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

#include "backtrace.h"

#include <KDebug>
#include <KStandardDirs>
#include <KMessageBox>
#include <KLocale>
#include <KTemporaryFile>
#include <KShell>

#include <signal.h>

#include "krashconf.h"
#include "backtrace.moc"

BackTrace::BackTrace(const KrashConfig *krashconf, QObject *parent)
  : QObject(parent),
    m_krashconf(krashconf), m_temp(0)
{
  m_proc = new KProcess;
  m_proc->setEnv( "LC_ALL", "C" ); // force C locale
}

BackTrace::~BackTrace()
{
  // Do not touch it if we never ran backtrace.
  if (m_proc->state() == QProcess::Running)
  {
    m_proc->kill();
    m_proc->waitForFinished();
    ::kill(m_krashconf->pid(), SIGCONT);
  }

  delete m_proc;
  delete m_temp;
}

bool BackTrace::start()
{
  QString exec = m_krashconf->tryExec();
  if ( !exec.isEmpty() && KStandardDirs::findExe(exec).isEmpty() )
    return false;

  m_temp = new KTemporaryFile;
  m_temp->open();
  m_temp->write(m_krashconf->backtraceCommand().toLatin1());
  m_temp->write("\n", 1);
  m_temp->flush();

  // start the debugger
  QString str = m_krashconf->debuggerBatchCommand();
  m_krashconf->expandString(str, KrashConfig::ExpansionUsageShell, m_temp->fileName());

  *m_proc << KShell::splitArgs(str);
  m_proc->setOutputChannelMode(KProcess::SeparateChannels); // Drop stderr
  m_proc->setNextOpenMode(QIODevice::ReadWrite | QIODevice::Text);
  connect(m_proc, SIGNAL(readyReadStandardOutput()),
          SLOT(slotReadInput()));
  connect(m_proc, SIGNAL(finished(int, QProcess::ExitStatus)),
          SLOT(slotProcessExited(int, QProcess::ExitStatus)));

  m_proc->start();
  if (!m_proc->waitForStarted())
  {
        emit someError();
        return false;
  }
  return true;
}

void BackTrace::slotReadInput()
{
    // we do not know if the output array ends in the middle of an utf-8 sequence
    m_output += m_proc->readAllStandardOutput();

    int pos;
    while ((pos = m_output.indexOf('\n')) != -1)
    {
        QString line = QString::fromLocal8Bit(m_output, pos + 1);
        m_output.remove(0, pos + 1);

        emit append(line);
    }
}

void BackTrace::slotProcessExited(int exitCode, QProcess::ExitStatus exitStatus)
{
  // start it again
  ::kill(m_krashconf->pid(), SIGCONT);

  if (exitStatus != QProcess::NormalExit || exitCode != 0)
  {
    emit someError();
    return;
  }

  emit done();
}
