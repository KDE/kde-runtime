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
#include "servicecontrol.h"
#include "fileexcludefilters.h"
#include "../kioslaves/common/standardqueries.h"
#include "fileindexerinterface.h"
#include "indexfolderselectiondialog.h"
#include "excludefilterselectiondialog.h"
#include "statuswidget.h"

#include <KPluginFactory>
#include <KPluginLoader>
#include <KAboutData>
#include <KSharedConfig>
#include <KMessageBox>
#include <KUrlLabel>
#include <KProcess>
#include <KStandardDirs>
#include <KCalendarSystem>
#include <KDirWatch>
#include <KDebug>

#include <Nepomuk2/Query/QueryParser>
#include <Nepomuk2/Query/FileQuery>

#include <QRadioButton>
#include <QInputDialog>
#include <QPushButton>
#include <QtCore/QDir>
#include <QtDBus/QDBusServiceWatcher>

#include <Soprano/PluginManager>
#include <Soprano/Backend>


K_PLUGIN_FACTORY( NepomukConfigModuleFactory, registerPlugin<Nepomuk2::ServerConfigModule>(); )
K_EXPORT_PLUGIN( NepomukConfigModuleFactory("kcm_nepomuk", "kcm_nepomuk") )


namespace {
    QStringList defaultFolders() {
        return QStringList() << QDir::homePath();
    }

    enum BackupFrequency {
        DisableAutomaticBackups = 0,
        DailyBackup = 1,
        WeeklyBackup = 2
    };


    QString backupFrequencyToString( BackupFrequency freq ) {
        switch( freq ) {
        case DailyBackup:
            return QLatin1String("daily");
        case WeeklyBackup:
            return QLatin1String("weekly");
        default:
            return QLatin1String("disabled");
        }
    }

    BackupFrequency parseBackupFrequency( const QString& s ) {
        for( int i = 0; i < 4; ++i ) {
            if( s == backupFrequencyToString(BackupFrequency(i)) )
                return BackupFrequency(i);
        }
        return DisableAutomaticBackups;
    }

}



