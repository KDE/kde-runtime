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
class BackTrace;

#include <qwidget.h>

class KrashDebugger : public QWidget
{
  Q_OBJECT

public:
  KrashDebugger(const KrashConfig *krashconf, QWidget *parent = 0, const char *name = 0);
  ~KrashDebugger();

public slots:
  void slotAppend(const QString &);
  void slotDone();
  void slotSomeError();

protected:
 void startDebugger();

protected slots:
 virtual void showEvent(QShowEvent *e);

private:
  const KrashConfig *m_krashconf;
  BackTrace *m_proctrace;
  QLabel *m_status;
  QTextView *m_backtrace;
};

#endif
