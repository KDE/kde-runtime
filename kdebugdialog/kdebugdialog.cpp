/* This file is part of the KDE libraries
   Copyright (C) 1997 Matthias Kalle Dalheimer (kalle@kde.org)
   Copyright (C) 1999 David Faure (faure@kde.org)

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <QLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QPushButton>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdialog.h>
#include <kconfig.h>
#include <kseparator.h>
#include <kapplication.h>
#include <dcopclient.h>

#include "kdebugdialog.h"

KDebugDialog::KDebugDialog( QStringList areaList, QWidget *parent, const char *name, bool modal )
  : KAbstractDebugDialog( parent, name, modal )
{
  setCaption(i18n("Debug Settings"));

  QVBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setMargin( KDialog::marginHint() );
  topLayout->setSpacing( KDialog::spacingHint() );

  QLabel * tmpLabel = new QLabel( i18n("Debug area:"), this );
  tmpLabel->setFixedHeight( fontMetrics().lineSpacing() );
  topLayout->addWidget( tmpLabel );

  // Build combo of debug areas
  pDebugAreas = new QComboBox( this );
  pDebugAreas->setEditable( false );
  pDebugAreas->setFixedHeight( pDebugAreas->sizeHint().height() );
  pDebugAreas->addItems( areaList );
  topLayout->addWidget( pDebugAreas );

  QGridLayout *gbox = new QGridLayout();
  gbox->setMargin( KDialog::marginHint() );
  topLayout->addLayout( gbox );

  QStringList destList;
  destList.append( i18n("File") );
  destList.append( i18n("Message Box") );
  destList.append( i18n("Shell") );
  destList.append( i18n("Syslog") );
  destList.append( i18n("None") );

  //
  // Upper left frame
  //
  pInfoGroup = new QGroupBox( i18n("Information"), this );
  gbox->addWidget( pInfoGroup, 0, 0 );
  QVBoxLayout *vbox = new QVBoxLayout( pInfoGroup );
  vbox->setSpacing( KDialog::spacingHint() );
  vbox->addSpacing( fontMetrics().lineSpacing() );
  pInfoLabel1 = new QLabel( i18n("Output to:"), pInfoGroup );
  vbox->addWidget( pInfoLabel1 );
  pInfoCombo = new QComboBox( pInfoGroup );
  pInfoCombo->setEditable( false );
  connect(pInfoCombo, SIGNAL(activated(int)),
	  this, SLOT(slotDestinationChanged(int)));
  vbox->addWidget( pInfoCombo );
  pInfoCombo->addItems( destList );
  pInfoLabel2 = new QLabel( i18n("Filename:"), pInfoGroup );
  vbox->addWidget( pInfoLabel2 );
  pInfoFile = new QLineEdit( pInfoGroup );
  vbox->addWidget( pInfoFile );
  /*
  pInfoLabel3 = new QLabel( i18n("Show only area(s):"), pInfoGroup );
  vbox->addWidget( pInfoLabel3 );
  pInfoShow = new QLineEdit( pInfoGroup );
  vbox->addWidget( pInfoShow );
  */

  //
  // Upper right frame
  //
  pWarnGroup = new QGroupBox( i18n("Warning"), this );
  gbox->addWidget( pWarnGroup, 0, 1 );
  vbox = new QVBoxLayout( pWarnGroup );
  vbox->setSpacing( KDialog::spacingHint() );
  vbox->addSpacing( fontMetrics().lineSpacing() );
  pWarnLabel1 = new QLabel( i18n("Output to:"), pWarnGroup );
  vbox->addWidget( pWarnLabel1 );
  pWarnCombo = new QComboBox( pWarnGroup );
  pWarnCombo->setEditable( false );
  connect(pWarnCombo, SIGNAL(activated(int)),
	  this, SLOT(slotDestinationChanged(int)));
  vbox->addWidget( pWarnCombo );
  pWarnCombo->addItems( destList );
  pWarnLabel2 = new QLabel( i18n("Filename:"), pWarnGroup );
  vbox->addWidget( pWarnLabel2 );
  pWarnFile = new QLineEdit( pWarnGroup );
  vbox->addWidget( pWarnFile );
  /*
  pWarnLabel3 = new QLabel( i18n("Show only area(s):"), pWarnGroup );
  vbox->addWidget( pWarnLabel3 );
  pWarnShow = new QLineEdit( pWarnGroup );
  vbox->addWidget( pWarnShow );
  */

  //
  // Lower left frame
  //
  pErrorGroup = new QGroupBox( i18n("Error"), this );
  gbox->addWidget( pErrorGroup, 1, 0 );
  vbox = new QVBoxLayout( pErrorGroup );
  vbox->setSpacing( KDialog::spacingHint() );
  vbox->addSpacing( fontMetrics().lineSpacing() );
  pErrorLabel1 = new QLabel( i18n("Output to:"), pErrorGroup );
  vbox->addWidget( pErrorLabel1 );
  pErrorCombo = new QComboBox( pErrorGroup );
  pErrorCombo->setEditable( false );
  connect(pErrorCombo, SIGNAL(activated(int)),
	  this, SLOT(slotDestinationChanged(int)));
  vbox->addWidget( pErrorCombo );
  pErrorCombo->addItems( destList );
  pErrorLabel2 = new QLabel( i18n("Filename:"), pErrorGroup );
  vbox->addWidget( pErrorLabel2 );
  pErrorFile = new QLineEdit( pErrorGroup );
  vbox->addWidget( pErrorFile );
  /*
  pErrorLabel3 = new QLabel( i18n("Show only area(s):"), pErrorGroup );
  vbox->addWidget( pErrorLabel3 );
  pErrorShow = new QLineEdit( pErrorGroup );
  vbox->addWidget( pErrorShow );
  */

  //
  // Lower right frame
  //
  pFatalGroup = new QGroupBox( i18n("Fatal Error"), this );
  gbox->addWidget( pFatalGroup, 1, 1 );
  vbox = new QVBoxLayout( pFatalGroup );
  vbox->setSpacing( KDialog::spacingHint() );
  vbox->addSpacing( fontMetrics().lineSpacing() );
  pFatalLabel1 = new QLabel( i18n("Output to:"), pFatalGroup );
  vbox->addWidget( pFatalLabel1 );
  pFatalCombo = new QComboBox( pFatalGroup );
  pFatalCombo->setEditable( false );
  connect(pFatalCombo, SIGNAL(activated(int)),
	  this, SLOT(slotDestinationChanged(int)));
  vbox->addWidget( pFatalCombo );
  pFatalCombo->addItems( destList );
  pFatalLabel2 = new QLabel( i18n("Filename:"), pFatalGroup );
  vbox->addWidget( pFatalLabel2 );
  pFatalFile = new QLineEdit( pFatalGroup );
  vbox->addWidget( pFatalFile );
  /*
  pFatalLabel3 = new QLabel( i18n("Show only area(s):"), pFatalGroup );
  vbox->addWidget( pFatalLabel3 );
  pFatalShow = new QLineEdit( pFatalGroup );
  vbox->addWidget( pFatalShow );
  */


  pAbortFatal = new QCheckBox( i18n("Abort on fatal errors"), this );
  topLayout->addWidget(pAbortFatal);

  topLayout->addStretch();
  KSeparator *hline = new KSeparator( Qt::Horizontal, this );
  topLayout->addWidget( hline );

  buildButtons( topLayout );

  connect( pDebugAreas, SIGNAL( activated( const QString &) ),
           SLOT( slotDebugAreaChanged( const QString & ) ) );

  // Get initial values ("initial" is understood by the slot)
  slotDebugAreaChanged( "0 initial" );
  slotDestinationChanged(0);

  resize( 300, height() );
}

