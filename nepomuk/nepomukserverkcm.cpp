/* This file is part of the KDE Project
   Copyright (c) 2007 Sebastian Trueg <trueg@kde.org>

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

#include "nepomukserverkcm.h"
#include "strigicontroller.h"
#include "nepomukserversettings.h"
#include "nepomukserverinterface.h"

#include <KPluginFactory>
#include <KPluginLoader>
#include <KAboutData>
#include <KSharedConfig>
#include <KLed>
#include <KMessageBox>

#include <strigi/qtdbus/strigiclient.h>


K_PLUGIN_FACTORY( NepomukConfigModuleFactory, registerPlugin<Nepomuk::ServerConfigModule>(); )
K_EXPORT_PLUGIN( NepomukConfigModuleFactory("kcm_nepomuk") )


Nepomuk::ServerConfigModule::ServerConfigModule( QWidget* parent, const QVariantList& args )
    : KCModule( NepomukConfigModuleFactory::componentData(), parent, args ),
      m_serverInterface( "org.kde.NepomukServer", "/modules/nepomukserver", QDBusConnection::sessionBus() )
{
    KAboutData *about = new KAboutData(
        "kcm_nepomuk", 0, ki18n("Nepomuk Configuration Module"),
        KDE_VERSION_STRING, KLocalizedString(), KAboutData::License_GPL,
        ki18n("Copyright 2007 Sebastian Trüg"));
    about->addAuthor(ki18n("Sebastian Trüg"), KLocalizedString(), "trueg@kde.org");
    setAboutData(about);

    setupUi( this );

    connect( m_checkEnableStrigi, SIGNAL( toggled(bool) ),
             this, SLOT( changed() ) );
    connect( m_checkEnableNepomuk, SIGNAL( toggled(bool) ),
             this, SLOT( changed() ) );
    connect( m_editStrigiFolders, SIGNAL( changed() ),
             this, SLOT( changed() ) );

    load();
}


Nepomuk::ServerConfigModule::~ServerConfigModule()
{
}


void Nepomuk::ServerConfigModule::load()
{
    if ( m_serverInterface.isValid() ) {
        m_checkEnableStrigi->setChecked( m_serverInterface.isStrigiEnabled().value() );
        m_checkEnableNepomuk->setChecked( m_serverInterface.isNepomukEnabled().value() );
    }
    else {
        KMessageBox::sorry( this,
                            i18n( "The Nepomuk Server KDED module is not running. The settings "
                                  "will be used the next time the server is started." ),
                            i18n( "Nepomuk server not running" ) );

        m_checkEnableStrigi->setChecked( NepomukServerSettings::self()->startStrigi() );
        m_checkEnableNepomuk->setChecked( NepomukServerSettings::self()->startNepomuk() );
    }

    if ( StrigiController::isRunning() ) {
        m_editStrigiFolders->insertStringList( StrigiClient().getIndexedDirectories() );
    }
    else {
        // FIXME: read the strigi daemon config.
    }

    updateStrigiStatus();
}


void Nepomuk::ServerConfigModule::save()
{
    // 1. change the settings (in case the server is not running)
    NepomukServerSettings::self()->setStartStrigi( m_checkEnableStrigi->isChecked() );
    NepomukServerSettings::self()->setStartNepomuk( m_checkEnableNepomuk->isChecked() );
    NepomukServerSettings::self()->writeConfig();

    // 2. update the current state of the nepomuk server
    if ( m_serverInterface.isValid() ) {
        m_serverInterface.enableNepomuk( m_checkEnableNepomuk->isChecked() );
        m_serverInterface.enableStrigi( m_checkEnableStrigi->isChecked() );
    }
    else {
        KMessageBox::sorry( this,
                            i18n( "The Nepomuk Server KDED module is not running. The settings have been saved "
                                  "and will be used the next time the server is started." ),
                            i18n( "Nepomuk server not running" ) );
    }

    if ( StrigiController::isRunning() ) {
        StrigiClient().setIndexedDirectories( m_editStrigiFolders->items() );
    }
    else {
        // FIXME: update strigi daemon config
    }

    updateStrigiStatus();
}


void Nepomuk::ServerConfigModule::defaults()
{
    NepomukServerSettings::self()->setDefaults();
    m_checkEnableStrigi->setChecked( NepomukServerSettings::self()->startStrigi() );
    m_checkEnableNepomuk->setChecked( NepomukServerSettings::self()->startNepomuk() );
}


void Nepomuk::ServerConfigModule::updateStrigiStatus()
{
    if ( StrigiController::isRunning() ) {
        m_strigiStatus->on();
        m_strigiStatusLabel->setText( i18n( "Strigi is running" ) );
    }
    else {
        m_strigiStatus->off();
        m_strigiStatusLabel->setText( i18n( "Strigi not running" ) );
    }
}

#include "nepomukserverkcm.moc"
