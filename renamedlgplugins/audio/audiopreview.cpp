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

#include <kfilemetainfo.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kurl.h>

#include <qlabel.h>
#include <qpixmap.h>
#include <kio/netaccess.h>
#include <kurllabel.h>
#include <kmimetype.h>
#include <kmediaplayer/player.h>
#include <kparts/componentfactory.h>

#include "audiopreview.h"

AudioPreview::AudioPreview( QWidget *parent, const QString &fileName, const QString &mimeType)
  : KVBox( parent )
{
  m_isTempFile = false;
  pic = 0;
  m_player = 0L;
  description = 0;
  // fileName is created by KUrl::prettyURL()
  KUrl url( fileName );
  setSpacing( 0 );
  if( url.isValid() && url.isLocalFile() ) {
    m_localFile = url.path();
    pic = new QLabel(this);
    pic->setPixmap(KMimeType::pixmapForURL( url ));
    pic->adjustSize();
    description = new QLabel(this);
    initView( mimeType );
  } else if( !url.isLocalFile() ) {
    KUrlLabel *label = new KUrlLabel( this );
    label->setText(i18n("This audio file isn't stored\non the local host.\nClick on this label to load it.\n" ) );
    label->setURL( url.prettyURL() );
    connect(label, SIGNAL(leftClickedURL(const QString&)), SLOT(downloadFile(const QString&)));
    pic = label;
    description = new QLabel(this);
    description->adjustSize( );
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
  KUrl url = KUrl::fromPathOrURL( m_localFile );
  pic->setText( QString::null );
  pic->setPixmap(KMimeType::pixmapForURL( url ));
  pic->adjustSize();
 
  KFileMetaInfo info(m_localFile);
  KMimeType::Ptr mimeptr = KMimeType::mimeType(mimeType);

  QString desc;
  if (info.isValid()) 
  {
    if (mimeptr->is("audio/x-mp3") || mimeptr->is("application/ogg"))
    {
      desc.append(i18n("Artist: %1\n", info.item("Artist").value().toString() ));
      desc.append(i18n("Title: %1\n", info.item("Title").value().toString() ));
      desc.append(i18n("Comment: %1\n", info.item("Comment").value().toString() ));
      desc.append(i18nc("Biterate: 160 kbits/s", "Bitrate: %1 %2\n", info.item("Bitrate").value().toString(), info.item("Bitrate").suffix() ));
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
 
  description->setText( desc );
  description->adjustSize();
  m_player = KParts::ComponentFactory::createInstanceFromQuery<KMediaPlayer::Player>( "KMediaPlayer/Player", QString::null, this );
  if ( m_player )
  {
    static_cast<KParts::ReadOnlyPart*>(m_player)->openURL( url );
    m_player->widget()->show();
  }
}

void AudioPreview::downloadFile( const QString& url )
{
  if( KIO::NetAccess::download( KUrl::fromPathOrURL( url ), m_localFile , topLevelWidget()) )
  {
    m_isTempFile = true;
    initView( KMimeType::findByPath( m_localFile )->name() );
  }
}

#include <audiopreview.moc>
#include <kvbox.h>
