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
#include "nepomukserverinterface.h"
#include "folderselectionmodel.h"

#include <KPluginFactory>
#include <KPluginLoader>
#include <KAboutData>
#include <KSharedConfig>
#include <KLed>
#include <KMessageBox>

#include <QtGui/QTreeView>

#include <Soprano/PluginManager>


K_PLUGIN_FACTORY( NepomukConfigModuleFactory, registerPlugin<Nepomuk::ServerConfigModule>(); )
K_EXPORT_PLUGIN( NepomukConfigModuleFactory("kcm_nepomuk", "nepomuk") )


namespace {
    QStringList defaultFolders() {
        return QStringList() << QDir::homePath();
    }

    QStringList defaultExcludeFilters() {
        return QStringList() << ".*/" << ".*" << "*~" << "*.part";
    }
}


Nepomuk::ServerConfigModule::ServerConfigModule( QWidget* parent, const QVariantList& args )
    : KCModule( NepomukConfigModuleFactory::componentData(), parent, args ),
      m_serverInterface( "org.kde.NepomukServer", "/nepomukserver", QDBusConnection::sessionBus() ),
      m_strigiInterface( "org.kde.nepomuk.services.nepomukstrigiservice", "/nepomukstrigiservice", QDBusConnection::sessionBus() )
{
    KAboutData *about = new KAboutData(
        "kcm_nepomuk", 0, ki18n("Nepomuk Configuration Module"),
        KDE_VERSION_STRING, KLocalizedString(), KAboutData::License_GPL,
        ki18n("Copyright 2007 Sebastian Trüg"));
    about->addAuthor(ki18n("Sebastian Trüg"), KLocalizedString(), "trueg@kde.org");
    setAboutData(about);
    setButtons(Apply|Default);
    setupUi( this );

    m_folderModel = new FolderSelectionModel( m_viewIndexFolders );
    m_viewIndexFolders->setModel( m_folderModel );
    m_viewIndexFolders->setHeaderHidden( true );
    m_viewIndexFolders->setRootIsDecorated( true );
    m_viewIndexFolders->setAnimated( true );
    m_viewIndexFolders->setRootIndex( m_folderModel->setRootPath( QDir::rootPath() ) );

    connect( m_checkEnableStrigi, SIGNAL( toggled(bool) ),
             this, SLOT( changed() ) );
    connect( m_checkEnableNepomuk, SIGNAL( toggled(bool) ),
             this, SLOT( changed() ) );
    connect( m_folderModel, SIGNAL( dataChanged(const QModelIndex&, const QModelIndex&) ),
             this, SLOT( changed() ) );
    connect( m_editStrigiExcludeFilters, SIGNAL( changed() ),
             this, SLOT( changed() ) );

    connect( &m_strigiInterface, SIGNAL( indexingStarted() ),
             this, SLOT( slotUpdateStrigiStatus() ) );
    connect( &m_strigiInterface, SIGNAL( indexingStopped() ),
             this, SLOT( slotUpdateStrigiStatus() ) );
    connect( &m_strigiInterface, SIGNAL( indexingFolder(QString) ),
             this, SLOT( slotUpdateStrigiStatus() ) );

    load();
}


Nepomuk::ServerConfigModule::~ServerConfigModule()
{
}


