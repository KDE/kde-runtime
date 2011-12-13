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

#include "kdebugdialog.h"

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

KDebugDialog::KDebugDialog(const AreaMap& areaMap, QWidget* parent)
    : KAbstractDebugDialog(parent)
{
    setCaption(i18n("Debug Settings"));
    setButtons(None);

    setupUi(mainWidget());
    mainWidget()->layout()->setContentsMargins(0, 0, 0, 0);

    // Debug area tree
    m_incrSearch->searchLine()->addTreeWidget(m_areaWidget);

    for( QMap<QString,QString>::const_iterator it = areaMap.begin(); it != areaMap.end(); ++it ) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_areaWidget, QStringList() << it.value());
        item->setData(0, Qt::UserRole, it.key().simplified());
    }

    QStringList destList;
    destList.append( i18n("File") );
    destList.append( i18n("Message Box") );
    destList.append( i18n("Shell") );
    destList.append( i18n("Syslog") );
    destList.append( i18n("None") );

    //
    // Upper left frame
    //
    connect(pInfoCombo, SIGNAL(activated(int)),
            this, SLOT(slotDestinationChanged()));
    pInfoCombo->addItems( destList );

    //
    // Upper right frame
    //
    connect(pWarnCombo, SIGNAL(activated(int)),
            this, SLOT(slotDestinationChanged()));
    pWarnCombo->addItems( destList );

    //
    // Lower left frame
    //
    connect(pErrorCombo, SIGNAL(activated(int)),
            this, SLOT(slotDestinationChanged()));
    pErrorCombo->addItems( destList );

    //
    // Lower right frame
    //
    connect(pFatalCombo, SIGNAL(activated(int)),
            this, SLOT(slotDestinationChanged()));
    pFatalCombo->addItems( destList );

    // Hack!
    m_disableAll = m_disableAll2;
    connect(m_disableAll, SIGNAL(toggled(bool)), this, SLOT(disableAllClicked()));

    showButtonSeparator(true);
    buildButtons();

    connect( m_areaWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            SLOT(slotDebugAreaChanged(QTreeWidgetItem*)) );

    // Get initial values
    showArea(QString("0"));

    load();

    resize(600, height());
}

KDebugDialog::~KDebugDialog()
{
}

void KDebugDialog::slotDebugAreaChanged(QTreeWidgetItem* item)
{
    // Save settings from previous page
    save();

    const QString areaName = item->data(0, Qt::UserRole).toString();
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
    bool enabled = !m_disableAll->isChecked();
    m_areaWidget->setEnabled(enabled);
    pInfoGroup->setEnabled(enabled);
    pWarnGroup->setEnabled(enabled);
    pErrorGroup->setEnabled(enabled);
    pFatalGroup->setEnabled(enabled);
    pAbortFatal->setEnabled(enabled);
}

#include "kdebugdialog.moc"