KDebugDialog::~KDebugDialog()
{
}

void KDebugDialog::slotDebugAreaChanged( const QString & text )
{
  // Save settings from previous page
  if ( text != "0 initial" ) // except on first call
    save();

  QString data = text.simplified();
  int space = data.indexOf(" ");
  if (space == -1)
      kError() << "No space:" << data << endl;

  bool longOK;
  unsigned long number = data.left(space).toULong(&longOK);
  if (!longOK)
      kError() << "The first part wasn't a number : " << data << endl;

  /* Fill dialog fields with values from config data */
  pConfig->setGroup( QString::number( number ) ); // Group name = debug area code
  pInfoCombo->setCurrentIndex( pConfig->readEntry( "InfoOutput", 2 ) );
  pInfoFile->setText( pConfig->readPathEntry( "InfoFilename","kdebug.dbg" ) );
  //pInfoShow->setText( pConfig->readEntry( "InfoShow" ) );
  pWarnCombo->setCurrentIndex( pConfig->readEntry( "WarnOutput", 2 ) );
  pWarnFile->setText( pConfig->readPathEntry( "WarnFilename","kdebug.dbg" ) );
  //pWarnShow->setText( pConfig->readEntry( "WarnShow" ) );
  pErrorCombo->setCurrentIndex( pConfig->readEntry( "ErrorOutput", 2 ) );
  pErrorFile->setText( pConfig->readPathEntry( "ErrorFilename","kdebug.dbg") );
  //pErrorShow->setText( pConfig->readEntry( "ErrorShow" ) );
  pFatalCombo->setCurrentIndex( pConfig->readEntry( "FatalOutput", 2 ) );
  pFatalFile->setText( pConfig->readPathEntry("FatalFilename","kdebug.dbg") );
  //pFatalShow->setText( pConfig->readEntry( "FatalShow" ) );
  pAbortFatal->setChecked( pConfig->readEntry( "AbortFatal", 1 ) );
  slotDestinationChanged(0);
}

void KDebugDialog::save()
{
  pConfig->writeEntry( "InfoOutput", pInfoCombo->currentIndex() );
  pConfig->writePathEntry( "InfoFilename", pInfoFile->text() );
  //pConfig->writeEntry( "InfoShow", pInfoShow->text() );
  pConfig->writeEntry( "WarnOutput", pWarnCombo->currentIndex() );
  pConfig->writePathEntry( "WarnFilename", pWarnFile->text() );
  //pConfig->writeEntry( "WarnShow", pWarnShow->text() );
  pConfig->writeEntry( "ErrorOutput", pErrorCombo->currentIndex() );
  pConfig->writePathEntry( "ErrorFilename", pErrorFile->text() );
  //pConfig->writeEntry( "ErrorShow", pErrorShow->text() );
  pConfig->writeEntry( "FatalOutput", pFatalCombo->currentIndex() );
  pConfig->writePathEntry( "FatalFilename", pFatalFile->text() );
  //pConfig->writeEntry( "FatalShow", pFatalShow->text() );
  pConfig->writeEntry( "AbortFatal", pAbortFatal->isChecked() );

  QByteArray data;
  if (!kapp->dcopClient()->send("*", "KDebug", "notifyKDebugConfigChanged()", data))
  {
    kError() << "Unable to send DCOP message" << endl;
  }
}

void KDebugDialog::slotDestinationChanged(int) {
    pInfoFile->setEnabled(pInfoCombo->currentIndex() == 0);
    pWarnFile->setEnabled(pWarnCombo->currentIndex() == 0);
    pErrorFile->setEnabled(pErrorCombo->currentIndex() == 0);
    pFatalFile->setEnabled(pFatalCombo->currentIndex() == 0);
}

#include "kdebugdialog.moc"
