/* This file is part of the KDE libraries
    Copyright (C) 2000 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KLISTDEBUGDIALOG__H
#define KLISTDEBUGDIALOG__H

#include "kabstractdebugdialog.h"
#include <qcheckbox.h>
#include <qptrlist.h>
#include <qstringlist.h>

class QVBox;
class KLineEdit;

/**
 * Control debug output of KDE applications
 * This dialog offers a reduced functionality compared to the full KDebugDialog
 * class, but allows to set debug output on or off to several areas much more easily.
 *
 * @author David Faure <faure@kde.org>
 */
class KListDebugDialog : public KAbstractDebugDialog
{
  Q_OBJECT

public:
  KListDebugDialog( QStringList areaList, QWidget *parent=0, const char *name=0, bool modal=true );
  virtual ~KListDebugDialog() {}

  void activateArea( QCString area, bool activate );

  virtual void save();

protected slots:
  void selectAll();
  void deSelectAll();

  void generateCheckBoxes( const QString& filter );

private:
  void load();
  QPtrList<QCheckBox> boxes;
  QStringList m_areaList;
  QVBox *m_box;
  KLineEdit *m_incrSearch;
  QMap<QCString, int> m_changes;
};

#endif
