/*****************************************************************
 * drkonki - The KDE Crash Handler
 * 
 * $Id:$
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#ifndef DEBUGER_H
#define DEBUGER_H

class QTextView;
class KrashConfig;
class KProcess;

#include <qwidget.h>

class KrashDebuger : public QWidget
{
  Q_OBJECT

public:
  KrashDebuger(const KrashConfig *krashconf, QWidget *parent = 0, const char *name = 0);
  ~KrashDebuger();

public slots:
  void slotReadInput(KProcess *proc, char *buffer, int buflen);

protected:
 void startDebuger();

protected slots:
 virtual void showEvent(QShowEvent *e);

private:
  const KrashConfig *m_krashconf;
  KProcess *m_proc;
  QTextView *m_textview;
};

#endif


