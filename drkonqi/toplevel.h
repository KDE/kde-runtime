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

#include <kdialogbase.h>

class Toplevel : public KDialogBase
{
  Q_OBJECT

public:
  Toplevel(int signal, const KAboutData &_oldabout, QWidget *parent = 0, const char * name = 0);
  ~Toplevel();

private:
  // helper methods
  void expandString(QString &str) const;
  void readConfig();
  QString generateText() const;

protected slots:
  void slotUser1();

private:
  bool showbugreport; 
  int signalnum;

  KAboutData oldabout;
  QString signal;

  QString signaldetails;
  QString whattodohint;
  QString errordescription;
};

#endif
