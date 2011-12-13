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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kabstractdebugdialog.h"
#include <kconfig.h>
#include <kpushbutton.h>
#include <QCheckBox>
#include <QLayout>
#include <klocale.h>
#include <kstandardguiitem.h>
#include <ktoolinvocation.h>

KAbstractDebugDialog::KAbstractDebugDialog(QWidget *parent)
    : KDialog(parent)
{
    pConfig = new KConfig( "kdebugrc", KConfig::NoGlobals );
}

KAbstractDebugDialog::~KAbstractDebugDialog()
{
    delete pConfig;
}

void KAbstractDebugDialog::buildButtons( QVBoxLayout * topLayout )
{
    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setSpacing( KDialog::spacingHint() );
    topLayout->addLayout( hbox );
    pHelpButton = new KPushButton( KStandardGuiItem::help() );
    hbox->addWidget( pHelpButton );
    hbox->addStretch(10);
    QSpacerItem *spacer = new QSpacerItem(40, 0);
    hbox->addItem(spacer);
    pOKButton = new KPushButton( KStandardGuiItem::ok() );
    hbox->addWidget( pOKButton );
    pApplyButton = new KPushButton( KStandardGuiItem::apply() );
    hbox->addWidget( pApplyButton );
    pCancelButton = new KPushButton( KStandardGuiItem::cancel() );
    hbox->addWidget( pCancelButton );

    int w1 = pHelpButton->sizeHint().width();
    int w2 = pOKButton->sizeHint().width();
    int w3 = pCancelButton->sizeHint().width();
    int w4 = qMax( w1, qMax( w2, w3 ) );
    int w5 = pApplyButton->sizeHint().width();
    w4 = qMax(w4, w5);

    pHelpButton->setFixedWidth( w4 );
    pOKButton->setFixedWidth( w4 );
    pApplyButton->setFixedWidth( w4 );
    pCancelButton->setFixedWidth( w4 );

    connect( pHelpButton, SIGNAL( clicked() ), SLOT( slotShowHelp() ) );
    connect( pOKButton, SIGNAL( clicked() ), SLOT( accept() ) );
    connect( pApplyButton, SIGNAL( clicked() ), SLOT( slotApply() ) );
    connect( pCancelButton, SIGNAL( clicked() ), SLOT( reject() ) );
}

void KAbstractDebugDialog::slotShowHelp()
{
    KToolInvocation::invokeHelp();
}

void KAbstractDebugDialog::slotApply()
{
    save();
    pConfig->sync();
}

void KAbstractDebugDialog::save()
{
    doSave();
    KConfigGroup topGroup(pConfig, QString());
    topGroup.writeEntry("DisableAll", m_disableAll->isChecked());
}

void KAbstractDebugDialog::load()
{
    doLoad();
    KConfigGroup topGroup(pConfig, QString());
    m_disableAll->setChecked(topGroup.readEntry("DisableAll", false));
}

#include "kabstractdebugdialog.moc"
