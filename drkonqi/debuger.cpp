/*****************************************************************
 * drkonki - The KDE Crash Handler
 * 
 * $Id:$
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

#include <kprocess.h>

#include "krashconf.h"
#include "debuger.h"
#include "debuger.moc"

KrashDebuger :: KrashDebuger (const KrashConfig *krashconf, QWidget *parent, const char *name)
  : QWidget( parent, name ),
    m_krashconf(krashconf),
    m_proc(0)
{
  QVBoxLayout *hbox = new QVBoxLayout(this);
  hbox->setAutoAdd(TRUE);

  m_textview = new QTextView(this);
}

KrashDebuger :: ~KrashDebuger()
{
  // we don't want the gdb process to hang around
  kill(m_proc->getPid(), SIGTERM);
  delete m_proc;
}

void KrashDebuger :: slotReadInput(KProcess *, char *buffer, int buflen)
{
  QString str = QString::fromLocal8Bit(buffer, buflen);

  if (str.left(6) == QString::fromLatin1("(gdb) ")) return;

  m_textview->append(str);
}

void KrashDebuger :: showEvent(QShowEvent *e)
{
  QWidget::showEvent(e);
  startDebuger();
}

void KrashDebuger :: startDebuger()
{
  // Only start one copy
  if (m_proc) return;

  // start the debuger
  m_proc = new KProcess;
  *m_proc << QString::fromLatin1("nice")
	  << QString::fromLatin1("-10")
	  << QString::fromLatin1("gdb")
	  << QString::fromLatin1(m_krashconf->appName())
    	  << QString::number(m_krashconf->pid());

  m_proc->start( KProcess::NotifyOnExit, KProcess::All );
  m_proc->writeStdin("bt\nquit\ny\n", 10); // we want a backtrace 

  connect(m_proc, SIGNAL(receivedStdout(KProcess*, char*, int)), SLOT(slotReadInput(KProcess*, char*, int)));
  connect(m_proc, SIGNAL(receivedStderr(KProcess*, char*, int)), SLOT(slotReadInput(KProcess*, char*, int)));
}


