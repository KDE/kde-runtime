/*****************************************************************
 * drkonki - The KDE Crash Handler
 * 
 * $Id$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <signal.h>

#include <qtextview.h>
#include <qlayout.h>
#include <qtextstream.h>
#include <qlabel.h>

#include <kprocess.h>
#include <ktempfile.h>
#include <klocale.h>

#include "krashconf.h"
#include "debugger.h"
#include "debugger.moc"

KrashDebugger :: KrashDebugger (const KrashConfig *krashconf, QWidget *parent, const char *name)
  : QWidget( parent, name ),
    m_krashconf(krashconf),
    m_proc(0), m_temp(0)
{
  QVBoxLayout *hbox = new QVBoxLayout(this);
  hbox->setAutoAdd(TRUE);

  m_backtrace = new QTextView(this);
  m_backtrace->setTextFormat(Qt::PlainText);
  m_status = new QLabel(this);
}

KrashDebugger :: ~KrashDebugger()
{
  pid_t pid = m_proc ? m_proc->getPid() : 0;
  // we don't want the gdb process to hang around
  delete m_proc; // this will kill gdb (SIGKILL, signal 9)

  // continue the process we ran backtrace on. Gdb sends SIGSTOP to the 
  // process. For some reason it doesn't work if we send the signal before the
  // gdb has exited, so we better wayt for it
  // Do not touch it if we never had never ran backtrace.
  if (pid) {
    waitpid(pid, NULL, 0);
    kill(m_krashconf->pid(), SIGCONT);
  }

  delete m_temp;
}

void KrashDebugger :: slotProcessExited(KProcess *proc)
{
  if (proc->normalExit() || proc->exitStatus() == 0)
    m_status->setText(i18n("Done."));
  else
    m_status->setText(i18n("Unable to create backtrace."));
}

void KrashDebugger :: slotReadInput(KProcess *, char *buffer, int buflen)
{
  m_status->setText(i18n("Loading backtrace..."));
  QString str = QString::fromLocal8Bit(buffer, buflen);

  m_backtrace->setText(m_backtrace->text() + str);
}

void KrashDebugger :: showEvent(QShowEvent *e)
{
  QWidget::showEvent(e);
  startDebugger();
}

void KrashDebugger :: startDebugger()
{
  // Only start one copy
  if (m_proc) return;

  m_status->setText(i18n("Loading symboles..."));

  m_temp = new KTempFile;
  m_temp->setAutoDelete(TRUE);
  int handle = m_temp->handle();
  ::write(handle, "bt\n", 3);
  ::fsync(handle);

  // start the debugger
  m_proc = new KProcess;
  *m_proc << QString::fromLatin1("gdb")
	  << QString::fromLatin1("-n")
	  << QString::fromLatin1("-batch")
	  << QString::fromLatin1("-x")
	  << m_temp->name()
	  << QString::fromLatin1(m_krashconf->appName())
    	  << QString::number(m_krashconf->pid());

  m_proc->start( KProcess::NotifyOnExit, KProcess::All );

  connect(m_proc, SIGNAL(receivedStdout(KProcess*, char*, int)),
	  SLOT(slotReadInput(KProcess*, char*, int)));
  connect(m_proc, SIGNAL(processExited(KProcess*)),
	  SLOT(slotProcessExited(KProcess*)));
}


