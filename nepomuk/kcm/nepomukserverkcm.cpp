/* This file is part of the KDE Project
   Copyright (c) 2007-2010 Sebastian Trueg <trueg@kde.org>

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
#include "fileexcludefilters.h"
#include "../kioslaves/common/standardqueries.h"
#include "nepomukservicecontrolinterface.h"
#include "indexfolderselectiondialog.h"
#include "statuswidget.h"

#include <KPluginFactory>
#include <KPluginLoader>
#include <KAboutData>
#include <KSharedConfig>
#include <KMessageBox>

#include <Nepomuk/Query/QueryParser>
#include <Nepomuk/Query/FileQuery>

#include <QtGui/QRadioButton>
#include <QtGui/QInputDialog>
#include <QtGui/QPushButton>
#include <QtCore/QDir>
#include <QtDBus/QDBusServiceWatcher>

#include <Soprano/PluginManager>
#include <Soprano/Backend>


K_PLUGIN_FACTORY( NepomukConfigModuleFactory, registerPlugin<Nepomuk::ServerConfigModule>(); )
K_EXPORT_PLUGIN( NepomukConfigModuleFactory("kcm_nepomuk", "nepomuk") )


namespace {
    QStringList defaultFolders() {
        return QStringList() << QDir::homePath();
    }

    /**
     * Extracts the top level folders from the include list and optionally adds a hint about
     * subfolders being excluded to them.
     */
    QString buildFolderLabel( const QStringList& includeFolders, const QStringList& excludeFolders ) {
        QStringList sortedIncludeFolders( includeFolders );
        qSort( sortedIncludeFolders );
        QStringList topLevelFolders;
        Q_FOREACH( const QString& folder, sortedIncludeFolders ) {
            if ( topLevelFolders.isEmpty() ||
                 !folder.startsWith( topLevelFolders.first() ) ) {
                topLevelFolders.append( folder );
            }
        }
        QHash<QString, bool> topLevelFolderExcludeHash;
        Q_FOREACH( const QString& folder, topLevelFolders ) {
            topLevelFolderExcludeHash.insert( folder, false );
        }
        Q_FOREACH( const QString& folder, excludeFolders ) {
            QMutableHashIterator<QString, bool> it( topLevelFolderExcludeHash );
            while ( it.hasNext() ) {
                it.next();
                const QString topLevelFolder = it.key();
                if ( folder.startsWith( topLevelFolder ) ) {
                    it.setValue( true );
                    break;
                }
            }
        }
        QStringList labels;
        QHashIterator<QString, bool> it( topLevelFolderExcludeHash );
        while ( it.hasNext() ) {
            it.next();
            QString path = it.key();
            if ( KUrl( path ).equals( KUrl( QDir::homePath() ), KUrl::CompareWithoutTrailingSlash ) )
                path = i18nc( "'Home' as in 'Home path', i.e. /home/username",  "Home" );
            QString label = i18n( "<strong><filename>%1</filename></strong>", path );
            if ( it.value() )
                label += QLatin1String( " (" ) + i18n( "some subfolders excluded" ) + ')';
            labels << label;
        }
        return labels.join( QLatin1String( ", " ) );
    }
}


