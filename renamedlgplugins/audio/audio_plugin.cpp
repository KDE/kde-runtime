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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <kpluginfactory.h>
#include <klocale.h>
#include <ksqueezedtextlabel.h>
#include <kio/renamedialogplugin.h>
#include <qlabel.h>
#include <qdialog.h>
#include <qwidget.h>
#include <qstringlist.h>
#include <QGridLayout>
#include <kdebug.h>
#include <kio/global.h>

#include <sys/types.h>

#include "audiopreview.h"

class AudioPlugin : public KIO::RenameDialogPlugin{
public:
  AudioPlugin( QWidget *dialog, const QVariantList & );
  ~AudioPlugin();

  bool wantToHandle( KIO::RenameDialog_Mode mode, const KIO::RenameDialogPlugin::FileItem& src,
                     const KIO::RenameDialogPlugin::FileItem& dst ) const;
  void handle( KIO::RenameDialog_Mode, const KIO::RenameDialogPlugin::FileItem& src,
               const KIO::RenameDialogPlugin::FileItem& dst );
};

AudioPlugin::AudioPlugin( QWidget *dialog, const QVariantList & )
    : RenameDialogPlugin(static_cast<QDialog*>(dialog)) {
    kDebug() << "loaded";
}
AudioPlugin::~AudioPlugin()
{
}

bool AudioPlugin::wantToHandle( KIO::RenameDialog_Mode, const KIO::RenameDialogPlugin::FileItem&,
                                const KIO::RenameDialogPlugin::FileItem& ) const {
    return true;
}

void AudioPlugin::handle( KIO::RenameDialog_Mode mode, const KIO::RenameDialogPlugin::FileItem& src,
                          const KIO::RenameDialogPlugin::FileItem& dst ) {

 QGridLayout *lay = new QGridLayout( this );
 if( mode & KIO::M_OVERWRITE ){
   QLabel *label_src  = new QLabel(this);
   QLabel *label_dst  = new QLabel(this);
   QLabel *label_ask  = new QLabel(this);

   QString sentence1;
   QString dest = dst.url().pathOrUrl();
   if ( src.mTime() < dst.mTime() )
      sentence1 = i18n("An older file named '%1' already exists.\n", dest);
   else if ( src.mTime() == dst.mTime() )
      sentence1 = i18n("A similar file named '%1' already exists.\n", dest);
   else
      sentence1 = i18n("A newer file named '%1' already exists.\n", dest);
   QLabel *label_head = new KSqueezedTextLabel(sentence1, this);
   label_src->setText(i18n("Source File"));
   label_dst->setText(i18n("Existing File"));
   label_ask->setText(i18n("Would you like to replace the existing file with the one on the right?") );
   label_head->adjustSize();
   label_src->adjustSize();
   label_dst->adjustSize();
   label_ask->adjustSize();
   lay->addWidget(label_head, 0, 0, 1, 3, Qt::AlignLeft);
   lay->addWidget(label_dst, 1, 0, Qt::AlignLeft);
   lay->addWidget(label_src, 1, 2, Qt::AlignLeft);
   lay->addWidget(label_ask, 3, 0, 1, 3,  Qt::AlignLeft);
   adjustSize();
 }
 AudioPreview *left= new AudioPreview(this,  dst.url(), dst.mimeType() );
 AudioPreview *right = new AudioPreview( this, src.url(), src.mimeType() );
 lay->addWidget(left, 2, 0 );
 lay->addWidget(right, 2, 2 );
 adjustSize();
}

K_PLUGIN_FACTORY(AudioPluginFactory,
                         registerPlugin<AudioPlugin>();)
K_EXPORT_PLUGIN(AudioPluginFactory("audiorename_plugin"))

