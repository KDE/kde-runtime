/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005-2007 Kevin Ottens <ervin@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "deviceactionsdialog.h"
#include <QtGui/QLayout>

#include <krun.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kio/global.h>
#include <klistwidget.h>
#include <QCheckBox>


#include "deviceaction.h"
#include "ui_deviceactionsdialogview.h"

DeviceActionsDialog::DeviceActionsDialog(QWidget *parent)
    : KDialog(parent)
{
    setModal(false);
    setButtons(Ok|Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);

    QWidget *page = new QWidget(this);
    m_view.setupUi(page);
    setMainWidget(page);
    updateActionsListBox();

    resize(QSize(400,400).expandedTo(minimumSizeHint()));

    connect(this, SIGNAL(okClicked()),
            this, SLOT(slotOk()));
    connect(m_view.actionsList, SIGNAL(doubleClicked(QListWidgetItem *, const QPoint &)),
            this, SLOT(slotOk()));

    connect(this, SIGNAL(finished()),
            this, SLOT(delayedDestruct()));
}

DeviceActionsDialog::~DeviceActionsDialog()
{
}

void DeviceActionsDialog::setDevice(const Solid::Device &device)
{
    m_device = device;
    
    QString label = device.vendor();
    if (!label.isEmpty()) label+=' ';
    label+= device.product();
    
    setWindowTitle(label); 
    
    m_view.iconLabel->setPixmap(KIcon(device.icon()).pixmap(64));
    m_view.descriptionLabel->setText(device.vendor()+' '+device.product());
    setWindowIcon(KIcon(device.icon()));
}

Solid::Device DeviceActionsDialog::device() const
{
    return m_device;
}

void DeviceActionsDialog::setActions(const QList<DeviceAction*> &actions)
{
    qDeleteAll(m_actions);
    m_actions.clear();

    m_actions = actions;

    updateActionsListBox();
}

QList<DeviceAction*> DeviceActionsDialog::actions() const
{
    return m_actions;
}

void DeviceActionsDialog::updateActionsListBox()
{
    m_view.actionsList->clear();

    foreach (DeviceAction *action, m_actions) {
        QListWidgetItem *item = new QListWidgetItem(KIcon(action->iconName()),
                                                    action->label());
        item->setData(Qt::UserRole, action->id());
        m_view.actionsList->addItem(item);
    }

    if (m_view.actionsList->count()>0)
        m_view.actionsList->item(0)->setSelected(true);
}

void DeviceActionsDialog::slotOk()
{
    QListWidgetItem *item = m_view.actionsList->selectedItems().value(0);

    if (item != 0L) {
        QString id = item->data(Qt::UserRole).toString();

        foreach (DeviceAction *action, m_actions) {
            if (action->id()==id) {
                launchAction(action);
                return;
            }
        }
    }
}

void DeviceActionsDialog::launchAction(DeviceAction *action)
{
    action->execute(m_device);
    close();
}

#include "deviceactionsdialog.moc"