Nepomuk::ServerConfigModule::ServerConfigModule( QWidget* parent, const QVariantList& args )
    : KCModule( NepomukConfigModuleFactory::componentData(), parent, args ),
      m_serverInterface( 0 ),
      m_strigiInterface( 0 ),
      m_failedToInitialize( false )
{
    KAboutData *about = new KAboutData(
        "kcm_nepomuk", 0, ki18n("Nepomuk Configuration Module"),
        KDE_VERSION_STRING, KLocalizedString(), KAboutData::License_GPL,
        ki18n("Copyright 2007-2010 Sebastian Trüg"));
    about->addAuthor(ki18n("Sebastian Trüg"), KLocalizedString(), "trueg@kde.org");
    setAboutData(about);
    setButtons(Help|Apply|Default);

    const Soprano::Backend* virtuosoBackend = Soprano::PluginManager::instance()->discoverBackendByName( QLatin1String( "virtuoso" ) );
    m_nepomukAvailable = ( virtuosoBackend && virtuosoBackend->isAvailable() );

    if ( m_nepomukAvailable ) {
        setupUi( this );

        m_indexFolderSelectionDialog = new IndexFolderSelectionDialog( this );

        QDBusServiceWatcher * watcher = new QDBusServiceWatcher( this );
        watcher->addWatchedService( QLatin1String("org.kde.nepomuk.services.nepomukstrigiservice") );
        watcher->addWatchedService( QLatin1String("org.kde.NepomukServer") );
        watcher->setConnection( QDBusConnection::sessionBus() );
        watcher->setWatchMode( QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration );

        connect( watcher, SIGNAL( serviceRegistered(const QString&) ),
                 this, SLOT( recreateInterfaces() ) );
        connect( watcher, SIGNAL( serviceUnregistered(const QString&) ),
                 this, SLOT( recreateInterfaces() ) );

        recreateInterfaces();

        connect( m_checkEnableStrigi, SIGNAL( toggled(bool) ),
                 this, SLOT( changed() ) );
        connect( m_checkEnableNepomuk, SIGNAL( toggled(bool) ),
                 this, SLOT( changed() ) );
        connect( m_sliderMemoryUsage, SIGNAL( valueChanged(int) ),
                 this, SLOT( changed() ) );
        connect( m_checkIndexRemovableMedia, SIGNAL( toggled(bool) ),
                 this, SLOT( changed() ) );
        connect( m_spinMaxResults, SIGNAL( valueChanged( int ) ),
                 this, SLOT( changed() ) );
        connect( m_rootQueryButtonGroup, SIGNAL( buttonClicked( int ) ),
                 this, SLOT( changed() ) );

        connect( m_checkRootQueryCustom, SIGNAL( toggled(bool) ),
                 this, SLOT( slotCustomQueryToggled( bool ) ) );
        connect( m_buttonEditCustomQuery, SIGNAL( leftClickedUrl() ),
                 this, SLOT( slotCustomQueryButtonClicked() ) );
        connect( m_buttonCustomizeIndexFolders, SIGNAL( leftClickedUrl() ),
                 this, SLOT( slotEditIndexFolders() ) );
        connect( m_buttonDetails, SIGNAL( leftClickedUrl() ),
                 this, SLOT( slotStatusDetailsClicked() ) );

        m_customQueryLabel->hide();
        m_buttonEditCustomQuery->hide();
    }
    else {
        QLabel* label = new QLabel( i18n( "The Nepomuk installation is not complete. No Nepomuk settings can be provided." ) );
        label->setAlignment( Qt::AlignCenter );
        QVBoxLayout* layout = new QVBoxLayout( this );
        layout->addWidget( label );
    }
}


Nepomuk::ServerConfigModule::~ServerConfigModule()
{
    delete m_strigiInterface;
    delete m_serverInterface;
}


void Nepomuk::ServerConfigModule::load()
{
    if ( !m_nepomukAvailable )
        return;

    // 1. basic setup
    KConfig config( "nepomukserverrc" );
    m_checkEnableNepomuk->setChecked( config.group( "Basic Settings" ).readEntry( "Start Nepomuk", true ) );
    m_checkEnableStrigi->setChecked( config.group( "Service-nepomukstrigiservice" ).readEntry( "autostart", true ) );


    // 2. strigi settings
    KConfig strigiConfig( "nepomukstrigirc" );
    m_indexFolderSelectionDialog->setIndexHiddenFolders( strigiConfig.group( "General" ).readEntry( "index hidden folders", false ) );
    m_indexFolderSelectionDialog->setFolders( strigiConfig.group( "General" ).readPathEntry( "folders", defaultFolders() ),
                                              strigiConfig.group( "General" ).readPathEntry( "exclude folders", QStringList() ) );
    m_indexFolderSelectionDialog->setExcludeFilters( strigiConfig.group( "General" ).readEntry( "exclude filters", Nepomuk::defaultExcludeFilterList() ) );
    m_checkIndexRemovableMedia->setChecked( strigiConfig.group( "General" ).readEntry( "index newly mounted", false ) );

    groupBox->setEnabled(m_checkEnableNepomuk->isChecked());

    // 3. storage service
    KConfig serverConfig( "nepomukserverrc" );
    const int maxMem = qMax( 20, serverConfig.group( "main Settings" ).readEntry( "Maximum memory", 50 ) );
    m_sliderMemoryUsage->setValue( maxMem );
    m_editMemoryUsage->setValue( maxMem );


    // 4. kio_nepomuksearch settings
    KConfig kio_nepomuksearchConfig( "kio_nepomuksearchrc" );
    KConfigGroup kio_nepomuksearchGeneral = kio_nepomuksearchConfig.group( "General" );
    m_spinMaxResults->setValue( kio_nepomuksearchGeneral.readEntry( "Default limit", 10 ) );

    // this is a temp solution until we have a proper query builder
    m_customQuery = kio_nepomuksearchGeneral.readEntry( "Custom query", QString() );
    m_customQueryLabel->setText( m_customQuery );

    buttonForQuery( Query::Query::fromString(
                        kio_nepomuksearchGeneral.readEntry(
                            "Root query",
                            Nepomuk::lastModifiedFilesQuery().toString() ) ) )->setChecked( true );



    // 5. update state
    m_labelIndexFolders->setText( buildFolderLabel( m_indexFolderSelectionDialog->includeFolders(),
                                                    m_indexFolderSelectionDialog->excludeFolders() ) );
    recreateInterfaces();
    updateStrigiStatus();
    updateNepomukServerStatus();

    // 6. all values loaded -> no changes
    emit changed(false);
}


