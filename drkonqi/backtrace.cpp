/*****************************************************************
 * drkonki - The KDE Crash Handler
 *
 * $Id$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#include <qfile.h>

#include <kprocess.h>
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
  pid_t pid = m_proc ? m_proc->getPid() : 0;
  // we don't want the gdb process to hang around
  delete m_proc; // this will kill gdb (SIGKILL, signal 9)

  // continue the process we ran backtrace on. Gdb sends SIGSTOP to the 
  // process. For some reason it doesn't work if we send the signal before
  // gdb has exited, so we better wait for it.
  // Do not touch it if we never ran backtrace.
  if (pid) {
    waitpid(pid, NULL, 0);
    kill(m_krashconf->pid(), SIGCONT);
  }

  delete m_temp;
}

void BackTrace::start()
{
  m_temp = new KTempFile;
  m_temp->setAutoDelete(TRUE);
  int handle = m_temp->handle();
  ::write(handle, "bt\n", 3); // this is the command for a backtrace
  ::fsync(handle);

  // start the debugger
  m_proc = new KProcess;
  *m_proc << QString::fromLatin1("gdb")
	  << QString::fromLatin1("-n")
	  << QString::fromLatin1("-batch")
	  << QString::fromLatin1("-x")
	  << m_temp->name()
	  << QFile::decodeName(m_krashconf->appName())
    	  << QString::number(m_krashconf->pid());

  connect(m_proc, SIGNAL(receivedStdout(KProcess*, char*, int)),
 	  SLOT(slotReadInput(KProcess*, char*, int)));
  connect(m_proc, SIGNAL(processExited(KProcess*)),
	  SLOT(slotProcessExited(KProcess*)));

  m_proc->start ( KProcess::NotifyOnExit, KProcess::All );
}

void BackTrace::slotReadInput(KProcess *, char* buf, int buflen)
{
  QString newstr = QString::fromLocal8Bit(buf, buflen);
  str.append(newstr);

  emit append(newstr);
}

void BackTrace::slotProcessExited(KProcess *proc)
{
  // start it again
  kill(m_krashconf->pid(), SIGCONT);

  int unknown = str.contains(QString::fromLatin1(" ?? "));
  int lines = str.contains('\n');
  if (proc->normalExit() && proc->exitStatus() == 0 &&
      !str.isNull() && unknown < lines) {
    emit done();
    emit done(str);
  } else
    emit someError();
}