Nepomuk2::ServerConfigModule::ServerConfigModule( QWidget* parent, const QVariantList& args )
    : KCModule( NepomukConfigModuleFactory::componentData(), parent, args ),
      m_serverInterface( 0 ),
      m_fileIndexerInterface( 0 ),
      m_failedToInitialize( false ),
      m_checkboxesChanged( false )
{
    KAboutData *about = new KAboutData(
        "kcm_nepomuk", "kcm_nepomuk", ki18n("Desktop Search Configuration Module"),
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
        m_excludeFilterSelectionDialog = new ExcludeFilterSelectionDialog( this );

        QDBusServiceWatcher * watcher = new QDBusServiceWatcher( this );
        watcher->addWatchedService( QLatin1String("org.kde.nepomuk.services.nepomukfileindexer") );
        watcher->addWatchedService( QLatin1String("org.kde.NepomukServer") );
        watcher->setConnection( QDBusConnection::sessionBus() );
        watcher->setWatchMode( QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration );

        connect( watcher, SIGNAL( serviceRegistered(const QString&) ),
                 this, SLOT( recreateInterfaces() ) );
        connect( watcher, SIGNAL( serviceUnregistered(const QString&) ),
                 this, SLOT( recreateInterfaces() ) );

        recreateInterfaces();

        connect( m_checkEnableFileIndexer, SIGNAL( toggled(bool) ),
                 this, SLOT( changed() ) );
        connect( m_checkEnableNepomuk, SIGNAL( toggled(bool) ),
                 this, SLOT( changed() ) );
        connect( m_checkEnableEmailIndexer, SIGNAL( toggled(bool) ),
                 this, SLOT( changed() ) );
        connect( m_sliderMemoryUsage, SIGNAL( valueChanged(int) ),
                 this, SLOT( changed() ) );
        connect( m_comboRemovableMediaHandling, SIGNAL( activated(int) ),
                 this, SLOT( changed() ) );

        connect( m_buttonCustomizeIndexFolders, SIGNAL( clicked() ),
                 this, SLOT( slotEditIndexFolders() ) );
        connect( m_buttonAdvancedFileIndexing, SIGNAL( clicked() ),
                 this, SLOT( slotAdvancedFileIndexing() ) );
        connect( m_buttonDetails, SIGNAL( leftClickedUrl() ),
                 this, SLOT( slotStatusDetailsClicked() ) );

        connect( m_checkboxAudio, SIGNAL(toggled(bool)),
                 this, SLOT(slotCheckBoxesChanged()) );
        connect( m_checkboxImage, SIGNAL(toggled(bool)),
                 this, SLOT(slotCheckBoxesChanged()) );
        connect( m_checkboxVideo, SIGNAL(toggled(bool)),
                 this, SLOT(slotCheckBoxesChanged()) );
        connect( m_checkboxDocuments, SIGNAL(toggled(bool)),
                 this, SLOT(slotCheckBoxesChanged()) );
        connect( m_checkboxSourceCode, SIGNAL(toggled(bool)),
                 this, SLOT(slotCheckBoxesChanged()) );

        // Backup
        m_comboBackupFrequency->addItem(i18nc("@item:inlistbox", "Disable Automatic Backups"));
        m_comboBackupFrequency->addItem(i18nc("@item:inlistbox", "Daily Backup"));
        m_comboBackupFrequency->addItem(i18nc("@item:inlistbox", "Weekly Backup"));

        for( int i = 1; i <= 7; ++i )
            m_comboBackupDay->addItem(KGlobal::locale()->calendar()->weekDayName(i), i);

        connect( m_comboBackupFrequency, SIGNAL(currentIndexChanged(int)),
                 this, SLOT(slotBackupFrequencyChanged()) );
        connect( m_comboBackupFrequency, SIGNAL(currentIndexChanged(int)),
                 this, SLOT(changed()) );
        connect( m_comboBackupDay, SIGNAL(currentIndexChanged(int)),
                 this, SLOT(changed()) );
        connect( m_editBackupTime, SIGNAL(timeChanged(QTime)),
                 this, SLOT(changed()) );
        connect( m_spinBackupMax, SIGNAL(valueChanged(int)),
                 this, SLOT(changed()) );

        connect( m_buttonManualBackup, SIGNAL(clicked(bool)),
                 this, SLOT(slotManualBackup()) );
        connect( m_buttonRestoreBackup, SIGNAL(clicked(bool)),
                 this, SLOT(slotRestoreBackup()) );

        // update backup status whenever manual backups are created
        KDirWatch::self()->addDir( KStandardDirs::locateLocal( "data", "nepomuk/backupsync/backups/" ) );
        connect( KDirWatch::self(), SIGNAL(dirty(QString)), this, SLOT(updateBackupStatus()) );

        // args[0] can be the page index allowing to open the config with a specific page
        if(args.count() > 0 && args[0].toInt() < m_mainTabWidget->count()) {
            m_mainTabWidget->setCurrentIndex(args[0].toInt());
        }
    }
    else {
        QLabel* label = new QLabel( i18n( "The Nepomuk installation is not complete. No desktop search settings can be provided." ) );
        label->setAlignment( Qt::AlignCenter );
        QVBoxLayout* layout = new QVBoxLayout( this );
        layout->addWidget( label );
    }
}


Nepomuk2::ServerConfigModule::~ServerConfigModule()
{
    delete m_fileIndexerInterface;
    delete m_serverInterface;
}


