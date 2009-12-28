/* This file is part of the KDE project
   Copyright (C) 2002 Holger Freyther <freyther@kde.org>
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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "imagevisualizer.h"

#include <qlabel.h>
#include <qpixmap.h>
#include <qimage.h>

#include <klocale.h>
#include <kurl.h>
#include <kurllabel.h>
#include <kio/netaccess.h>
#include <kvbox.h>

ImageVisualizer::ImageVisualizer( QWidget *parent, const KUrl &url )
  : KVBox( parent )
{
  pic = 0;
  description = 0;
  setSpacing( 0 );
  if( url.isValid() && url.isLocalFile() ) {
    pic = new QLabel(this );
    description = new QLabel( this );
    loadImage( url.toLocalFile() );
  } else if( !url.isLocalFile() ) {
    KUrlLabel *label = new KUrlLabel( this );
    label->setText(i18n("This picture is not stored\non the local host.\nClick on this label to load it.\n" ) );
    label->setUrl( url.prettyUrl() );
    connect(label, SIGNAL(leftClickedUrl(const QString&)), SLOT(downloadImage(const QString&)));
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
  QPixmap pix(path);
  QPixmap pixmap(pix.scaled(180, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation) );
  pic->setText( QString() );
  pic->setPixmap(pixmap );
  pic->adjustSize();

  QString desc;
  desc.append(i18nc("The color depth of an image", "Depth: %1\n", pix.depth() ));
  desc.append(i18nc("The dimensions of an image", "Dimensions: %1x%2", pix.width(), pix.height() ));
  description->setText(desc );
  description->adjustSize();
}

void ImageVisualizer::downloadImage(const QString& url)
{
  QString tmpFile;
  if( KIO::NetAccess::download( KUrl( url ), tmpFile, window()) )
  {
    loadImage( tmpFile );
    KIO::NetAccess::removeTempFile( tmpFile );
  }
}

#include "imagevisualizer.moc"
