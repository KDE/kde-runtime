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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef imagevisualizer_h
#define imagevisualizer_h

#include <q3vbox.h>
//Added by qt3to4:
#include <QPixmap>
#include <QLabel>

class QPixmap;
class QLabel;
class ImageVisualizer : public Q3VBox
{
  Q_OBJECT
public:
  ImageVisualizer(QWidget *parent, const char *name, const QString &fileName );

private:
  void loadImage( const QString& path );

private slots:
  void downloadImage( const QString& url );

private:
  QLabel *pic;
  QLabel *description;
};
#endif























