/* This file is part of the KDE project
   Copyright (C) 2003 Fabian Wolf <fabianw@gmx.net>

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

#include <kfilemetainfo.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kurl.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qpixmap.h>
#include "audiopreview.h"

AudioPreview::~AudioPreview()
{

}
AudioPreview::AudioPreview( QWidget *parent, const char *name, const QString &fileName, const QString &mimeType)
  : QVBox( parent, name )
{
  KIconLoader icon;
  pic = 0;
  description = 0;
  KURL url( fileName );
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
    KFileMetaInfo info(path);
    pic = new QLabel(this);
    pic->setPixmap(icon.loadIcon("mime_audio", KIcon::Desktop, 64));
    pic->adjustSize();
    QString desc;
    if (mimeType == "audio/x-mp3" || mimeType == "audio/x-ogg"){
      desc.append(i18n("Artist: ") + info.item("Artist").value().toString() + "\n" );
      desc.append(i18n("Title: ") + info.item("Title").value().toString() + "\n" );
      desc.append(i18n("Comment: ") + info.item("Comment").value().toString() + "\n" );
      desc.append(i18n("Bitrate: ") + info.item("Bitrate").value().toString() + info.item("Bitrate").suffix() + "\n" );
    }
    desc.append(i18n("Sample Rate: ") + info.item("Sample Rate").value().toString() + info.item("Sample Rate").suffix() + "\n" );
    desc.append(i18n("Length: "));
    /* Calculate length in mm:ss format */
    int length = info.item("Length").value().toInt();
    if (length/60 < 10)
      desc.append("0");
    desc.append(QString("%1:").arg(length/60, 0, 10));
    if (length%60 < 10)
      desc.append("0");
    desc.append(QString("%1\n").arg(length%60, 0, 10));
    description = new QLabel( this );
    description->setText(desc);
  } else if( !url.isLocalFile() ) {
    pic = new QLabel(this );
    description = new QLabel(this );
    /* This needs to implemented ... */
    description->setText(i18n("This audio file isn't stored\non the local host.\nClick on this label to load it.\n" ) );
      description->adjustSize( );
  } else {
    description = new QLabel(this );
    description->setText(i18n("Unable to load audio file") );
  }
}

