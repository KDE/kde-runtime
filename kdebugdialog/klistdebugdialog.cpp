/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "klistdebugdialog.h"

#include <kconfig.h>
#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <klineedit.h>

#include <dbus/qdbus.h>
#include <QLayout>
#include <QListWidget>
#include <QPushButton>

KListDebugDialog::KListDebugDialog( QStringList areaList, QWidget *parent, const char *name, bool modal )
  : KAbstractDebugDialog( parent, name, modal ),
  m_areaList( areaList )
{
  setCaption(i18n("Debug Settings"));

  QVBoxLayout *lay = new QVBoxLayout( this );
  lay->setMargin( KDialog::marginHint() );
  lay->setSpacing( KDialog::spacingHint() );
  setLayout(lay);

  m_incrSearch = new KLineEdit( this );
  lay->addWidget( m_incrSearch );
  connect( m_incrSearch, SIGNAL( textChanged( const QString& ) ),
           SLOT( filterCheckBoxes( const QString& ) ) );

  m_areaWidget = new QListWidget(this);
  lay->addWidget(m_areaWidget);

  generateCheckBoxes();

  QHBoxLayout* selectButs = new QHBoxLayout();
  lay->addLayout( selectButs );
  QPushButton* all = new QPushButton( i18n("&Select All"), this );
  QPushButton* none = new QPushButton( i18n("&Deselect All"), this );
  selectButs->addWidget( all );
  selectButs->addWidget( none );

  connect( all, SIGNAL( clicked() ), this, SLOT( selectAll() ) );
  connect( none, SIGNAL( clicked() ), this, SLOT( deSelectAll() ) );

  buildButtons( lay );
  resize( 350, 400 );
}

void KListDebugDialog::filterCheckBoxes( const QString & filter )
{
  bool doFilter = filter.length();

  for (int i = 0; i < m_areaWidget->count(); ++i) {
    QListWidgetItem* item = m_areaWidget->item(i);
    m_areaWidget->setItemHidden(item, doFilter);
  }

  if (doFilter)
    foreach(QListWidgetItem* item, m_areaWidget->findItems(filter, Qt::MatchContains)) {
      m_areaWidget->setItemHidden(item, false);
    }
}

void KListDebugDialog::generateCheckBoxes()
{
  foreach (QString area, m_areaList) {
    QString data = area.simplified();
    int space = data.indexOf(" ");
    if (space == -1)
      kError() << "No space:" << data << endl;

    QListWidgetItem* item = new QListWidgetItem(data, m_areaWidget);
    item->setData(Qt::UserRole, data.left(space).toLatin1());
  }

  load();
}

void KListDebugDialog::selectAll()
{
  for (int i = 0; i < m_areaWidget->count(); ++i) {
    QListWidgetItem* item = m_areaWidget->item(i);
    if (!m_areaWidget->isItemHidden(item)) {
      item->setCheckState(Qt::Checked);
    }
  }
}

void KListDebugDialog::deSelectAll()
{
  for (int i = 0; i < m_areaWidget->count(); ++i) {
    QListWidgetItem* item = m_areaWidget->item(i);
    if (!m_areaWidget->isItemHidden(item)) {
      item->setCheckState(Qt::Unchecked);
    }
  }
}

void KListDebugDialog::load()
{
  for (int i = 0; i < m_areaWidget->count(); ++i) {
    QListWidgetItem* item = m_areaWidget->item(i);
    pConfig->setGroup( item->data(Qt::UserRole).toByteArray() ); // Group name = debug area code = cb's name

    int setting = pConfig->readEntry( "InfoOutput", 2 );

    switch (setting) {
      case 4: // off
        item->setCheckState(Qt::Unchecked);
        break;
      case 2: //shell
        item->setCheckState(Qt::Checked);
        break;
      case 3: //syslog
      case 1: //msgbox
      case 0: //file
      default:
        item->setCheckState(Qt::PartiallyChecked);
        /////// Uses the triState capability of checkboxes
        break;
    }
  }
}

void KListDebugDialog::save()
{
  for (int i = 0; i < m_areaWidget->count(); ++i) {
    QListWidgetItem* item = m_areaWidget->item(i);
    pConfig->setGroup( item->data(Qt::UserRole).toByteArray() ); // Group name = debug area code = cb's name
    if (item->checkState() != Qt::PartiallyChecked)
    {
      int setting = (item->checkState() == Qt::Checked) ? 2 : 4;
      pConfig->writeEntry( "InfoOutput", setting );
    }
  }
  //sync done by main.cpp

  // send DCOP message to all clients
  QDBusMessage msg = QDBusMessage::signal("/", "org.kde.KDebug", "configChanged");
  if (!QDBus::sessionBus().send(msg))
  {
    kError() << "Unable to send D-BUS message" << endl;
  }
}

void KListDebugDialog::activateArea( QByteArray area, bool activate )
{
  foreach(QListWidgetItem* item, m_areaWidget->findItems(area, Qt::MatchContains)) {
    item->setCheckState( activate ? Qt::Checked : Qt::Unchecked );
    return;
  }
}

#include "klistdebugdialog.moc"