void Nepomuk::ServerConfigModule::save()
{
    if ( !m_nepomukAvailable )
        return;

    QStringList includeFolders = m_indexFolderSelectionDialog->includeFolders();
    QStringList excludeFolders = m_indexFolderSelectionDialog->excludeFolders();

    // 1. change the settings (in case the server is not running)
    KConfig config( "nepomukserverrc" );
    config.group( "Basic Settings" ).writeEntry( "Start Nepomuk", m_checkEnableNepomuk->isChecked() );
    config.group( "Service-nepomukstrigiservice" ).writeEntry( "autostart", m_checkEnableStrigi->isChecked() );
    config.group( "main Settings" ).writeEntry( "Maximum memory", m_sliderMemoryUsage->value() );


    // 2. update Strigi config
    KConfig strigiConfig( "nepomukstrigirc" );
    strigiConfig.group( "General" ).writePathEntry( "folders", includeFolders );
    strigiConfig.group( "General" ).writePathEntry( "exclude folders", excludeFolders );
    strigiConfig.group( "General" ).writeEntry( "exclude filters", m_indexFolderSelectionDialog->excludeFilters() );
    strigiConfig.group( "General" ).writeEntry( "index hidden folders", m_indexFolderSelectionDialog->indexHiddenFolders() );
    strigiConfig.group( "General" ).writeEntry( "index newly mounted", m_checkIndexRemovableMedia->isChecked() );


    // 3. update the current state of the nepomuk server
    if ( m_serverInterface->isValid() ) {
        m_serverInterface->enableNepomuk( m_checkEnableNepomuk->isChecked() );
        m_serverInterface->enableStrigi( m_checkEnableStrigi->isChecked() );
    }
    else {
        if ( !QProcess::startDetached( QLatin1String( "nepomukserver" ) ) ) {
            KMessageBox::error( this,
                                i18n( "Failed to start Nepomuk Server. The settings have been saved "
                                      "and will be used the next time the server is started." ),
                                i18n( "Nepomuk server not running" ) );
        }
    }


    // 4. update kio_nepomuksearch config
    KConfig kio_nepomuksearchConfig( "kio_nepomuksearchrc" );
    kio_nepomuksearchConfig.group( "General" ).writeEntry( "Default limit", m_spinMaxResults->value() );
    kio_nepomuksearchConfig.group( "General" ).writeEntry( "Root query", queryForButton( m_rootQueryButtonGroup->checkedButton() ).toString() );
    kio_nepomuksearchConfig.group( "General" ).writeEntry( "Custom query", m_customQuery );


    // 5. update state
    recreateInterfaces();
    updateStrigiStatus();
    updateNepomukServerStatus();


    // 6. all values saved -> no changes
    emit changed(false);
}


void Nepomuk::ServerConfigModule::defaults()
{
    if ( !m_nepomukAvailable )
        return;

    m_checkEnableStrigi->setChecked( true );
    m_checkEnableNepomuk->setChecked( true );
    m_indexFolderSelectionDialog->setIndexHiddenFolders( false );
    m_indexFolderSelectionDialog->setExcludeFilters( Nepomuk::defaultExcludeFilterList() );
    m_indexFolderSelectionDialog->setFolders( defaultFolders(), QStringList() );
    m_spinMaxResults->setValue( 10 );
    m_checkRootQueryLastModified->setChecked( true );
}


