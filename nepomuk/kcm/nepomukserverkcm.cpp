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

    void expandRecursively( const QModelIndex& index, QTreeView* view ) {
        if ( index.isValid() ) {
            view->expand( index );
            expandRecursively( index.parent(), view );
        }
    }

    bool isDirHidden( const QString& dir ) {
        QDir d( dir );
        while ( !d.isRoot() ) {
            if ( QFileInfo( d.path() ).isHidden() )
                return true;
            if ( !d.cdUp() )
                return false; // dir does not exist or is not readable
        }
        return false;
    }

    QStringList removeHiddenFolders( const QStringList& folders ) {
        QStringList newFolders( folders );
        for ( QStringList::iterator it = newFolders.begin(); it != newFolders.end(); /* do nothing here */ ) {
            if ( isDirHidden( *it ) ) {
                it = newFolders.erase( it );
            }
            else {
                ++it;
            }
        }
        return newFolders;
    }
}


Nepomuk::ServerConfigModule::ServerConfigModule( QWidget* parent, const QVariantList& args )
    : KCModule( NepomukConfigModuleFactory::componentData(), parent, args ),
      m_serverInterface( "org.kde.NepomukServer", "/nepomukserver", QDBusConnection::sessionBus() ),
      m_serviceManagerInterface( "org.kde.NepomukServer", "/servicemanager", QDBusConnection::sessionBus() ),
      m_strigiInterface( 0 ),
      m_failedToInitialize( false )
{
    KAboutData *about = new KAboutData(
        "kcm_nepomuk", 0, ki18n("Nepomuk Configuration Module"),
        KDE_VERSION_STRING, KLocalizedString(), KAboutData::License_GPL,
        ki18n("Copyright 2007 Sebastian Trüg"));
    about->addAuthor(ki18n("Sebastian Trüg"), KLocalizedString(), "trueg@kde.org");
    setAboutData(about);
    setButtons(Help|Apply|Default);
    setupUi( this );
    m_editStrigiExcludeFilters->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_folderModel = new FolderSelectionModel( m_viewIndexFolders );
    m_viewIndexFolders->setModel( m_folderModel );
    m_viewIndexFolders->setHeaderHidden( true );
    m_viewIndexFolders->header()->setStretchLastSection( false );
    m_viewIndexFolders->header()->setResizeMode( QHeaderView::ResizeToContents );
    m_viewIndexFolders->setRootIsDecorated( true );
    m_viewIndexFolders->setAnimated( true );
    m_viewIndexFolders->setRootIndex( m_folderModel->setRootPath( QDir::rootPath() ) );

    connect( m_checkEnableStrigi, SIGNAL( toggled(bool) ),
             this, SLOT( changed() ) );
    connect( m_checkEnableNepomuk, SIGNAL( toggled(bool) ),
             this, SLOT( changed() ) );
    connect( m_checkShowHiddenFolders, SIGNAL( toggled(bool) ),
             this, SLOT( changed() ) );
    connect( m_folderModel, SIGNAL( dataChanged(const QModelIndex&, const QModelIndex&) ),
             this, SLOT( changed() ) );
    connect( m_editStrigiExcludeFilters, SIGNAL( changed() ),
             this, SLOT( changed() ) );
    connect( m_sliderMemoryUsage, SIGNAL( valueChanged(int) ),
             this, SLOT( changed() ) );
    connect( m_checkIndexRemovableMedia, SIGNAL( toggled(bool) ),
             this, SLOT( changed() ) );
    connect( m_checkShowHiddenFolders, SIGNAL( toggled( bool ) ),
             m_folderModel, SLOT( setHiddenFoldersShown( bool ) ) );

    connect( QDBusConnection::sessionBus().interface(),
             SIGNAL( serviceOwnerChanged( const QString&, const QString&, const QString& ) ),
             this,
             SLOT( slotUpdateStrigiStatus() ) );

    recreateStrigiInterface();
}