void Nepomuk::ServerConfigModule::load()
{
    bool sopranoBackendAvailable = !Soprano::PluginManager::instance()->allBackends().isEmpty();

    m_checkEnableNepomuk->setEnabled( sopranoBackendAvailable );

    if ( !sopranoBackendAvailable ) {
        KMessageBox::sorry( this,
                            i18n( "No Soprano Database backend available. Please check your installation." ),
                            i18n( "Nepomuk cannot be started" ) );
    }
    else if ( m_serverInterface.isValid() ) {
        m_checkEnableStrigi->setChecked( m_serverInterface.isStrigiEnabled().value() );
        m_checkEnableNepomuk->setChecked( m_serverInterface.isNepomukEnabled().value() );
    }
    else {
        KMessageBox::sorry( this,
                            i18n( "The Nepomuk Server is not running. The settings "
                                  "will be used the next time the server is started." ),
                            i18n( "Nepomuk server not running" ) );

        KConfig config( "nepomukserverrc" );
        m_checkEnableNepomuk->setChecked( config.group( "Basic Settings" ).readEntry( "Start Nepomuk", true ) );
        m_checkEnableStrigi->setChecked( config.group( "Service-nepomukstrigiservice" ).readEntry( "autostart", true ) );
    }

    KConfig strigiConfig( "nepomukstrigirc" );
    m_folderModel->setFolders( strigiConfig.group( "General" ).readPathEntry( "folders", defaultFolders() ) );
    m_editStrigiExcludeFilters->setItems( strigiConfig.group( "General" ).readEntry( "exclude filters", defaultExcludeFilters() ) );

    // make sure that the tree is expanded to show all selected items
    foreach( const QString& dir, m_folderModel->folders() ) {
        QModelIndex index = m_folderModel->index( dir );
        m_viewIndexFolders->scrollTo( index, QAbstractItemView::EnsureVisible );
    }

    slotUpdateStrigiStatus();
}


void Nepomuk::ServerConfigModule::save()
{
    // 1. change the settings (in case the server is not running)
    KConfig config( "nepomukserverrc" );
    config.group( "Basic Settings" ).writeEntry( "Start Nepomuk", m_checkEnableNepomuk->isChecked() );
    config.group( "Service-nepomukstrigiservice" ).writeEntry( "autostart", m_checkEnableStrigi->isChecked() );


    // 2. update Strigi config
    KConfig strigiConfig( "nepomukstrigirc" );
    strigiConfig.group( "General" ).writePathEntry( "folders", m_folderModel->folders() );
    strigiConfig.group( "General" ).writeEntry( "exclude filters", m_editStrigiExcludeFilters->items() );


    // 3. update the current state of the nepomuk server
    if ( m_serverInterface.isValid() ) {
        m_serverInterface.enableNepomuk( m_checkEnableNepomuk->isChecked() );
        m_serverInterface.enableStrigi( m_checkEnableStrigi->isChecked() );
    }
    else {
        KMessageBox::sorry( this,
                            i18n( "The Nepomuk Server is not running. The settings have been saved "
                                  "and will be used the next time the server is started." ),
                            i18n( "Nepomuk server not running" ) );
    }

    slotUpdateStrigiStatus();
}


void Nepomuk::ServerConfigModule::defaults()
{
    m_checkEnableStrigi->setChecked( true );
    m_checkEnableNepomuk->setChecked( true );
    m_editStrigiExcludeFilters->setItems( defaultExcludeFilters() );
    m_folderModel->setFolders( defaultFolders() );
}


void Nepomuk::ServerConfigModule::slotUpdateStrigiStatus()
{
    if ( m_strigiInterface.isValid() ) {
        bool indexing = m_strigiInterface.isIndexing();
        bool suspended = m_strigiInterface.isSuspended();
        QString folder = m_strigiInterface.currentFolder();

        if ( m_strigiInterface.lastError().isValid() )
            m_labelStrigiStatus->setText( i18nc( "@info:status %1 is an error message returned by a dbus interface.",
                                                 "Failed to contact Strigi indexer (%1)",
                                                 m_strigiInterface.lastError().message() ) );
        else if ( suspended )
            m_labelStrigiStatus->setText( i18nc( "@info_status", "File indexer is suspended" ) );
        else if ( indexing )
            m_labelStrigiStatus->setText( i18nc( "@info_status", "Strigi is currently indexing files in folder %1", folder ) );
        else
            m_labelStrigiStatus->setText( i18nc( "@info_status", "File indexer is idle" ) );
    }
    else {
        m_labelStrigiStatus->setText( i18nc( "@info_status", "Strigi service not running." ) );
    }
}

#include "nepomukserverkcm.moc"
