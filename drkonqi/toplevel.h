/*****************************************************************
 * drkonki - The KDE Crash Handler
 * 
 * toplevel.cpp
 *
 * Copyright (C) 2000 Hans Petter Bieker <bieker@kde.org>
 *****************************************************************/

#ifndef TOPLEVEL_H
#define TOPLEVEL_H

#include <kdialog.h>

class Toplevel : public KDialog
{
  Q_OBJECT

public:
  Toplevel(int signal, const QCString appname, QWidget *parent = 0, const char * name = 0);
  ~Toplevel();
};

#endif