void Nepomuk2::ServerConfigModule::load()
{
    if ( !m_nepomukAvailable )
        return;

    // 1. basic setup
    KConfig config( "nepomukserverrc" );
    m_checkEnableNepomuk->setChecked( config.group( "Basic Settings" ).readEntry( "Start Nepomuk", true ) );
    m_checkEnableFileIndexer->setChecked( config.group( "Service-nepomukfileindexer" ).readEntry( "autostart", true ) );

    const QString akonadiCtlExe = KStandardDirs::findExe( "akonadictl" );
    m_emailIndexingBox->setVisible( !akonadiCtlExe.isEmpty() );
    KConfig akonadiConfig( "akonadi_nepomuk_feederrc" );
    m_checkEnableEmailIndexer->setChecked( akonadiConfig.group( "akonadi_nepomuk_email_feeder" ).readEntry( "Enabled", true ) );

    // 2. file indexer settings
    KConfig fileIndexerConfig( "nepomukstrigirc" );
    m_indexFolderSelectionDialog->setIndexHiddenFolders( fileIndexerConfig.group( "General" ).readEntry( "index hidden folders", false ) );
    m_indexFolderSelectionDialog->setFolders( fileIndexerConfig.group( "General" ).readPathEntry( "folders", defaultFolders() ),
                                              fileIndexerConfig.group( "General" ).readPathEntry( "exclude folders", QStringList() ) );

    m_excludeFilterSelectionDialog->setExcludeFilters( fileIndexerConfig.group( "General" ).readEntry( "exclude filters", Nepomuk2::defaultExcludeFilterList() ) );

    // MimeTypes
    QStringList mimetypes = fileIndexerConfig.group( "General" ).readEntry( "exclude mimetypes", defaultExcludeMimetypes() );
    m_excludeFilterSelectionDialog->setExcludeMimeTypes( mimetypes );
    syncCheckBoxesFromMimetypes( mimetypes );

    const bool indexNewlyMounted = fileIndexerConfig.group( "RemovableMedia" ).readEntry( "index newly mounted", false );
    const bool askIndividually = fileIndexerConfig.group( "RemovableMedia" ).readEntry( "ask user", false );
    // combobox items: 0 - ignore, 1 - index all, 2 - ask user
    m_comboRemovableMediaHandling->setCurrentIndex(int(indexNewlyMounted) + int(askIndividually));

    groupBox->setEnabled(m_checkEnableNepomuk->isChecked());

    // 3. storage service
    KConfig serverConfig( "nepomukserverrc" );
    const int maxMem = qMax( 20, serverConfig.group( "main Settings" ).readEntry( "Maximum memory", 50 ) );
    m_sliderMemoryUsage->setValue( maxMem );
    m_editMemoryUsage->setValue( maxMem );


    // 4. Backup settings
    KConfig backupConfig( "nepomukbackuprc" );
    KConfigGroup backupCfg = backupConfig.group("Backup");
    m_comboBackupFrequency->setCurrentIndex(parseBackupFrequency(backupCfg.readEntry("backup frequency", "disabled")));
    m_editBackupTime->setTime( QTime::fromString( backupCfg.readEntry("backup time", "18:00:00"), Qt::ISODate ) );
    m_comboBackupDay->setCurrentIndex( backupCfg.readEntry("backup day", 1) - 1 );
    m_spinBackupMax->setValue( backupCfg.readEntry("max backups", 10) );

    slotBackupFrequencyChanged();
    updateBackupStatus();

    recreateInterfaces();
    updateFileIndexerStatus();
    updateNepomukServerStatus();

    // 7. all values loaded -> no changes
    m_checkboxesChanged = false;
    emit changed(false);
}


