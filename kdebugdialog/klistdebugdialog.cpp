/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/


#include "klistdebugdialog.h"
#include <kconfig.h>
#include <kapplication.h>
#include <kdebug.h>
#include <qlayout.h>
#include <qscrollview.h>
#include <qvbox.h>
#include <klocale.h>
#include <qpushbutton.h>
#include <klineedit.h>
#include <dcopclient.h>

KListDebugDialog::KListDebugDialog( QStringList areaList, QWidget *parent, const char *name, bool modal )
  : KAbstractDebugDialog( parent, name, modal ),
  m_areaList( areaList )
{
  setCaption(i18n("Debug Settings"));

  QVBoxLayout *lay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  m_incrSearch = new KLineEdit( this );
  lay->addWidget( m_incrSearch );
  connect( m_incrSearch, SIGNAL( textChanged( const QString& ) ),
           SLOT( generateCheckBoxes( const QString& ) ) );

  QScrollView * scrollView = new QScrollView( this );
  scrollView->setResizePolicy( QScrollView::AutoOneFit );
  lay->addWidget( scrollView );

  m_box = new QVBox( scrollView->viewport() );
  scrollView->addChild( m_box );

  generateCheckBoxes( QString::null );

  QHBoxLayout* selectButs = new QHBoxLayout( lay );
  QPushButton* all = new QPushButton( i18n("&Select All"), this );
  QPushButton* none = new QPushButton( i18n("&Deselect All"), this );
  selectButs->addWidget( all );
  selectButs->addWidget( none );

  connect( all, SIGNAL( clicked() ), this, SLOT( selectAll() ) );
  connect( none, SIGNAL( clicked() ), this, SLOT( deSelectAll() ) );

  buildButtons( lay );
  resize( 350, 400 );
}

void KListDebugDialog::generateCheckBoxes( const QString& filter )
{
  QPtrListIterator<QCheckBox> cb_it ( boxes );
  for( ; cb_it.current() ; ++cb_it )
  {
    if( (*cb_it)->state() != QButton::NoChange )
      m_changes.insert( (*cb_it)->name(), (*cb_it)->isChecked() ? 2 : 4 );
  }

  boxes.setAutoDelete( true );
  boxes.clear();
  boxes.setAutoDelete( false );

  QWidget* taborder = m_incrSearch;
  QStringList::Iterator it = m_areaList.begin();
  for ( ; it != m_areaList.end() ; ++it )
  {
    QString data = (*it).simplifyWhiteSpace();
    if ( filter.isEmpty() || data.lower().contains( filter.lower() ) )
    {
      int space = data.find(" ");
      if (space == -1)
        kdError() << "No space:" << data << endl;

      QString areaNumber = data.left(space);
      //kdDebug() << areaNumber << endl;
      QCheckBox * cb = new QCheckBox( data, m_box, areaNumber.latin1() );
      cb->show();
      boxes.append( cb );
      setTabOrder( taborder, cb );
      taborder = cb;
    }
  }

  load();
}

void KListDebugDialog::selectAll()
{
  QPtrListIterator<QCheckBox> it ( boxes );
  for ( ; it.current() ; ++it ) {
    (*it)->setChecked( true );
    m_changes.insert( (*it)->name(), 2 );
  }
}

void KListDebugDialog::deSelectAll()
{
  QPtrListIterator<QCheckBox> it ( boxes );
  for ( ; it.current() ; ++it ) {
    (*it)->setChecked( false );
    m_changes.insert( (*it)->name(), 4 );
  }
}

void KListDebugDialog::load()
{
  QPtrListIterator<QCheckBox> it ( boxes );
  for ( ; it.current() ; ++it )
  {
      pConfig->setGroup( (*it)->name() ); // Group name = debug area code = cb's name

      int setting = pConfig->readNumEntry( "InfoOutput", 2 );
      // override setting if in m_changes
      if( m_changes.find( (*it)->name() ) != m_changes.end() ) {
        setting = m_changes[ (*it)->name() ];
      }

      switch (setting) {
        case 4: // off
          (*it)->setChecked(false);
          break;
        case 2: //shell
          (*it)->setChecked(true);
          break;
        case 3: //syslog
        case 1: //msgbox
        case 0: //file
        default:
          (*it)->setNoChange();
          /////// Uses the triState capability of checkboxes
          ////// Note: it seems some styles don't draw that correctly (BUG)
          break;
      }
  }
}

void KListDebugDialog::save()
{
  QPtrListIterator<QCheckBox> it ( boxes );
  for ( ; it.current() ; ++it )
  {
      pConfig->setGroup( (*it)->name() ); // Group name = debug area code = cb's name
      if ( (*it)->state() != QButton::NoChange )
      {
          int setting = (*it)->isChecked() ? 2 : 4;
          pConfig->writeEntry( "InfoOutput", setting );
      }
  }
  //sync done by main.cpp

  // send DCOP message to all clients
  QByteArray data;
  if (!kapp->dcopClient()->send("*", "KDebug", "notifyKDebugConfigChanged()", data))
  {
    kdError() << "Unable to send DCOP message" << endl;
  }

  m_changes.clear();
}

void KListDebugDialog::activateArea( QCString area, bool activate )
{
  QPtrListIterator<QCheckBox> it ( boxes );
  for ( ; it.current() ; ++it )
  {
      if ( area == (*it)->name()  // debug area code = cb's name
          || (*it)->text().find( QString::fromLatin1(area) ) != -1 ) // area name included in cb text
      {
          (*it)->setChecked( activate );
          return;
      }
  }
}

#include "klistdebugdialog.moc"
