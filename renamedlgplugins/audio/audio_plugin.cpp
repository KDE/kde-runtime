/* This file is part of the KDE project
   Copyright (C) 2003 Fabian Wolf <fabianw@gmx.net>

   image_plugin.cpp (also Part of the KDE Project) used as template

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

#include <kgenericfactory.h>
#include <renamedlgplugin.h>
#include <kio/renamedlg.h>
#include <qlabel.h>
#include <qdialog.h>
#include <qwidget.h>
#include <qstringlist.h>
#include <kio/global.h>
#include <qlayout.h>

#include <sys/types.h>

#include "audiopreview.h"

class KTestMenu : public RenameDlgPlugin{
public:
  KTestMenu( QDialog *dialog, const char *name, const QStringList & );
  ~KTestMenu();
  virtual bool initialize( KIO::RenameDlg_Mode /*mod*/, const QString &/*_src*/, const QString &/*_dest*/,
		  const QString &/*mimeSrc*/,
		  const QString &/*mimeDest*/,
		  KIO::filesize_t /*sizeSrc*/,
		  KIO::filesize_t /*sizeDest*/,
		  time_t /*ctimeSrc*/,
		  time_t /*ctimeDest*/,
		  time_t /*mtimeSrc*/,
		  time_t /*mtimeDest*/ );
};

KTestMenu::KTestMenu( QDialog *dialog, const char *name, const QStringList &list ) : RenameDlgPlugin( dialog, name, list) {
  qWarning("loaded" );  
};
KTestMenu::~KTestMenu()
{
}
bool KTestMenu::initialize( KIO::RenameDlg_Mode mode, const QString &_src, const QString &_dest,
		  const QString &mimeSrc,
		  const QString &mimeDest,
		  KIO::filesize_t sizeSrc,
		  KIO::filesize_t sizeDest,
		  time_t ctimeSrc,
		  time_t ctimeDest,
		  time_t mtimeSrc,
                  time_t mtimeDest ) {
 QGridLayout *lay = new QGridLayout(this, 4, 3, 5);
 if( mode & KIO::M_OVERWRITE ){
   QLabel *label_head = new QLabel(this);
   QLabel *label_src  = new QLabel(this);
   QLabel *label_dst  = new QLabel(this);
   QLabel *label_ask  = new QLabel(this);

   QString sentence1;
   if (mtimeDest < mtimeSrc)
      sentence1 = i18n("An older file named '%1' already exists.\n").arg(_dest);
   else if (mtimeDest == mtimeSrc)
      sentence1 = i18n("A similar file named '%1' already exists.\n").arg(_dest);
   else
      sentence1 = i18n("A newer file named '%1' already exists.\n").arg(_dest);
   label_head->setText(sentence1);
   label_src->setText(i18n("Source File"));
   label_dst->setText(i18n("Existing File"));
   label_ask->setText(i18n("Would you like to replace the existing file with the one on the right?") );
   label_head->adjustSize();
   label_src->adjustSize();
   label_dst->adjustSize();
   label_ask->adjustSize();
   lay->addMultiCellWidget(label_head, 0, 0, 0, 2, Qt::AlignLeft);
   lay->addWidget(label_dst, 1, 0, Qt::AlignLeft);
   lay->addWidget(label_src, 1, 2, Qt::AlignLeft);
   lay->addMultiCellWidget(label_ask, 3, 3, 0, 2,  Qt::AlignLeft);
   adjustSize();
 }
 AudioPreview *left= new AudioPreview(this, "Preview Left", _dest, mimeDest );
 AudioPreview *right = new AudioPreview( this, "Preview Right", _src, mimeSrc);
 lay->addWidget(left, 2, 0 );
 lay->addWidget(right, 2, 2 );
 adjustSize();
 return true;
}

typedef KGenericFactory<KTestMenu, QDialog> KImageFactory;
K_EXPORT_COMPONENT_FACTORY( librenaudioplugin, KImageFactory("audiorename_plugin") );
