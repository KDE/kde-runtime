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
#include "nepomukserversettings.h"
#include "nepomukserverinterface.h"
#include "../common/strigiconfigfile.h"

#include <KPluginFactory>
#include <KPluginLoader>
#include <KAboutData>
#include <KSharedConfig>
#include <KLed>
#include <KMessageBox>
#include <KUrlRequester>

#include <strigi/qtdbus/strigiclient.h>


K_PLUGIN_FACTORY( NepomukConfigModuleFactory, registerPlugin<Nepomuk::ServerConfigModule>(); )
K_EXPORT_PLUGIN( NepomukConfigModuleFactory("kcm_nepomuk") )


Nepomuk::ServerConfigModule::ServerConfigModule( QWidget* parent, const QVariantList& args )
    : KCModule( NepomukConfigModuleFactory::componentData(), parent, args ),
      m_serverInterface( "org.kde.NepomukServer", "/nepomukserver", QDBusConnection::sessionBus() )
{
    KAboutData *about = new KAboutData(
        "kcm_nepomuk", 0, ki18n("Nepomuk Configuration Module"),
        KDE_VERSION_STRING, KLocalizedString(), KAboutData::License_GPL,
        ki18n("Copyright 2007 Sebastian Trüg"));
    about->addAuthor(ki18n("Sebastian Trüg"), KLocalizedString(), "trueg@kde.org");
    setAboutData(about);
    setButtons(Apply|Default);
    setupUi( this );

    KUrlRequester* urlReq = new KUrlRequester( m_editStrigiFolders );
    urlReq->setMode( KFile::Directory|KFile::LocalOnly|KFile::ExistingOnly );
    KEditListBox::CustomEditor ce( urlReq, urlReq->lineEdit() );
    m_editStrigiFolders->setCustomEditor( ce );

    connect( m_checkEnableStrigi, SIGNAL( toggled(bool) ),
             this, SLOT( changed() ) );
    connect( m_checkEnableNepomuk, SIGNAL( toggled(bool) ),
             this, SLOT( changed() ) );
    connect( m_editStrigiFolders, SIGNAL( changed() ),
             this, SLOT( changed() ) );
    connect( m_editStrigiExcludeFilters, SIGNAL( changed() ),
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

    if ( isStrigiRunning() ) {
        StrigiClient strigiClient;
        m_editStrigiFolders->setItems( strigiClient.getIndexedDirectories() );
        QList<QPair<bool, QString> > filters = strigiClient.getFilters();
        m_editStrigiExcludeFilters->clear();
        for( QList<QPair<bool, QString> >::const_iterator it = filters.constBegin();
             it != filters.constEnd(); ++it ) {
            if ( !it->first ) {
                m_editStrigiExcludeFilters->insertItem( it->second );
            }
            // else: we simply drop include filters for now
        }
    }
    else {
        StrigiConfigFile strigiConfig( StrigiConfigFile::defaultStrigiConfigFilePath() );
        strigiConfig.load();
        m_editStrigiFolders->setItems( strigiConfig.defaultRepository().indexedDirectories() );
        m_editStrigiExcludeFilters->setItems( strigiConfig.excludeFilters() );
    }

    updateStrigiStatus();
}


void Nepomuk::ServerConfigModule::save()
{
    // 1. change the settings (in case the server is not running)
    NepomukServerSettings::self()->setStartStrigi( m_checkEnableStrigi->isChecked() );
    NepomukServerSettings::self()->setStartNepomuk( m_checkEnableNepomuk->isChecked() );
    NepomukServerSettings::self()->writeConfig();


    // 2. update Strigi config
    StrigiConfigFile strigiConfig( StrigiConfigFile::defaultStrigiConfigFilePath() );
    strigiConfig.load();
    if ( NepomukServerSettings::self()->startNepomuk() ) {
        strigiConfig.defaultRepository().setType( "sopranobackend" );
    }
    else {
        strigiConfig.defaultRepository().setType( "clucene" );
    }
    strigiConfig.defaultRepository().setIndexedDirectories( m_editStrigiFolders->items() );
    strigiConfig.setExcludeFilters( m_editStrigiExcludeFilters->items() );
    strigiConfig.save();


    // 3. update the current state of the nepomuk server
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


    // 4. update values in the running Strigi instance
    // TODO: there should be a dbus method to re-read the config
    // -----------------------------
    if ( m_checkEnableStrigi->isChecked() ) {
        // give strigi some time to start
        QTimer::singleShot( 2000, this, SLOT( updateStrigiSettingsInRunningInstance() ) );
    }

    // give strigi some time to start
    QTimer::singleShot( 2000, this, SLOT( updateStrigiStatus() ) );
}


void Nepomuk::ServerConfigModule::defaults()
{
    NepomukServerSettings::self()->setDefaults();
    m_checkEnableStrigi->setChecked( NepomukServerSettings::self()->startStrigi() );
    m_checkEnableNepomuk->setChecked( NepomukServerSettings::self()->startNepomuk() );
    // create Strigi default config
    StrigiConfigFile defaultConfig;
    m_editStrigiFolders->setItems( defaultConfig.defaultRepository().indexedDirectories() );
    m_editStrigiExcludeFilters->setItems( defaultConfig.excludeFilters() );
}


void Nepomuk::ServerConfigModule::updateStrigiStatus()
{
    if ( isStrigiRunning() ) {
        m_strigiStatus->on();
        m_strigiStatusLabel->setText( i18n( "Strigi is running" ) );
    }
    else {
        m_strigiStatus->off();
        m_strigiStatusLabel->setText( i18n( "Strigi not running" ) );
    }
}


void Nepomuk::ServerConfigModule::updateStrigiSettingsInRunningInstance()
{
    if ( isStrigiRunning() ) {
        StrigiClient strigiClient;
        strigiClient.setIndexedDirectories( m_editStrigiFolders->items() );

        // FIXME: there should be a rereadConfig method in strigi
        StrigiConfigFile strigiConfig( StrigiConfigFile::defaultStrigiConfigFilePath() );
        strigiConfig.load();

        QList<QPair<bool, QString> > filters;
        foreach( QString filter, strigiConfig.excludeFilters() ) {
            filters.append( qMakePair( false, filter ) );
        }
        strigiClient.setFilters( filters );
    }
}


bool Nepomuk::ServerConfigModule::isStrigiRunning()
{
    return QDBusConnection::sessionBus().interface()->isServiceRegistered( "vandenoever.strigi" ).value();
}

#include "nepomukserverkcm.moc"