void Nepomuk::ServerConfigModule::updateNepomukServerStatus()
{
    if ( m_serverInterface &&
         m_serverInterface->isNepomukEnabled() ) {
        m_labelNepomukStatus->setText( i18nc( "@info:status", "Nepomuk system is active" ) );
    }
    else {
        m_labelNepomukStatus->setText( i18nc( "@info:status", "Nepomuk system is inactive" ) );
    }
}


void Nepomuk::ServerConfigModule::updateStrigiStatus()
{
    if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.kde.nepomuk.services.nepomukstrigiservice" ) ) {
        if ( org::kde::nepomuk::ServiceControl( "org.kde.nepomuk.services.nepomukstrigiservice", "/servicecontrol", QDBusConnection::sessionBus() ).isInitialized() ) {
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


void Nepomuk::ServerConfigModule::recreateInterfaces()
{
    delete m_strigiInterface;
    delete m_serverInterface;

    m_strigiInterface = new org::kde::nepomuk::Strigi( "org.kde.nepomuk.services.nepomukstrigiservice", "/nepomukstrigiservice", QDBusConnection::sessionBus() );
    m_serverInterface = new org::kde::NepomukServer( "org.kde.NepomukServer", "/nepomukserver", QDBusConnection::sessionBus() );

    connect( m_strigiInterface, SIGNAL( statusChanged() ),
             this, SLOT( updateStrigiStatus() ) );
}


void Nepomuk::ServerConfigModule::slotCustomQueryButtonClicked()
{
    // this is a temp solution until we have a proper query builder
    QString queryString = QInputDialog::getText( this,
                                                 i18n( "Custom root folder query" ),
                                                 i18n( "Please enter a query to be listed in the root folder" ),
                                                 QLineEdit::Normal,
                                                 m_customQuery );
    if ( !queryString.isEmpty() ) {
        m_customQuery = queryString;
        m_customQueryLabel->setText( queryString );
        changed();
    }
}


void Nepomuk::ServerConfigModule::slotStatusDetailsClicked()
{
    StatusWidget statusDialog( this );
    statusDialog.exec();
}


void Nepomuk::ServerConfigModule::slotEditIndexFolders()
{
    const QStringList oldIncludeFolders = m_indexFolderSelectionDialog->includeFolders();
    const QStringList oldExcludeFolders = m_indexFolderSelectionDialog->excludeFolders();
    const QStringList oldExcludeFilters = m_indexFolderSelectionDialog->excludeFilters();
    const bool oldIndexHidden = m_indexFolderSelectionDialog->indexHiddenFolders();

    if ( m_indexFolderSelectionDialog->exec() ) {
        m_labelIndexFolders->setText( buildFolderLabel( m_indexFolderSelectionDialog->includeFolders(),
                                                        m_indexFolderSelectionDialog->excludeFolders() ) );
        changed();
    }
    else {
        // revert to previous settings
        m_indexFolderSelectionDialog->setFolders( oldIncludeFolders, oldExcludeFolders );
        m_indexFolderSelectionDialog->setExcludeFilters( oldExcludeFilters );
        m_indexFolderSelectionDialog->setIndexHiddenFolders( oldIndexHidden );
    }
}


void Nepomuk::ServerConfigModule::slotCustomQueryToggled( bool on )
{
    if ( on && m_customQuery.isEmpty() ) {
        slotCustomQueryButtonClicked();
    }
}


QRadioButton* Nepomuk::ServerConfigModule::buttonForQuery( const Query::Query& query ) const
{
    if ( query == Nepomuk::neverOpenedFilesQuery() )
        return m_checkRootQueryNeverOpened;
    else if ( query == Nepomuk::lastModifiedFilesQuery() )
        return m_checkRootQueryLastModified;
    else if ( query == Nepomuk::mostImportantFilesQuery() )
        return m_checkRootQueryFancy;
    else
        return m_checkRootQueryCustom;
}


Nepomuk::Query::Query Nepomuk::ServerConfigModule::queryForButton( QAbstractButton* button ) const
{
    if ( button == m_checkRootQueryNeverOpened )
        return Nepomuk::neverOpenedFilesQuery();
    else if ( button == m_checkRootQueryLastModified )
        return Nepomuk::lastModifiedFilesQuery();
    else if ( button == m_checkRootQueryFancy )
        return Nepomuk::mostImportantFilesQuery();
    else {
        // force to always only query for files
        Nepomuk::Query::FileQuery query = Query::QueryParser::parseQuery( m_customQuery );
        query.setFileMode( Query::FileQuery::QueryFiles );
        return query;
    }
}

#include "nepomukserverkcm.moc"
