/*
  Copyright (c) 2001 Daniel Molkentin <molkentin@kde.org>
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
*/

#ifndef __moduleIface_h__
#define __moduleIface_h__

#include <dcopobject.h> 

#include <qfont.h>
#include <qpalette.h>
#include <qwidget.h>

class ModuleIface : public QObject, public DCOPObject {

Q_OBJECT
K_DCOP

public:
	ModuleIface(QObject *parent, const char *name);
	~ModuleIface();

k_dcop:
	QFont getFont();
	QPalette getPalette();
	QString getStyle();
	void invokeHelp();

signals:
	void helpClicked();

private:
	QWidget *_parent;

};

#endif
