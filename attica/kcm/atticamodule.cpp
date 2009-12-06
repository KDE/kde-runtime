/*
    This file is part of KDE.

    Copyright (c) 2009 Eckhart Wörner <ewoerner@kde.org>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "atticamodule.h"

#include <QtCore/QDebug>
#include <QtGui/QProgressDialog>

#include <KDE/KPluginFactory>
#include <kaboutdata.h>
#include <KDE/KWallet/Wallet>

#include <attica/providermanager.h>

#include "providereditdialog.h"


K_PLUGIN_FACTORY(AtticaModuleFactory, registerPlugin<AtticaModule>();)
K_EXPORT_PLUGIN(AtticaModuleFactory("kcm_attica"))


using namespace Attica;
using namespace KWallet;

AtticaModule::AtticaModule(QWidget* parent, const QVariantList&)
    : KCModule(AtticaModuleFactory::componentData(), parent), m_wallet(0)
{
    KAboutData *about = new KAboutData(
            "kcm_attica", 0, ki18n("Open Collaboration Services Configuration"),
            KDE_VERSION_STRING, KLocalizedString(), KAboutData::License_GPL,
            ki18n("Copyright 2009 Eckhart Wörner"));
    about->addAuthor(ki18n("Eckhart Wörner"), KLocalizedString(), "ewoerner@kde.org");
    about->addAuthor(ki18n("Dmitry Suzdalev"), KLocalizedString(), "dimsuz@gmail.com");
    setAboutData(about);

    m_management.setupUi(this);
    m_management.providerTree->setColumnCount(2);
    m_management.providerTree->setHeaderLabels(QStringList(i18n("Provider")) << i18n("Base URL"));
    m_management.providerTree->setIconSize(QSize(48, 48));
    connect(m_management.editButton, SIGNAL(clicked(bool)), SLOT(edit()));
    m_wallet = Wallet::openWallet(Wallet::NetworkWallet(), 0);
    m_wallet->createFolder("Attica");
    m_wallet->setFolder("Attica");
    m_manager.loadDefaultProviders();
    connect(&m_manager, SIGNAL(providerAdded(const Attica::Provider&)), SLOT(providerAdded(const Attica::Provider&)));
    connect(m_management.providerTree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), SLOT(currentItemChanged(QTreeWidgetItem*)));
    connect(m_management.providerTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(edit()));
}


void AtticaModule::defaults()
{
}


void AtticaModule::load()
{
    m_manager.clear();
    m_manager.loadDefaultProviders();
}


void AtticaModule::save()
{
}


void AtticaModule::edit() {
    QTreeWidgetItem* currentItem = m_management.providerTree->currentItem();
    QUrl currentProviderUrl(m_providerForItem.value(currentItem));
    ProviderEditDialog* dialog = new ProviderEditDialog(m_manager.providerByUrl(currentProviderUrl), m_wallet, this);
    dialog->show();
}


void AtticaModule::currentItemChanged(QTreeWidgetItem* current) {
    m_management.editButton->setEnabled(current != 0);
}


void AtticaModule::providerAdded(const Attica::Provider& provider)
{
    // Add new provider
    QString baseUrl = provider.baseUrl().toString();
    if (!m_itemForProvider.contains(baseUrl)) {
        qDebug() << m_itemForProvider << "does not contain" << baseUrl;
        QTreeWidgetItem* providerItem = new QTreeWidgetItem(0);
        providerItem->setText(0, provider.name());
        providerItem->setIcon(0, KIcon("system-users"));
        providerItem->setText(1, baseUrl);
        m_management.providerTree->addTopLevelItem(providerItem);
        m_itemForProvider.insert(baseUrl, providerItem);
        m_providerForItem.insert(providerItem, baseUrl);
    }

    m_management.providerTree->resizeColumnToContents(0);
}


#include "atticamodule.moc"
