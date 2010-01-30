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

#include <QDebug>
#include <QProgressDialog>

#include <KPluginFactory>
#include <KAboutData>
#include <KWallet/Wallet>
#include <KDebug>

#include <attica/providermanager.h>

#include "providerconfigwidget.h"


K_PLUGIN_FACTORY(AtticaModuleFactory, registerPlugin<AtticaModule>();)
K_EXPORT_PLUGIN(AtticaModuleFactory("kcm_attica"))

AtticaModule::AtticaModule(QWidget* parent, const QVariantList&)
    : KCModule(AtticaModuleFactory::componentData(), parent)
{
    KAboutData *about = new KAboutData(
            "kcm_attica", 0, ki18n("Open Collaboration Services"),
            KDE_VERSION_STRING, KLocalizedString(), KAboutData::License_GPL,
            ki18n("Copyright 2009 Eckhart Wörner"));
    about->addAuthor(ki18n("Eckhart Wörner"), KLocalizedString(), "ewoerner@kde.org");
    about->addAuthor(ki18n("Dmitry Suzdalev"), KLocalizedString(), "dimsuz@gmail.com");
    setAboutData(about);

    m_management.setupUi(this);
    connect(m_management.providerConfigWidget, SIGNAL(changed(bool)),
            this, SIGNAL(changed(bool)));

    m_manager.setAuthenticationSuppressed(true);

    connect(&m_manager, SIGNAL(providerAdded(const Attica::Provider&)), SLOT(providerAdded(const Attica::Provider&)));
    connect(&m_manager, SIGNAL(defaultProvidersLoaded()), SLOT(onDefaultProvidersLoaded()));

    startLoadingDefaultProviders();
}

AtticaModule::~AtticaModule()
{
}

void AtticaModule::defaults()
{
}


void AtticaModule::load()
{
    startLoadingDefaultProviders();
}


void AtticaModule::save()
{
    m_management.providerConfigWidget->saveData();
}

void AtticaModule::startLoadingDefaultProviders()
{
    emit changed(true);
    m_manager.clear();
    m_manager.loadDefaultProviders();
    m_management.lblProviderList->setText(i18n("Loading provider list..."));
    m_management.providerComboBox->hide();
    m_management.providerConfigWidget->setEnabled(false);
}


void AtticaModule::providerAdded(const Attica::Provider& provider)
{
    // Add new provider
    QString baseUrl = provider.baseUrl().toString();
    int idx = m_management.providerComboBox->findData(baseUrl);

    if ( idx == -1)
    {
        kDebug() << "Adding provider" << baseUrl;
        QString name = provider.name();
        if (name.isEmpty())
            name = baseUrl;
        m_management.providerComboBox->addItem(KIcon("system-users"), name);
    }

    // set only if this is a first provider, otherwise it will be
    // set on explicit selection
    if (m_management.providerComboBox->count() == 1)
        m_management.providerConfigWidget->setProvider(provider);
}

void AtticaModule::onDefaultProvidersLoaded()
{
    m_management.lblProviderList->setText(i18n("Choose a provider to manage:"));
    m_management.providerComboBox->show();
    m_management.providerConfigWidget->setEnabled(true);

    // at least now set it to not changed
    emit changed(false);
}

#include "atticamodule.moc"
