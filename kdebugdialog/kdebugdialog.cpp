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

#include <QLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QPushButton>
#include <QtDBus/QtDBus>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdialog.h>
#include <kconfig.h>
#include <kseparator.h>

#include "kdebugdialog.h"

KDebugDialog::KDebugDialog(const AreaMap& areaMap, QWidget* parent)
    : KAbstractDebugDialog(parent)
{
  setCaption(i18n("Debug Settings"));
  setButtons(None);

  QVBoxLayout *topLayout = new QVBoxLayout(mainWidget());
  topLayout->setMargin( KDialog::marginHint() );
  topLayout->setSpacing( KDialog::spacingHint() );

  m_container = new QWidget(mainWidget());
  QVBoxLayout* containerLayout = new QVBoxLayout(m_container);
  containerLayout->setMargin(0);
  containerLayout->setSpacing(0);
  topLayout->addWidget(m_container);

  QLabel * tmpLabel = new QLabel( i18n("Debug area:"), m_container );
  tmpLabel->setFixedHeight( fontMetrics().lineSpacing() );
  containerLayout->addWidget( tmpLabel );

  // Build combo of debug areas
  pDebugAreas = new QComboBox( m_container );
  pDebugAreas->setEditable( false );
  pDebugAreas->setFixedHeight( pDebugAreas->sizeHint().height() );
  for( QMap<QString,QString>::const_iterator it = areaMap.begin(); it != areaMap.end(); ++it ) {
      pDebugAreas->addItem(it.value(), QVariant(it.key().simplified()));
  }
  containerLayout->addWidget( pDebugAreas );

  QGridLayout *gbox = new QGridLayout();
  gbox->setMargin( KDialog::marginHint() );
  containerLayout->addLayout( gbox );

  QStringList destList;
  destList.append( i18n("File") );
  destList.append( i18n("Message Box") );
  destList.append( i18n("Shell") );
  destList.append( i18n("Syslog") );
  destList.append( i18n("None") );

  //
  // Upper left frame
  //
  pInfoGroup = new QGroupBox( i18n("Information"), m_container );
  gbox->addWidget( pInfoGroup, 0, 0 );
  QVBoxLayout *vbox = new QVBoxLayout( pInfoGroup );
  vbox->setSpacing( KDialog::spacingHint() );
  vbox->addSpacing( fontMetrics().lineSpacing() );
  pInfoLabel1 = new QLabel( i18n("Output to:"), pInfoGroup );
  vbox->addWidget( pInfoLabel1 );
  pInfoCombo = new QComboBox( pInfoGroup );
  pInfoCombo->setEditable( false );
  connect(pInfoCombo, SIGNAL(activated(int)),
	  this, SLOT(slotDestinationChanged()));
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
  pWarnGroup = new QGroupBox( i18n("Warning"), m_container );
  gbox->addWidget( pWarnGroup, 0, 1 );
  vbox = new QVBoxLayout( pWarnGroup );
  vbox->setSpacing( KDialog::spacingHint() );
  vbox->addSpacing( fontMetrics().lineSpacing() );
  pWarnLabel1 = new QLabel( i18n("Output to:"), pWarnGroup );
  vbox->addWidget( pWarnLabel1 );
  pWarnCombo = new QComboBox( pWarnGroup );
  pWarnCombo->setEditable( false );
  connect(pWarnCombo, SIGNAL(activated(int)),
	  this, SLOT(slotDestinationChanged()));
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
  pErrorGroup = new QGroupBox( i18n("Error"), m_container );
  gbox->addWidget( pErrorGroup, 1, 0 );
  vbox = new QVBoxLayout( pErrorGroup );
  vbox->setSpacing( KDialog::spacingHint() );
  vbox->addSpacing( fontMetrics().lineSpacing() );
  pErrorLabel1 = new QLabel( i18n("Output to:"), pErrorGroup );
  vbox->addWidget( pErrorLabel1 );
  pErrorCombo = new QComboBox( pErrorGroup );
  pErrorCombo->setEditable( false );
  connect(pErrorCombo, SIGNAL(activated(int)),
	  this, SLOT(slotDestinationChanged()));
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
  pFatalGroup = new QGroupBox( i18n("Fatal Error"), m_container );
  gbox->addWidget( pFatalGroup, 1, 1 );
  vbox = new QVBoxLayout( pFatalGroup );
  vbox->setSpacing( KDialog::spacingHint() );
  vbox->addSpacing( fontMetrics().lineSpacing() );
  pFatalLabel1 = new QLabel( i18n("Output to:"), pFatalGroup );
  vbox->addWidget( pFatalLabel1 );
  pFatalCombo = new QComboBox( pFatalGroup );
  pFatalCombo->setEditable( false );
  connect(pFatalCombo, SIGNAL(activated(int)),
	  this, SLOT(slotDestinationChanged()));
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


  pAbortFatal = new QCheckBox( i18n("Abort on fatal errors"), m_container );
  containerLayout->addWidget(pAbortFatal);

  m_disableAll = new QCheckBox(mainWidget());
  m_disableAll->setText(i18n("Disable all debug output"));
  connect(m_disableAll, SIGNAL(toggled(bool)), this, SLOT(disableAllClicked()));
  topLayout->addWidget(m_disableAll);

  topLayout->addStretch();
  KSeparator *hline = new KSeparator( Qt::Horizontal, mainWidget() );
  topLayout->addWidget( hline );

  buildButtons( topLayout );

  connect( pDebugAreas, SIGNAL(activated(int)),
           SLOT(slotDebugAreaChanged(int)) );

  // Get initial values
  showArea(QString("0"));

  load();

  resize( 300, height() );
}

