/*****************************************************************
 * drkonki - The KDE Crash Handler
 * 
 * toplevel.cpp
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#ifndef TOPLEVEL_H
#define TOPLEVEL_H

class KAboutData;
class KrashConfig;
class DrKBugReport;

#include <kdialogbase.h>

class Toplevel : public KDialogBase
{
  Q_OBJECT

public:
  Toplevel(const KrashConfig *krash, QWidget *parent = 0, const char * name = 0);
  ~Toplevel();

private:
  // helper methods
  void expandString(QString &str) const;
  QString generateText() const;

protected slots:
  void slotUser1();

 protected slots:
  void slotBacktraceSomeError();
  void slotBacktraceDone(const QString &);

private:
  const KrashConfig *m_krashconf;
  DrKBugReport *bugreport;
};

#endif
