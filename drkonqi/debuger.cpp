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
#include "debuger.h"
#include "debuger.moc"

KrashDebuger :: KrashDebuger (const KrashConfig *krashconf, QWidget *parent, const char *name)
  : QWidget( parent, name ),
    m_krashconf(krashconf),
    m_proc(0)
{
  QVBoxLayout *hbox = new QVBoxLayout(this);
  hbox->setAutoAdd(TRUE);

  m_backtrace = new QTextView(this);
  m_backtrace->setTextFormat(Qt::PlainText);
  m_status = new QLabel(this);
}

KrashDebuger :: ~KrashDebuger()
{
  // we don't want the gdb process to hang around
  m_proc->kill();
  delete m_proc;
}

void KrashDebuger :: slotProcessExited(KProcess *proc)
{
  if (proc->normalExit() || proc->exitStatus() == 0)
    m_status->setText(i18n("Done."));
  else
    m_status->setText(i18n("Unable to create backtrace."));
}

void KrashDebuger :: slotReadInput(KProcess *, char *buffer, int buflen)
{
  m_status->setText(i18n("Loading backtrace..."));
  QString str = QString::fromLocal8Bit(buffer, buflen);

  m_backtrace->setText(m_backtrace->text() + str);
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

  m_status->setText(i18n("Loading symboles..."));

  KTempFile temp;
  // temp.setAutoDelete(true);
  QTextStream *stream = temp.textStream();
  stream->writeRawBytes("bt\n", 3); // backtrace

  // start the debuger
  m_proc = new KProcess;
  *m_proc << QString::fromLatin1("gdb")
	  << QString::fromLatin1("-n")
	  << QString::fromLatin1("-batch")
	  << QString::fromLatin1("-x")
	  << temp.name()
	  << QString::fromLatin1(m_krashconf->appName())
    	  << QString::number(m_krashconf->pid());

  m_proc->start( KProcess::NotifyOnExit, KProcess::All );

  connect(m_proc, SIGNAL(receivedStdout(KProcess*, char*, int)),
	  SLOT(slotReadInput(KProcess*, char*, int)));
  connect(m_proc, SIGNAL(processExited(KProcess*)),
	  SLOT(slotProcessExited(KProcess*)));
}


