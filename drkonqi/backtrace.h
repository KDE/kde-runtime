/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * $Id$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#ifndef BACKTRACE_H
#define BACKTRACE_H

class KProcess;
class KrashConfig;
class KTempFile;

#include <qobject.h>

class BackTrace : public QObject
{
  Q_OBJECT

 public:
  BackTrace(const KrashConfig *krashconf, QObject *parent,
	    const char *name = 0);
  ~BackTrace();

  void start();

 signals:
  // Just the new text
  void append(const QString &str);

  void someError();
  void done();
  void done(const QString &);

 protected slots:
  void slotProcessExited(KProcess * proc);
  void slotReadInput(KProcess * proc, char * buf, int buflen);

 private:
  KProcess *m_proc;
  const KrashConfig *m_krashconf;
  KTempFile *m_temp;
  QString str;
};
#endif