void Nepomuk2::ServerConfigModule::save()
{
    if ( !m_nepomukAvailable )
        return;

    QStringList includeFolders = m_indexFolderSelectionDialog->includeFolders();
    QStringList excludeFolders = m_indexFolderSelectionDialog->excludeFolders();

    // 1. change the settings (in case the server is not running)
    KConfig config( "nepomukserverrc" );
    config.group( "Basic Settings" ).writeEntry( "Start Nepomuk", m_checkEnableNepomuk->isChecked() );
    config.group( "Service-nepomukfileindexer" ).writeEntry( "autostart", m_checkEnableFileIndexer->isChecked() );
    config.group( "main Settings" ).writeEntry( "Maximum memory", m_sliderMemoryUsage->value() );

    KConfig akonadiConfig( "akonadi_nepomuk_feederrc" );
    akonadiConfig.group( "akonadi_nepomuk_email_feeder" ).writeEntry( "Enabled", m_checkEnableEmailIndexer->isChecked() );
    akonadiConfig.sync();
    QDBusInterface akonadiIface( "org.freedesktop.Akonadi.Agent.akonadi_nepomuk_email_feeder", "/", "org.freedesktop.Akonadi.Agent.Control" );
    akonadiIface.asyncCall( "reconfigure" );

    // 2. update file indexer config
    KConfig fileIndexerConfig( "nepomukstrigirc" );
    fileIndexerConfig.group( "General" ).writePathEntry( "folders", includeFolders );
    fileIndexerConfig.group( "General" ).writePathEntry( "exclude folders", excludeFolders );
    fileIndexerConfig.group( "General" ).writeEntry( "index hidden folders", m_indexFolderSelectionDialog->indexHiddenFolders() );
    fileIndexerConfig.group( "General" ).writeEntry( "exclude filters", m_excludeFilterSelectionDialog->excludeFilters() );

    QStringList excludeMimetypes = m_excludeFilterSelectionDialog->excludeMimeTypes();
    if( m_checkboxesChanged ) {
        excludeMimetypes = mimetypesFromCheckboxes();
        m_checkboxesChanged = false;
    }

    fileIndexerConfig.group( "General" ).writeEntry( "exclude mimetypes", excludeMimetypes );

    // combobox items: 0 - ignore, 1 - index all, 2 - ask user
    fileIndexerConfig.group( "RemovableMedia" ).writeEntry( "index newly mounted", m_comboRemovableMediaHandling->currentIndex() > 0 );
    fileIndexerConfig.group( "RemovableMedia" ).writeEntry( "ask user", m_comboRemovableMediaHandling->currentIndex() == 2 );

    // 3. Update backup config
    KConfig backup("nepomukbackuprc");
    KConfigGroup backupCfg = backup.group("Backup");
    backupCfg.writeEntry("backup frequency", backupFrequencyToString(BackupFrequency(m_comboBackupFrequency->currentIndex())));
    backupCfg.writeEntry("backup day", m_comboBackupDay->itemData(m_comboBackupDay->currentIndex()).toInt());
    backupCfg.writeEntry("backup time", m_editBackupTime->time().toString(Qt::ISODate));
    backupCfg.writeEntry("max backups", m_spinBackupMax->value());


    // 4. update the current state of the nepomuk server
    if ( m_serverInterface->isValid() ) {
        m_serverInterface->enableNepomuk( m_checkEnableNepomuk->isChecked() );
        m_serverInterface->enableFileIndexer( m_checkEnableFileIndexer->isChecked() );
    }
    else if( m_checkEnableNepomuk->isChecked() ) {
        if ( !QProcess::startDetached( QLatin1String( "nepomukserver" ) ) ) {
            KMessageBox::error( this,
                                i18n( "Failed to start the desktop search service (Nepomuk). The settings have been saved "
                                      "and will be used the next time the server is started." ),
                                i18n( "Desktop search service not running" ) );
        }
    }


    // 5. update state
    recreateInterfaces();
    updateFileIndexerStatus();
    updateNepomukServerStatus();


    // 6. all values saved -> no changes
    m_checkboxesChanged = false;
    emit changed(false);
}


void Nepomuk2::ServerConfigModule::defaults()
{
    if ( !m_nepomukAvailable )
        return;

    m_checkEnableFileIndexer->setChecked( true );
    m_checkEnableNepomuk->setChecked( true );
    m_checkEnableEmailIndexer->setChecked( true );
    m_indexFolderSelectionDialog->setIndexHiddenFolders( false );
    m_indexFolderSelectionDialog->setFolders( defaultFolders(), QStringList() );
    m_excludeFilterSelectionDialog->setExcludeFilters( Nepomuk2::defaultExcludeFilterList() );

    // FIXME: set backup config
}


void Nepomuk2::ServerConfigModule::updateNepomukServerStatus()
{
    if ( m_serverInterface &&
         m_serverInterface->isNepomukEnabled() ) {
        m_labelNepomukStatus->setText( i18nc( "@info:status", "Desktop search services are active" ) );
    }
    else {
        m_labelNepomukStatus->setText( i18nc( "@info:status", "Desktop search services are disabled" ) );
    }
}


void Nepomuk2::ServerConfigModule::setFileIndexerStatusText( const QString& text, bool elide )
{
    m_labelFileIndexerStatus->setWordWrap( !elide );
    m_labelFileIndexerStatus->setTextElideMode( elide ? Qt::ElideMiddle : Qt::ElideNone );
    m_labelFileIndexerStatus->setText( text );
}


