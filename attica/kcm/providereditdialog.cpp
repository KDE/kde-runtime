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

#include "providereditdialog.h"

#include <KDE/KWallet/Wallet>
#include <KDebug>

#include <attica/person.h>

ProviderEditDialog::ProviderEditDialog(const Attica::Provider& provider,
                                       KWallet::Wallet* wallet,
                                       QWidget* parent)
    : KPageDialog(parent), m_provider(provider), m_wallet(wallet),
    m_loginPageItem(0), m_registerPageItem(0)
{
    setFaceType(List);
    setButtons(KDialog::Ok | KDialog::Cancel);
    showButtonSeparator(true);
    initLoginPage();
    initRegisterPage();

    resize(640, sizeHint().height());
}

void ProviderEditDialog::initLoginPage()
{
    QWidget* loginPage = new QWidget(this);
    m_settingsWidget.setupUi(loginPage);
    m_loginPageItem = addPage(loginPage, i18n("Login"));

    QString header;
    if (m_provider.name().isEmpty()) {
        header = i18n("Account details");
    } else {
        header = i18n("Account details for %1", m_provider.name());
    }
    m_loginPageItem->setHeader(header);
    m_loginPageItem->setIcon(KIcon("applications-internet"));

    QMap<QString, QString> details;
    m_wallet->readMap(m_provider.baseUrl().toString(), details);
    m_settingsWidget.userEdit->setText(details.value("user"));
    m_settingsWidget.passwordEdit->setText(details.value("password"));

    m_settingsWidget.iconLabel->setPixmap(KIcon("help-about").pixmap(24,24));

    connect(m_settingsWidget.userEdit, SIGNAL(textChanged(const QString&)), this, SLOT(loginChanged()));
    connect(m_settingsWidget.passwordEdit, SIGNAL(textChanged(const QString&)), this, SLOT(loginChanged()));
    connect(m_settingsWidget.testLoginButton, SIGNAL(clicked()), this, SLOT(testLogin()));
    connect(m_settingsWidget.infoLabel, SIGNAL(linkActivated(const QString&)), this, SLOT(infoLinkActivated()));
}

void ProviderEditDialog::initRegisterPage()
{
    QWidget* registerPage = new QWidget(this);
    m_registerWidget.setupUi(registerPage);
    m_registerPageItem = addPage(registerPage, i18n("Register"));

    QString header;
    if (m_provider.name().isEmpty()) {
        header = i18n("Register new account");
    } else {
        header = i18n("Register new account at %1", m_provider.name());
    }
    m_registerPageItem->setHeader(header);
    m_registerPageItem->setIcon(KIcon("list-add-user"));

    m_registerWidget.infoLabel->setFont(KGlobalSettings::smallestReadableFont());

    connect(m_registerWidget.userEdit, SIGNAL(textChanged(QString)), SLOT(validateRegisterFields()));
    connect(m_registerWidget.mailEdit, SIGNAL(textChanged(QString)), SLOT(validateRegisterFields()));
    connect(m_registerWidget.firstNameEdit, SIGNAL(textChanged(QString)), SLOT(validateRegisterFields()));
    connect(m_registerWidget.lastNameEdit, SIGNAL(textChanged(QString)), SLOT(validateRegisterFields()));
    connect(m_registerWidget.passwordEdit, SIGNAL(textChanged(QString)), SLOT(validateRegisterFields()));
    connect(m_registerWidget.passwordRepeatEdit, SIGNAL(textChanged(QString)), SLOT(validateRegisterFields()));

    connect(m_registerWidget.registerButton, SIGNAL(clicked()), SLOT(onRegisterClicked()));

    validateRegisterFields();
}

void ProviderEditDialog::loginChanged()
{
    m_settingsWidget.testLoginButton->setText(i18n("Test login"));
    m_settingsWidget.testLoginButton->setEnabled(true);
}

void ProviderEditDialog::testLogin()
{
    m_settingsWidget.testLoginButton->setEnabled(false);
    m_settingsWidget.testLoginButton->setText(i18n("Testing login..."));

    Attica::PostJob* postJob = m_provider.checkLogin(m_settingsWidget.userEdit->text(), m_settingsWidget.passwordEdit->text());
    connect(postJob, SIGNAL(finished(Attica::BaseJob*)), SLOT(testLoginFinished(Attica::BaseJob*)));
    postJob->start();
}

