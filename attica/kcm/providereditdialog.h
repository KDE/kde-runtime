/*
    This file is part of KDE.

    Copyright (c) 2009 Eckhart Wörner <ewoerner@kde.org>
    Copyright (c) 2009 Dmitry Suzdalev <dimsuz@gmail.com>

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

#ifndef PROVIDEREDITDIALOG_H
#define PROVIDEREDITDIALOG_H

#include <KDE/KPageDialog>

#include <attica/provider.h>

#include "ui_providersettingswidget.h"
#include "ui_registerwidget.h"

namespace KWallet {
    class Wallet;
}

class KPageWidgetItem;

class ProviderEditDialog : public KPageDialog
{
    Q_OBJECT

public:
    explicit ProviderEditDialog(const Attica::Provider& provider, KWallet::Wallet* wallet, QWidget* parent = 0);
    void accept();

private Q_SLOTS:
    /// reset the validate button
    void loginChanged();
    /// test login was clicked
    void testLogin();
    void infoLinkActivated();
    void validateRegisterFields();
    void onRegisterClicked();

    /// result of login test
    void testLoginFinished(Attica::BaseJob* job);
    void registerAccountFinished(Attica::BaseJob* job);

private:
    void initLoginPage();
    void initRegisterPage();
    void showRegisterHint(const QString&, const QString&);

private:
    Attica::Provider m_provider;
    Ui::ProviderSettingsWidget m_settingsWidget;
    Ui::RegisterWidget m_registerWidget;
    KWallet::Wallet* m_wallet;
    KPageWidgetItem* m_loginPageItem;
    KPageWidgetItem* m_registerPageItem;
};

#endif
