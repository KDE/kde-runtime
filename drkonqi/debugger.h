/*****************************************************************
 * drkonki - The KDE Crash Handler
 * 
 * $Id$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#ifndef DEBUGGER_H
#define DEBUGGER_H

class QTextView;
class QLabel;
class KrashConfig;
class KProcess;
class KTempFile;

#include <qwidget.h>

class KrashDebugger : public QWidget
{
  Q_OBJECT

public:
  KrashDebugger(const KrashConfig *krashconf, QWidget *parent = 0, const char *name = 0);
  ~KrashDebugger();

public slots:
  void slotReadInput(KProcess *proc, char *buffer, int buflen);
  void slotProcessExited(KProcess *proc); 

protected:
 void startDebugger();

protected slots:
 virtual void showEvent(QShowEvent *e);

private:
  const KrashConfig *m_krashconf;
  KProcess *m_proc;
  KTempFile *m_temp;
  QLabel *m_status;
  QTextView *m_backtrace;
};

#endif
