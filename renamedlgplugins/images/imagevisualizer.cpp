/* This file is part of the KDE project
   Copyright (C) 2002 Holger Freyther <freyther@yahoo.com>

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

#include <klocale.h>
#include <kurl.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qimage.h>
#include "imagevisualizer.h"


ImageVisualizer::~ImageVisualizer()
{

}
/** ok I dunno if I'm tto stupid ...
 * KURL is giving me just a fileName no absolute Path so I need my hack
 */
ImageVisualizer::ImageVisualizer( QWidget *parent, const char *name, const QString &fileName )
  : QVBox( parent, name )
{
  qWarning("filename %s", fileName.latin1()  );
  pixmap =0;
  pic = 0;
  description = 0;
  KURL url( fileName );
  qWarning("filename %s", url.filename().latin1() );
  qWarning("filename %s", url.fileName().latin1() );
  setSpacing( 0 );
  if( url.isValid() && url.isLocalFile() ) {
    QString path;
    if( QString::fromLatin1("file://") == fileName.left(7) ){
      qWarning("file://" );
      path = fileName.mid(6);
    }else if(QString::fromLatin1("file:/") == fileName.left(6) ) {
      path = fileName.mid(5);
    }else {
      path = fileName;
    }
    qWarning("path: %s", path.latin1() );
    QImage img(path );
    pixmap = new QPixmap(img.smoothScale(180,200, QImage::ScaleMin) );
    pic = new QLabel(this );
    pic->setPixmap(*pixmap );
    pic->adjustSize();
    QString desc;
    desc.append(i18n("Depth: ") + QString::number(img.depth() ) + "\n" );
    desc.append(i18n("Dimensions: ") + QString::number(img.width() ) + "x" + QString::number(img.height() ) );
    description = new QLabel( this );
    description->setText(desc );
  } else if( !url.isLocalFile() ) {
    pic = new QLabel(this );
    description = new QLabel(this );
    description->setText(i18n("This picture isn't stored\non the local host.\nClick on this label to load it.\n" ) );
      description->adjustSize( );
  } else {
    description = new QLabel(this );
    description->setText(i18n("Sorry couldn't load the image") );
  }
}

