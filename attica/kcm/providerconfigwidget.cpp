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

#include "providerconfigwidget.h"

#include <KDebug>

#include <attica/person.h>

static const int loginTabIdx = 0;
static const int registerTabIdx = 1;

ProviderConfigWidget::ProviderConfigWidget(QWidget* parent)
    : QWidget(parent)
{
}

void ProviderConfigWidget::setProvider(const Attica::Provider& provider)
{
    m_provider = provider;
    m_settingsWidget.setupUi(this);

    // TODO ensure that it reinits all fields nicely for new provider!
    initLoginPage();
    initRegisterPage();
}

void ProviderConfigWidget::initLoginPage()
{
    QString header;
    if (m_provider.name().isEmpty()) {
        header = i18n("Account details");
    } else {
        header = i18n("Account details for %1", m_provider.name());
    }
    m_settingsWidget.titleWidgetLogin->setText(header);
    m_settingsWidget.tabWidget->setTabIcon(loginTabIdx, KIcon("applications-internet"));

    if (m_provider.hasCredentials()) {
        QString user;
        QString password;
        m_provider.loadCredentials(user, password);
        m_settingsWidget.userEditLP->setText(user);
        m_settingsWidget.passwordEditLP->setText(password);
    }
    m_settingsWidget.iconLabelLP->setPixmap(KIcon("help-about").pixmap(24,24));

    connect(m_settingsWidget.userEditLP, SIGNAL(textChanged(const QString&)), this, SLOT(loginChanged()));
    connect(m_settingsWidget.passwordEditLP, SIGNAL(textChanged(const QString&)), this, SLOT(loginChanged()));
    connect(m_settingsWidget.testLoginButton, SIGNAL(clicked()), this, SLOT(testLogin()));
    connect(m_settingsWidget.infoLabelLP, SIGNAL(linkActivated(const QString&)), this, SLOT(infoLinkActivated()));
}

void ProviderConfigWidget::initRegisterPage()
{
    QString header;
    if (m_provider.name().isEmpty()) {
        header = i18n("Register new account");
    } else {
        header = i18n("Register new account at %1", m_provider.name());
    }
    m_settingsWidget.titleWidgetRegister->setText(header);
    m_settingsWidget.tabWidget->setTabIcon(registerTabIdx, KIcon("list-add-user"));

    m_settingsWidget.infoLabelRP->setFont(KGlobalSettings::smallestReadableFont());

    connect(m_settingsWidget.userEditRP, SIGNAL(textChanged(QString)), SLOT(validateRegisterFields()));
    connect(m_settingsWidget.mailEdit, SIGNAL(textChanged(QString)), SLOT(validateRegisterFields()));
    connect(m_settingsWidget.firstNameEdit, SIGNAL(textChanged(QString)), SLOT(validateRegisterFields()));
    connect(m_settingsWidget.lastNameEdit, SIGNAL(textChanged(QString)), SLOT(validateRegisterFields()));
    connect(m_settingsWidget.passwordEditRP, SIGNAL(textChanged(QString)), SLOT(validateRegisterFields()));
    connect(m_settingsWidget.passwordRepeatEdit, SIGNAL(textChanged(QString)), SLOT(validateRegisterFields()));

    connect(m_settingsWidget.registerButton, SIGNAL(clicked()), SLOT(onRegisterClicked()));

    validateRegisterFields();
}

void ProviderConfigWidget::loginChanged()
{
    m_settingsWidget.testLoginButton->setText(i18n("Test login"));
    m_settingsWidget.testLoginButton->setEnabled(true);
}

void ProviderConfigWidget::testLogin()
{
    m_settingsWidget.testLoginButton->setEnabled(false);
    m_settingsWidget.testLoginButton->setText(i18n("Testing login..."));

    Attica::PostJob* postJob = m_provider.checkLogin(m_settingsWidget.userEditLP->text(), m_settingsWidget.passwordEditLP->text());
    connect(postJob, SIGNAL(finished(Attica::BaseJob*)), SLOT(testLoginFinished(Attica::BaseJob*)));
    postJob->start();
}

void ProviderConfigWidget::testLoginFinished(Attica::BaseJob* job)
{
    Attica::PostJob* postJob = static_cast<Attica::PostJob*>(job);

    if (postJob->metadata().error() == Attica::Metadata::NoError) {
        m_settingsWidget.testLoginButton->setText(i18n("Success"));
    }

    if (postJob->metadata().error() == Attica::Metadata::OcsError) {
        m_settingsWidget.testLoginButton->setText(i18n("Login failed"));
    }
}

void ProviderConfigWidget::infoLinkActivated()
{
    m_settingsWidget.tabWidget->setCurrentIndex(registerTabIdx);
}