void Nepomuk2::ServerConfigModule::updateFileIndexerStatus()
{
    if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.kde.nepomuk.services.nepomukfileindexer" ) ) {
        if ( org::kde::nepomuk::ServiceControl( "org.kde.nepomuk.services.nepomukfileindexer", "/servicecontrol",
                                                 QDBusConnection::sessionBus() ).isInitialized() ) {
            QString status = m_fileIndexerInterface->userStatusString();
            if ( status.isEmpty() ) {
                setFileIndexerStatusText( i18nc( "@info:status %1 is an error message returned by a dbus interface.",
                                                 "Failed to contact File Indexer service (%1)",
                                                 m_fileIndexerInterface->lastError().message() ), false );
            }
            else {
                m_failedToInitialize = false;
                setFileIndexerStatusText( status, true );
            }
        }
        else {
            m_failedToInitialize = true;
            setFileIndexerStatusText( i18nc( "@info:status", "File indexing service failed to initialize, "
                                             "most likely due to an installation problem." ), false );
        }
    }
    else if ( !m_failedToInitialize ) {
        setFileIndexerStatusText( i18nc( "@info:status", "File indexing service not running." ), false );
    }
}


void Nepomuk2::ServerConfigModule::updateBackupStatus()
{
    const QString backupUrl = KStandardDirs::locateLocal( "data", "nepomuk/backupsync/backups/" );
    QDir dir( backupUrl );
    const QStringList backupFiles = dir.entryList( QDir::Files | QDir::NoDotAndDotDot, QDir::Name );

    QString text = i18np("1 existing backup", "%1 existing backups", backupFiles.size() );
    if( !backupFiles.isEmpty() ) {
        text += QLatin1String(" (");
        text += i18nc("@info %1 is the creation date of a backup formatted vi KLocale::formatDateTime",
                      "Oldest: %1",
                      KGlobal::locale()->formatDateTime(QFileInfo(backupUrl+QLatin1String("/")+backupFiles.last()).created(), KLocale::FancyShortDate));
        text += QLatin1String(")");
    }

    m_labelBackupStats->setText( text );
}


void Nepomuk2::ServerConfigModule::recreateInterfaces()
{
    delete m_fileIndexerInterface;
    delete m_serverInterface;

    m_fileIndexerInterface = new org::kde::nepomuk::FileIndexer( "org.kde.nepomuk.services.nepomukfileindexer", "/nepomukfileindexer", QDBusConnection::sessionBus() );
    m_serverInterface = new org::kde::NepomukServer( "org.kde.NepomukServer", "/nepomukserver", QDBusConnection::sessionBus() );

    connect( m_fileIndexerInterface, SIGNAL( statusChanged() ),
             this, SLOT( updateFileIndexerStatus() ) );
}


void Nepomuk2::ServerConfigModule::slotStatusDetailsClicked()
{
    StatusWidget statusDialog( this );
    statusDialog.exec();
}


void Nepomuk2::ServerConfigModule::slotEditIndexFolders()
{
    const QStringList oldIncludeFolders = m_indexFolderSelectionDialog->includeFolders();
    const QStringList oldExcludeFolders = m_indexFolderSelectionDialog->excludeFolders();
    const bool oldIndexHidden = m_indexFolderSelectionDialog->indexHiddenFolders();

    if ( m_indexFolderSelectionDialog->exec() ) {
        changed();
    }
    else {
        // revert to previous settings
        m_indexFolderSelectionDialog->setFolders( oldIncludeFolders, oldExcludeFolders );
        m_indexFolderSelectionDialog->setIndexHiddenFolders( oldIndexHidden );
    }
}

