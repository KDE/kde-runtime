/* This file is part of the KDE project
   Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; version 2
   of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

//#include "image_plugin.moc"

#include <kdebug.h>
#include <kgenericfactory.h>
#include <kiconloader.h>
#include <renamedlgplugin.h>
#include <qlabel.h>
#include <qdialog.h>
#include <qwidget.h>
#include <qstringlist.h>


class KTestMenu : public RenameDlgPlugin{
public:
  KTestMenu( QDialog *dialog, const char *name, const QStringList & );

};

KTestMenu::KTestMenu( QDialog *dialog, const char *name, const QStringList &list ) : RenameDlgPlugin( dialog, name, list) {
  QLabel *label = new QLabel(this );
  label->setText("Yuhu" );
  qWarning("hidi ho");
};

typedef KGenericFactory<KTestMenu, QDialog> KImageFactory;
K_EXPORT_COMPONENT_FACTORY( librenimageplugin, KImageFactory );