void ProviderConfigWidget::validateRegisterFields()
{
    QString login = m_settingsWidget.userEditRP->text();
    QString mail = m_settingsWidget.mailEdit->text();
    QString firstName = m_settingsWidget.firstNameEdit->text();
    QString lastName = m_settingsWidget.lastNameEdit->text();
    QString password = m_settingsWidget.passwordEditRP->text();
    QString passwordRepeat = m_settingsWidget.passwordRepeatEdit->text();

    bool isDataValid = (!login.isEmpty() && !mail.isEmpty() && !firstName.isEmpty() &&
                        !lastName.isEmpty() && !password.isEmpty() && !passwordRepeat.isEmpty());
    bool isPasswordValid = !password.isEmpty() && !passwordRepeat.isEmpty() && password == passwordRepeat;

    if (!isDataValid)
        showRegisterHint("dialog-cancel", i18n("Not all required fields are filled"));
    if (isDataValid && !isPasswordValid)
        showRegisterHint("dialog-cancel", i18n("Passwords do not match"));
    if (isDataValid && isPasswordValid )
        showRegisterHint("dialog-ok-apply", i18n("All required information is provided"));

    showRegisterHint("dialog-ok-apply", i18n("Registration complete. New account was successfully registered.<br/>Please <b>check your Email</b> to <b>activate</b> the account."));


    m_settingsWidget.registerButton->setEnabled(isDataValid && isPasswordValid);
}

void ProviderConfigWidget::showRegisterHint(const QString& iconName, const QString& hint)
{
    m_settingsWidget.iconLabelRP->setPixmap(KIcon(iconName).pixmap(16,16));
    m_settingsWidget.infoLabelRP->setText(hint);
}

void ProviderConfigWidget::onRegisterClicked()
{
    // here we assume that all data has been checked with validateRegisterFields()

    QString login = m_settingsWidget.userEditRP->text();
    QString mail = m_settingsWidget.mailEdit->text();
    QString firstName = m_settingsWidget.firstNameEdit->text();
    QString lastName = m_settingsWidget.lastNameEdit->text();
    QString password = m_settingsWidget.passwordEditRP->text();
    //QString passwordRepeat = m_settingsWidget.passwordRepeatEdit->text();

    Attica::PostJob* postJob = m_provider.registerAccount(login, password, mail, firstName, lastName);
    connect(postJob, SIGNAL(finished(Attica::BaseJob*)), SLOT(registerAccountFinished(Attica::BaseJob*)));
    postJob->start();
    showRegisterHint("help-about", i18n("Registration is in progress..."));
    m_settingsWidget.registerButton->setEnabled(false); // should be disabled while registering
}

void ProviderConfigWidget::registerAccountFinished(Attica::BaseJob* job)
{
    Attica::PostJob* postJob = static_cast<Attica::PostJob*>(job);

    // this will enable "register" button if possible
    // (important to have this call before showRegisterHint() below,
    // so that the correct message will be displayed)
    validateRegisterFields();

    if (postJob->metadata().error() == Attica::Metadata::NoError)
    {
        showRegisterHint("dialog-ok-apply", i18n("Registration complete. New account was successfully registered. Please <b>check your Email</b> to <b>activate</b> the account."));

        QString user = m_settingsWidget.userEditRP->text();
        QString password = m_settingsWidget.passwordEditRP->text();
        m_settingsWidget.userEditLP->setText(user);
        m_settingsWidget.passwordEditLP->setText(password);
        m_provider.saveCredentials(user, password);
    }
    else
    {
        kDebug() << "register error:" << postJob->metadata().error() << "statusCode:" << postJob->metadata().statusCode()
            << "status string:"<< postJob->metadata().statusCode();
        if (postJob->metadata().error() == Attica::Metadata::NetworkError) {
            showRegisterHint("dialog-close", i18n("Failed to register new account."));
        } else {
            /*
# 100 - successfull / valid account
# 101 - please specify all mandatory fields
# 102 - please specify a valid password
# 103 - please specify a valid login
# 104 - login already exists
# 105 - email already taken
            */
            switch (postJob->metadata().statusCode()) {
            case 102:
                showRegisterHint("dialog-close", i18n("Failed to register new account: Password invalid."));
                break;
            case 103:
                showRegisterHint("dialog-close", i18n("Failed to register new account: User name invalid."));
                break;
            case 104:
                showRegisterHint("dialog-close", i18n("Failed to register new account: User name already taken."));
                break;
            case 105:
                showRegisterHint("dialog-close", i18n("Failed to register new account: Email already taken."));
                break;
            default:
                showRegisterHint("dialog-close", i18n("Failed to register new account."));
                break;
            }
        }
    }
}

void ProviderConfigWidget::saveData()
{
    if (m_settingsWidget.userEditLP->text().isEmpty()) {
        return;
    }

    m_provider.saveCredentials(m_settingsWidget.userEditLP->text(), m_settingsWidget.passwordEditLP->text());
}

#include "providerconfigwidget.moc"
