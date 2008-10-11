/*
   Copyright (C) 2002 Cornelius Schumacher <schumacher@kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef KCMCGI_H
#define KCMCGI_H

#include <kcmodule.h>

class QListWidget;
class QListWidgetItem;
class QPushButton;

class KConfig;

class KCMCgi : public KCModule
{
    Q_OBJECT
  public:
    KCMCgi(QWidget *parent, const QVariantList &args);
    ~KCMCgi();

    void load();
    void save();
    void defaults();
    QString quickHelp() const;

  public Q_SLOTS:

  protected Q_SLOTS:
    void addPath();
    void removePath();
    void slotItemSelected( QListWidgetItem * item );
  private:
    void updateButton();
    QListWidget *mListBox;
    QPushButton *mAddButton;
    QPushButton *mRemoveButton;

    KConfig *mConfig;
};

#endif