KDebugDialog::~KDebugDialog()
{
}

void KDebugDialog::slotDebugAreaChanged(int index)
{
    // Save settings from previous page
    save();

    const QString areaName = pDebugAreas->itemData(index).toString();
    showArea(areaName);
}

void KDebugDialog::showArea(const QString& areaName)
{
  /* Fill dialog fields with values from config data */
  mCurrentDebugArea = areaName;
  KConfigGroup group = pConfig->group(areaName);
  pInfoCombo->setCurrentIndex( group.readEntry( "InfoOutput", 2 ) );
  pInfoFile->setText( group.readPathEntry( "InfoFilename","kdebug.dbg" ) );
  //pInfoShow->setText( group.readEntry( "InfoShow" ) );
  pWarnCombo->setCurrentIndex( group.readEntry( "WarnOutput", 2 ) );
  pWarnFile->setText( group.readPathEntry( "WarnFilename","kdebug.dbg" ) );
  //pWarnShow->setText( group.readEntry( "WarnShow" ) );
  pErrorCombo->setCurrentIndex( group.readEntry( "ErrorOutput", 2 ) );
  pErrorFile->setText( group.readPathEntry( "ErrorFilename","kdebug.dbg") );
  //pErrorShow->setText( group.readEntry( "ErrorShow" ) );
  pFatalCombo->setCurrentIndex( group.readEntry( "FatalOutput", 2 ) );
  pFatalFile->setText( group.readPathEntry("FatalFilename","kdebug.dbg") );
  //pFatalShow->setText( group.readEntry( "FatalShow" ) );
  pAbortFatal->setChecked( group.readEntry( "AbortFatal", 1 ) );
  slotDestinationChanged();
}

void KDebugDialog::doSave()
{
  KConfigGroup group = pConfig->group( mCurrentDebugArea ); // Group name = debug area code
  group.writeEntry( "InfoOutput", pInfoCombo->currentIndex() );
  group.writePathEntry( "InfoFilename", pInfoFile->text() );
  //group.writeEntry( "InfoShow", pInfoShow->text() );
  group.writeEntry( "WarnOutput", pWarnCombo->currentIndex() );
  group.writePathEntry( "WarnFilename", pWarnFile->text() );
  //group.writeEntry( "WarnShow", pWarnShow->text() );
  group.writeEntry( "ErrorOutput", pErrorCombo->currentIndex() );
  group.writePathEntry( "ErrorFilename", pErrorFile->text() );
  //group.writeEntry( "ErrorShow", pErrorShow->text() );
  group.writeEntry( "FatalOutput", pFatalCombo->currentIndex() );
  group.writePathEntry( "FatalFilename", pFatalFile->text() );
  //group.writeEntry( "FatalShow", pFatalShow->text() );
  group.writeEntry( "AbortFatal", pAbortFatal->isChecked() );

  QDBusMessage msg = QDBusMessage::createSignal("/", "org.kde.KDebug", "configChanged");
  if (!QDBusConnection::sessionBus().send(msg))
  {
    kError() << "Unable to send D-BUS message" << endl;
  }
}

void KDebugDialog::slotDestinationChanged()
{
    pInfoFile->setEnabled(pInfoCombo->currentIndex() == 0);
    pWarnFile->setEnabled(pWarnCombo->currentIndex() == 0);
    pErrorFile->setEnabled(pErrorCombo->currentIndex() == 0);
    pFatalFile->setEnabled(pFatalCombo->currentIndex() == 0);
}

void KDebugDialog::disableAllClicked()
{
    kDebug();
    m_container->setEnabled(!m_disableAll->isChecked());
}

#include "kdebugdialog.moc"
