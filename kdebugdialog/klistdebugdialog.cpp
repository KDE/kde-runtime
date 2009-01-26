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
#include <ktreewidgetsearchline.h>

#include <QtDBus/QtDBus>
#include <QLayout>
#include <QTreeWidget>
#include <QPushButton>

KListDebugDialog::KListDebugDialog( QStringList areaList, QWidget *parent, const char *name, bool modal )
  : KAbstractDebugDialog( parent, name, modal ),
  m_areaList( areaList )
{
  setCaption(i18n("Debug Settings"));
  QWidget* mainWidget = new QWidget( this );
  QVBoxLayout *lay = new QVBoxLayout( mainWidget );
  lay->setMargin( KDialog::marginHint() );
  lay->setSpacing( KDialog::spacingHint() );

  m_incrSearch = new KTreeWidgetSearchLineWidget();
  m_incrSearch->searchLine()->setClearButtonShown(true);
  lay->addWidget( m_incrSearch );
//  connect( m_incrSearch, SIGNAL( textChanged( const QString& ) ),
//           SLOT( filterCheckBoxes( const QString& ) ) );

  //@TODO: Change back to QListWidget once Trolltech fixed the task: #214420
  //See http://trolltech.com/developer/task-tracker/index_html?id=214420&method=entry
  m_areaWidget = new QTreeWidget();
  m_areaWidget->setHeaderHidden(true);
  m_areaWidget->setItemsExpandable(false);
  m_areaWidget->setRootIsDecorated(false);
  m_areaWidget->setUniformRowHeights(true);
  lay->addWidget(m_areaWidget);

  m_incrSearch->searchLine()->addTreeWidget(m_areaWidget);

  generateCheckBoxes();

  QHBoxLayout* selectButs = new QHBoxLayout();
  lay->addLayout( selectButs );
  QPushButton* all = new QPushButton( i18n("&Select All"));
  QPushButton* none = new QPushButton( i18n("&Deselect All"));
  selectButs->addWidget( all );
  selectButs->addWidget( none );

  connect( all, SIGNAL( clicked() ), this, SLOT( selectAll() ) );
  connect( none, SIGNAL( clicked() ), this, SLOT( deSelectAll() ) );

  buildButtons( lay );
  resize( 350, 400 );
  setMainWidget( mainWidget );
  setButtons( KDialog::NoDefault );
}

void KListDebugDialog::generateCheckBoxes()
{
  foreach (const QString& area, m_areaList) {
    QString data = area.simplified();
    int space = data.indexOf(" ");
    if (space == -1)
      kError() << "No space:" << data << endl;

    QTreeWidgetItem* item = new QTreeWidgetItem(m_areaWidget, QStringList() << data);
    item->setData(0, Qt::UserRole, data.left(space).toLatin1());
  }

  load();
}

void KListDebugDialog::selectAll()
{
  for (int i = 0; i < m_areaWidget->topLevelItemCount(); ++i) {
    QTreeWidgetItem* item = m_areaWidget->topLevelItem(i);
    if (!m_areaWidget->isItemHidden(item)) {
      item->setCheckState(0, Qt::Checked);
    }
  }
}

void KListDebugDialog::deSelectAll()
{
  for (int i = 0; i < m_areaWidget->topLevelItemCount(); ++i) {
    QTreeWidgetItem* item = m_areaWidget->topLevelItem(i);
    if (!m_areaWidget->isItemHidden(item)) {
      item->setCheckState(0, Qt::Unchecked);
    }
  }
}

void KListDebugDialog::load()
{
  for (int i = 0; i < m_areaWidget->topLevelItemCount(); ++i) {
    QTreeWidgetItem* item = m_areaWidget->topLevelItem(i);
    KConfigGroup group = pConfig->group( item->data(0, Qt::UserRole).toByteArray() ); // Group name = debug area code = cb's name

    int setting = group.readEntry("InfoOutput", -1);

    switch (setting) {
      case 4: // off
        item->setCheckState(0, Qt::Unchecked);
        break;
      case 2: //shell
        item->setCheckState(0, Qt::Checked);
        break;
      case 3: //syslog
      case 1: //msgbox
      case 0: //file
      default:
        item->setCheckState(0, Qt::PartiallyChecked);
        /////// Uses the triState capability of checkboxes
        break;
    }
  }
}

void KListDebugDialog::save()
{
  for (int i = 0; i < m_areaWidget->topLevelItemCount(); ++i) {
    QTreeWidgetItem* item = m_areaWidget->topLevelItem(i);
    KConfigGroup group = pConfig->group( item->data(0, Qt::UserRole).toByteArray() ); // Group name = debug area code = cb's name
    if (item->checkState(0) != Qt::PartiallyChecked)
    {
      int setting = (item->checkState(0) == Qt::Checked) ? 2 : 4;
      group.writeEntry( "InfoOutput", setting );
    }
  }
  //sync done by main.cpp

  // send DBus message to all clients
  QDBusMessage msg = QDBusMessage::createSignal("/", "org.kde.KDebug", "configChanged" );
  if (!QDBusConnection::sessionBus().send(msg))
  {
    kError() << "Unable to send D-BUS message" << endl;
  }
}

void KListDebugDialog::activateArea( QByteArray area, bool activate )
{
  foreach(QTreeWidgetItem* item, m_areaWidget->findItems(area, Qt::MatchContains)) {
    item->setCheckState( 0, activate ? Qt::Checked : Qt::Unchecked );
    return;
  }
}

#include "klistdebugdialog.moc"
