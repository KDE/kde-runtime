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

#ifndef audiopreview_h
#define audiopreview_h

#include <kvbox.h>

namespace KMediaPlayer
{
  class Player;
}

class QLabel;
class KUrl;

class AudioPreview : public KVBox
{
  Q_OBJECT
public:
  AudioPreview(QWidget *parent, const KUrl &url, const QString &mimeType);
  ~AudioPreview();

private slots:
  void downloadFile(const QString& url);

private:
  void initView(const QString& mimeType);

  QLabel *pic;
  QLabel *description;
  QString m_localFile;
  bool m_isTempFile;

  KMediaPlayer::Player *m_player;
};
#endif