void Nepomuk2::ServerConfigModule::slotAdvancedFileIndexing()
{
    const QStringList oldExcludeFilters = m_excludeFilterSelectionDialog->excludeFilters();
    QStringList oldExcludeMimeTypes = m_excludeFilterSelectionDialog->excludeMimeTypes();

    if( m_checkboxesChanged ) {
        oldExcludeMimeTypes = mimetypesFromCheckboxes();
        m_excludeFilterSelectionDialog->setExcludeMimeTypes( oldExcludeMimeTypes );
        m_checkboxesChanged = false;
    }

    if( m_excludeFilterSelectionDialog->exec() ) {
        changed();

        QStringList mimetypes = m_excludeFilterSelectionDialog->excludeMimeTypes();
        syncCheckBoxesFromMimetypes( mimetypes );
    }
    else {
        m_excludeFilterSelectionDialog->setExcludeFilters( oldExcludeFilters );
        m_excludeFilterSelectionDialog->setExcludeMimeTypes( oldExcludeMimeTypes );
    }
}


void Nepomuk2::ServerConfigModule::slotBackupFrequencyChanged()
{
    m_comboBackupDay->setShown(m_comboBackupFrequency->currentIndex() >= WeeklyBackup);
    m_comboBackupDay->setDisabled(m_comboBackupFrequency->currentIndex() == DisableAutomaticBackups);
    m_editBackupTime->setDisabled(m_comboBackupFrequency->currentIndex() == DisableAutomaticBackups);
    m_spinBackupMax->setDisabled(m_comboBackupFrequency->currentIndex() == DisableAutomaticBackups);
}

void Nepomuk2::ServerConfigModule::slotManualBackup()
{
    KProcess::execute( "nepomukbackup", QStringList() << "--backup" );
}

void Nepomuk2::ServerConfigModule::slotRestoreBackup()
{
    KProcess::execute( "nepomukbackup", QStringList() << "--restore" );
}

namespace {
    bool containsRegex(const QStringList& list, const QString& regex) {
        QRegExp exp( regex, Qt::CaseInsensitive, QRegExp::Wildcard );
        foreach( const QString& string, list ) {
            if( string.contains( exp ) )
                return true;;
        }
        return false;
    }

    void syncCheckBox(const QStringList& mimetypes, const QString& type, QCheckBox* checkbox) {
        if( containsRegex( mimetypes, type ) ) {
            if( mimetypes.contains( type ) )
                checkbox->setChecked( false );
            else
                checkbox->setCheckState( Qt::PartiallyChecked );
        }
        else {
            checkbox->setChecked( true );
        }
    }

    void syncCheckBox(const QStringList& mimetypes, const QStringList& types, QCheckBox* checkbox) {
        bool containsAll = true;
        bool containsAny = false;

        foreach( const QString& type, types ) {
            if( mimetypes.contains(type) ) {
                containsAny = true;
            }
            else {
                containsAll = false;
            }
        }

        if( containsAll )
            checkbox->setCheckState( Qt::Unchecked );
        else if( containsAny )
            checkbox->setCheckState( Qt::PartiallyChecked );
        else
            checkbox->setCheckState( Qt::Checked );
    }

}

void Nepomuk2::ServerConfigModule::syncCheckBoxesFromMimetypes(const QStringList& mimetypes)
{
    syncCheckBox( mimetypes, QLatin1String("image/*"), m_checkboxImage );
    syncCheckBox( mimetypes, QLatin1String("audio/*"), m_checkboxAudio );
    syncCheckBox( mimetypes, QLatin1String("video/*"), m_checkboxVideo );

    syncCheckBox( mimetypes, documentMimetypes(), m_checkboxDocuments );
    syncCheckBox( mimetypes, sourceCodeMimeTypes(), m_checkboxSourceCode );
    m_checkboxesChanged = false;
}

QStringList Nepomuk2::ServerConfigModule::mimetypesFromCheckboxes()
{
    QStringList types;
    if( !m_checkboxAudio->isChecked() )
        types << QLatin1String("audio/*");
    if( !m_checkboxImage->isChecked() )
        types << QLatin1String("image/*");
    if( !m_checkboxVideo->isChecked() )
        types << QLatin1String("video/*");
    if( !m_checkboxDocuments->isChecked() )
        types << documentMimetypes();
    if( !m_checkboxSourceCode->isChecked() )
        types << sourceCodeMimeTypes();

    return types;
}

void Nepomuk2::ServerConfigModule::slotCheckBoxesChanged()
{
    m_checkboxesChanged = true;;
    changed( true );
}


#include "nepomukserverkcm.moc"
