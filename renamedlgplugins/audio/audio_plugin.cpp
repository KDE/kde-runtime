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

#include <kgenericfactory.h>
#include <kio/renamedlgplugin.h>
#include <qlabel.h>
#include <qdialog.h>
#include <qwidget.h>
#include <qstringlist.h>
//Added by qt3to4:
#include <QGridLayout>
#include <kio/global.h>
#include <qlayout.h>

#include <sys/types.h>

#include "audiopreview.h"

class AudioPlugin : public KIO::RenameDlgPlugin{
public:
  AudioPlugin( QDialog *dialog, const QStringList & );
  ~AudioPlugin();

  bool wantToHandle( KIO::RenameDlg_Mode mode, const KIO::RenameDlgPlugin::FileItem& src,
                     const KIO::RenameDlgPlugin::FileItem& dst ) const;
  void handle( KIO::RenameDlg_Mode, const KIO::RenameDlgPlugin::FileItem& src,
               const KIO::RenameDlgPlugin::FileItem& dst );
};

AudioPlugin::AudioPlugin( QDialog *dialog, const QStringList & ) : RenameDlgPlugin( dialog) {
  qWarning("loaded" );
}
AudioPlugin::~AudioPlugin()
{
}

bool AudioPlugin::wantToHandle( KIO::RenameDlg_Mode, const KIO::RenameDlgPlugin::FileItem&,
                                const KIO::RenameDlgPlugin::FileItem& ) const {
    return true;
}

void AudioPlugin::handle( KIO::RenameDlg_Mode mode, const KIO::RenameDlgPlugin::FileItem& src,
                          const KIO::RenameDlgPlugin::FileItem& dst ) {
 QGridLayout *lay = new QGridLayout( this );
 if( mode & KIO::M_OVERWRITE ){
   QLabel *label_head = new QLabel(this);
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
   label_head->setText(sentence1);
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

typedef KGenericFactory<AudioPlugin, QDialog> AudioPluginFactory;
K_EXPORT_COMPONENT_FACTORY( librenaudioplugin, AudioPluginFactory("audiorename_plugin") )