void ProviderEditDialog::testLoginFinished(Attica::BaseJob* job)
{
    Attica::PostJob* postJob = static_cast<Attica::PostJob*>(job);

    if (postJob->metadata().error() == Attica::Metadata::NoError) {
        m_settingsWidget.testLoginButton->setText(i18n("Success"));
    }

    if (postJob->metadata().error() == Attica::Metadata::OcsError) {
        m_settingsWidget.testLoginButton->setText(i18n("Login failed"));
    }
}

void ProviderEditDialog::infoLinkActivated()
{
    setCurrentPage(m_registerPageItem);
}

void ProviderEditDialog::validateRegisterFields()
{
    QString login = m_registerWidget.userEdit->text();
    QString mail = m_registerWidget.mailEdit->text();
    QString firstName = m_registerWidget.firstNameEdit->text();
    QString lastName = m_registerWidget.lastNameEdit->text();
    QString password = m_registerWidget.passwordEdit->text();
    QString passwordRepeat = m_registerWidget.passwordRepeatEdit->text();

    bool isDataValid = (!login.isEmpty() && !mail.isEmpty() && !firstName.isEmpty() &&
                        !lastName.isEmpty() && !password.isEmpty() && !passwordRepeat.isEmpty());
    bool isPasswordValid = !password.isEmpty() && !passwordRepeat.isEmpty() && password == passwordRepeat;

    if (!isDataValid)
        showRegisterHint("dialog-cancel", i18n("Not all required fields are filled"));
    if (isDataValid && !isPasswordValid)
        showRegisterHint("dialog-cancel", i18n("Passwords do not match"));
    if (isDataValid && isPasswordValid )
        showRegisterHint("dialog-ok-apply", i18n("All required information is provided"));

    m_registerWidget.registerButton->setEnabled(isDataValid && isPasswordValid);
}

void ProviderEditDialog::showRegisterHint(const QString& iconName, const QString& hint)
{
    m_registerWidget.iconLabel->setPixmap(KIcon(iconName).pixmap(16,16));
    m_registerWidget.infoLabel->setText(hint);
}

void ProviderEditDialog::onRegisterClicked()
{
    // here we assume that all data has been checked with validateRegisterFields()

    QString login = m_registerWidget.userEdit->text();
    QString mail = m_registerWidget.mailEdit->text();
    QString firstName = m_registerWidget.firstNameEdit->text();
    QString lastName = m_registerWidget.lastNameEdit->text();
    QString password = m_registerWidget.passwordEdit->text();
    //QString passwordRepeat = m_registerWidget.passwordRepeatEdit->text();

    Attica::PostJob* postJob = m_provider.registerAccount(login, password, mail, firstName, lastName);
    connect(postJob, SIGNAL(finished(Attica::BaseJob*)), SLOT(registerAccountFinished(Attica::BaseJob*)));
    postJob->start();
    showRegisterHint("help-about", i18n("Registration is in progress..."));
    m_registerWidget.registerButton->setEnabled(false); // should be disabled while registering
}

void ProviderEditDialog::registerAccountFinished(Attica::BaseJob* job)
{
    Attica::PostJob* postJob = static_cast<Attica::PostJob*>(job);

    // this will enable "register" button if possible
    // (important to have this call before showRegisterHint() below,
    // so that the correct message will be displayed)
    validateRegisterFields();

    if (postJob->metadata().error() == Attica::Metadata::NoError)
    {
        showRegisterHint("dialog-ok-apply", i18n("Congratulations! New account was successfully registered."));
    }
    else
    {
        // TODO: more detailed error parsing
        showRegisterHint("dialog-close", i18n("Failed to register new account."));
        kDebug() << "register error:" << postJob->metadata().error() << "statusCode:" << postJob->metadata().statusCode()
            << "status string:"<< postJob->metadata().statusCode();
    }
}

void ProviderEditDialog::accept()
{
    QMap<QString, QString> details;
    details.insert("user", m_settingsWidget.userEdit->text());
    details.insert("password", m_settingsWidget.passwordEdit->text());
    m_wallet->writeMap(m_provider.baseUrl().toString(), details);
    KDialog::accept();
}

#include "providereditdialog.moc"