Nepomuk::ServerConfigModule::~ServerConfigModule()
{
    delete m_strigiInterface;
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
    m_checkShowHiddenFolders->setChecked( strigiConfig.group( "General" ).readEntry( "index hidden folders", false ) );
    m_folderModel->setFolders( strigiConfig.group( "General" ).readPathEntry( "folders", defaultFolders() ),
                               strigiConfig.group( "General" ).readPathEntry( "exclude folders", QStringList() ) );
    m_editStrigiExcludeFilters->setItems( strigiConfig.group( "General" ).readEntry( "exclude filters", defaultExcludeFilters() ) );
    m_checkIndexRemovableMedia->setChecked( strigiConfig.group( "General" ).readEntry( "index newly mounted", false ) );

    KConfig serverConfig( "nepomukserverrc" );
    const int maxMem = qMax( 20, serverConfig.group( "main Settings" ).readEntry( "Maximum memory", 50 ) );
    m_sliderMemoryUsage->setValue( maxMem );
    m_editMemoryUsage->setValue( maxMem );

    // make sure we do not have a hidden folder to expand which would make QFileSystemModel crash
    // + it would be weird to have a hidden folder indexed but not shown
    if ( !m_checkShowHiddenFolders->isChecked() ) {
        foreach( const QString& dir, m_folderModel->includeFolders() + m_folderModel->excludeFolders() ) {
            if ( isDirHidden( dir ) ) {
                m_checkShowHiddenFolders->setChecked( true );
                break;
            }
        }
    }

    // make sure that the tree is expanded to show all selected items
    foreach( const QString& dir, m_folderModel->includeFolders() + m_folderModel->excludeFolders() ) {
        expandRecursively( m_folderModel->index( dir ), m_viewIndexFolders );
    }
    groupBox->setEnabled(m_checkEnableNepomuk->isChecked());
    recreateStrigiInterface();
    slotUpdateStrigiStatus();
    emit changed(false);
}


void Nepomuk::ServerConfigModule::save()
{
    QStringList includeFolders = m_folderModel->includeFolders();
    QStringList excludeFolders = m_folderModel->excludeFolders();

    // 0. remove all hidden dirs from the folder lists if hidden folders are not to be indexed
    if ( !m_checkShowHiddenFolders->isChecked() ) {
        includeFolders = removeHiddenFolders( includeFolders );
        excludeFolders = removeHiddenFolders( excludeFolders );
    }

    // 1. change the settings (in case the server is not running)
    KConfig config( "nepomukserverrc" );
    config.group( "Basic Settings" ).writeEntry( "Start Nepomuk", m_checkEnableNepomuk->isChecked() );
    config.group( "Service-nepomukstrigiservice" ).writeEntry( "autostart", m_checkEnableStrigi->isChecked() );
    config.group( "main Settings" ).writeEntry( "Maximum memory", m_sliderMemoryUsage->value() );


    // 2. update Strigi config
    KConfig strigiConfig( "nepomukstrigirc" );
    strigiConfig.group( "General" ).writePathEntry( "folders", includeFolders );
    strigiConfig.group( "General" ).writePathEntry( "exclude folders", excludeFolders );
    strigiConfig.group( "General" ).writeEntry( "exclude filters", m_editStrigiExcludeFilters->items() );
    strigiConfig.group( "General" ).writeEntry( "index hidden folders", m_checkShowHiddenFolders->isChecked() );
    strigiConfig.group( "General" ).writeEntry( "index newly mounted", m_checkIndexRemovableMedia->isChecked() );


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

    recreateStrigiInterface();
    slotUpdateStrigiStatus();

    emit changed(false);
}


void Nepomuk::ServerConfigModule::defaults()
{
    m_checkEnableStrigi->setChecked( true );
    m_checkEnableNepomuk->setChecked( true );
    m_checkShowHiddenFolders->setChecked( false );
    m_editStrigiExcludeFilters->setItems( defaultExcludeFilters() );
    m_folderModel->setFolders( defaultFolders(), QStringList() );
}


void Nepomuk::ServerConfigModule::slotUpdateStrigiStatus()
{
    if ( m_serviceManagerInterface.isServiceRunning( "nepomukstrigiservice") ) {
        if ( m_serviceManagerInterface.isServiceInitialized( "nepomukstrigiservice") ) {
            QString status = m_strigiInterface->userStatusString();
            if ( status.isEmpty() ) {
                m_labelStrigiStatus->setText( i18nc( "@info:status %1 is an error message returned by a dbus interface.",
                                                     "Failed to contact Strigi indexer (%1)",
                                                     m_strigiInterface->lastError().message() ) );
            }
            else {
                m_failedToInitialize = false;
                m_labelStrigiStatus->setText( status );
            }
        }
        else {
            m_failedToInitialize = true;
            m_labelStrigiStatus->setText( i18nc( "@info:status", "Strigi service failed to initialize, most likely due to an installation problem." ) );
        }
    }
    else if ( !m_failedToInitialize ) {
        m_labelStrigiStatus->setText( i18nc( "@info:status", "Strigi service not running." ) );
    }
}


void Nepomuk::ServerConfigModule::recreateStrigiInterface()
{
    delete m_strigiInterface;
    m_strigiInterface = new org::kde::nepomuk::Strigi( "org.kde.nepomuk.services.nepomukstrigiservice", "/nepomukstrigiservice", QDBusConnection::sessionBus() );
    connect( m_strigiInterface, SIGNAL( statusChanged() ),
             this, SLOT( slotUpdateStrigiStatus() ) );
}

#include "nepomukserverkcm.moc"
