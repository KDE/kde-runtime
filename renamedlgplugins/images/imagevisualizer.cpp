/* This file is part of the KDE project
   Copyright (C) 2002 Holger Freyther <freyther@yahoo.com>
                 2003 Carsten Pfeiffer <pfeiffer@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <klocale.h>
#include <kurl.h>
#include <kurllabel.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qimage.h>

#include <kio/netaccess.h>

#include "imagevisualizer.h"

ImageVisualizer::ImageVisualizer( QWidget *parent, const char *name, const QString &fileName )
  : QVBox( parent, name )
{
  pic = 0;
  description = 0;
  KURL url=KURL::fromPathOrURL( fileName );
  setSpacing( 0 );
  if( url.isValid() && url.isLocalFile() ) {
    pic = new QLabel(this );
    description = new QLabel( this );
    loadImage( url.path() );
  } else if( !url.isLocalFile() ) {
    KURLLabel *label = new KURLLabel( this );
    label->setText(i18n("This picture isn't stored\non the local host.\nClick on this label to load it.\n" ) );
    label->setURL( url.prettyURL() );
    connect(label, SIGNAL(leftClickedURL(const QString&)), SLOT(downloadImage(const QString&)));
    pic = label;
    description = new QLabel(this);
    description->adjustSize( );
  } else {
    description = new QLabel(this );
    description->setText(i18n("Unable to load image") );
  }
}

void ImageVisualizer::loadImage( const QString& path )
{
  QImage img(path);
  QPixmap pixmap(img.smoothScale(180,200, QImage::ScaleMin) );
  pic->setText( QString::null );
  pic->setPixmap(pixmap );
  pic->adjustSize(); 

  QString desc;
  desc.append(i18n("The color depth of an image", "Depth: %1\n").arg( img.depth() ));
  desc.append(i18n("The dimensions of an image", "Dimensions: %1x%1").arg(img.width()).arg(img.height() ));
  description->setText(desc );
  description->adjustSize();
}

void ImageVisualizer::downloadImage(const QString& url)
{
  QString tmpFile;
  if( KIO::NetAccess::download( KURL::fromPathOrURL( url ), tmpFile , topLevelWidget()) )
  {
    loadImage( tmpFile );
    KIO::NetAccess::removeTempFile( tmpFile );
  }
}

#include "imagevisualizer.moc"
