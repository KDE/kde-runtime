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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "audiopreview.h"

#include <QLabel>
#include <QPixmap>

#include <kfilemetainfo.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kurl.h>
#include <kio/netaccess.h>
#include <kurllabel.h>
#include <kmediaplayer/player.h>
#include <kservicetypetrader.h>
#include <ksqueezedtextlabel.h>

AudioPreview::AudioPreview( QWidget *parent, const KUrl &url, const QString &mimeType)
  : KVBox( parent )
{
  m_isTempFile = false;
  pic = 0;
  m_player = 0L;
  description = 0;
  setSpacing( 0 );
  if( url.isValid() && url.isLocalFile() ) {
    m_localFile = url.toLocalFile();
    pic = new QLabel(this);
    pic->setPixmap(KIO::pixmapForUrl( url ));
    pic->adjustSize();
    initView( mimeType );
  } else if( !url.isLocalFile() ) {
    KUrlLabel *label = new KUrlLabel( this );
    label->setText(i18n("This audio file is not stored\non the local host.\nClick on this label to load it.\n" ) );
    label->setUrl( url.prettyUrl() );
    connect(label, SIGNAL(leftClickedUrl(const QString&)), SLOT(downloadFile(const QString&)));
    pic = label;
  } else {
    description = new QLabel(this );
    description->setText(i18n("Unable to load audio file") );
  }
}

AudioPreview::~AudioPreview()
{
  if ( m_isTempFile )
    KIO::NetAccess::removeTempFile( m_localFile );

  delete m_player;
}

void AudioPreview::initView( const QString& mimeType )
{
  KUrl url = KUrl::fromPath( m_localFile );
  pic->setText( QString() );
  pic->setPixmap(KIO::pixmapForUrl( url ));
  pic->adjustSize();

  KFileMetaInfo info(m_localFile);
  KMimeType::Ptr mimeptr = KMimeType::mimeType(mimeType);

  QString desc;
  if (info.isValid())
  {
    if (mimeptr->is("audio/mpeg") || mimeptr->is("application/ogg"))
    {
      // following 3 labels might be very long; make sure they get squeezed
      KSqueezedTextLabel *sl;

      sl = new KSqueezedTextLabel(this);
      sl->setText(i18n("Artist: %1", info.item("Artist").value().toString()));

      sl = new KSqueezedTextLabel(this);
      sl->setText(i18n("Title: %1", info.item("Title").value().toString()));

      sl = new KSqueezedTextLabel(this);
      sl->setText(i18n("Comment: %1", info.item("Comment").value().toString()));

      desc.append(i18nc("Bitrate: 160 kbits/s", "Bitrate: %1 %2\n", info.item("Bitrate").value().toString(), info.item("Bitrate").suffix() ));
    }
    desc.append(i18n("Sample rate: %1 %2\n", info.item("Sample Rate").value().toString(), info.item("Sample Rate").suffix() ));
    desc.append(i18n("Length: "));

    /* Calculate length in mm:ss format */
    int length = info.item("Length").value().toInt();
    if (length/60 < 10)
      desc.append("0");
    desc.append(QString("%1:").arg(length/60, 0, 10));
    if (length%60 < 10)
      desc.append("0");
    desc.append(QString("%1\n").arg(length%60, 0, 10));
  }

  description = new QLabel(this);
  description->setText( desc );
  description->adjustSize();
  m_player = KServiceTypeTrader::createInstanceFromQuery<KMediaPlayer::Player>( "KMediaPlayer/Player", QString(), this );
  if ( m_player )
  {
    static_cast<KParts::ReadOnlyPart*>(m_player)->openUrl( url );
    m_player->widget()->show();
  }
}

void AudioPreview::downloadFile( const QString& url )
{
  if( KIO::NetAccess::download( KUrl( url ), m_localFile , window()) )
  {
    m_isTempFile = true;
    initView( KMimeType::findByPath( m_localFile )->name() );
  }
}

#include <audiopreview.moc>
