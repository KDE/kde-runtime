/**
 * main.h
 *
 * Copyright (c) 2000 Torsten Rahn <torsten@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __ICONS_H__
#define __ICONS_H__

#include <qtabwidget.h>
#include <kcmodule.h>
#include "icngeneral.h"
#include "icnlabel.h"

class KIconConfigMain : public KCModule
{
  Q_OBJECT

public:
  KIconConfigMain(QWidget *parent = 0L, const char *name = 0L);
  
  void load();
  void save();
  void defaults();
  
  QString quickHelp();
  
protected slots:
  void moduleChanged(bool state);
      
private:
 QTabWidget *tab;

 KIconConfigLabel *label;
 KIconConfigGeneral *general;

};

#endif
