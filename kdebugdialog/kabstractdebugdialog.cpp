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

void KAbstractDebugDialog::buildButtons()
{
    setButtons(KDialog::Help | KDialog::Ok | KDialog::Apply | KDialog::Cancel);

    connect(this, SIGNAL(helpClicked()), SLOT( slotShowHelp() ));
    connect(this, SIGNAL(okClicked()), SLOT( accept() ));
    connect(this, SIGNAL(applyClicked()), SLOT( slotApply() ));
    connect(this, SIGNAL(cancelClicked()), SLOT( reject() ));
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
