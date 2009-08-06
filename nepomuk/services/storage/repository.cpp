/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "repository.h"
#include "nepomukstorage-config.h"
#include "modelcopyjob.h"

#ifdef HAVE_CLUCENE
#include "cluceneanalyzer.h"
#endif

#include <Soprano/Backend>
#include <Soprano/Global>
#include <Soprano/Version>
#include <Soprano/StorageModel>
#include <Soprano/Error/Error>
#include <Soprano/Vocabulary/RDF>

#ifdef HAVE_SOPRANO_INDEX
#include <Soprano/Index/IndexFilterModel>
#include <Soprano/Index/CLuceneIndex>
#endif

#include <KStandardDirs>
#include <KDebug>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KLocale>
#include <KNotification>
#include <KIcon>

#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>


namespace {
    QString createStoragePath( const QString& repositoryId )
    {
        return KStandardDirs::locateLocal( "data", "nepomuk/repository/" + repositoryId + '/' );
    }

#if defined(HAVE_SOPRANO_INDEX) && defined(HAVE_CLUCENE) && SOPRANO_IS_VERSION(2,1,64)
    class RebuildIndexThread : public QThread
    {
    public:
        RebuildIndexThread( Soprano::Index::IndexFilterModel* model )
            : m_model( model ) {
        }

        void run() {
            m_model->rebuildIndex();
        }

    private:
        Soprano::Index::IndexFilterModel* m_model;
    };

    // The index version that is written by this implementation. Increase this if a major bug
    // was fixed in the CLucene index or new information needs to be written into the index
    // (for example using Soprano::IndexFilterModel::addForceIndexPredicate.)
    const int s_indexVersion = 2;
#endif
}


Nepomuk::Repository::Repository( const QString& name )
    : m_name( name ),
      m_state( CLOSED ),
      m_model( 0 ),
      m_analyzer( 0 ),
      m_index( 0 ),
      m_indexModel( 0 )
{
}


Nepomuk::Repository::~Repository()
{
    close();
}


void Nepomuk::Repository::close()
{
    if ( m_state == OPEN ) {
#ifdef HAVE_SOPRANO_INDEX
        delete m_indexModel;
        delete m_index;
        m_indexModel = 0;
        m_index = 0;
#ifdef HAVE_CLUCENE
        delete m_analyzer;
        m_analyzer = 0;
#endif
#endif
        delete m_model;
        m_model = 0;

        m_state = CLOSED;
    }
}


void Nepomuk::Repository::open()
{
    Q_ASSERT( m_state == CLOSED );

    m_state = OPENING;

    // get used backend
    // =================================
    const Soprano::Backend* backend = activeSopranoBackend();
    if ( !backend ) {
        m_state = CLOSED;
        emit opened( this, false );
        return;
    }

    // read config
    // =================================
    KConfigGroup repoConfig = KSharedConfig::openConfig( "nepomukserverrc" )->group( name() + " Settings" );
    QString oldBackendName = repoConfig.readEntry( "Used Soprano Backend", backend->pluginName() );
    QString oldBasePath = repoConfig.readPathEntry( "Storage Dir", QString() ); // backward comp: empty string means old storage path

    // If possible we want to keep the old storage path. exception: oldStoragePath is empty. In that case we stay backwards
    // compatible and convert the data to the new default location createStoragePath( name ) + "data/" + backend->pluginName()
    //
    // If we have a proper oldStoragePath and a different backend we use the oldStoragePath as basePath
    // newDataPath = oldStoragePath + "data/" + backend->pluginName()
    // oldDataPath = oldStoragePath + "data/" + oldBackendName


    // create storage paths
    // =================================
    m_basePath = oldBasePath.isEmpty() ? createStoragePath( name() ) : oldBasePath;
    QString indexPath = m_basePath + "index";
    QString storagePath = m_basePath + "data/" + backend->pluginName();

    KStandardDirs::makeDir( indexPath );
    KStandardDirs::makeDir( storagePath );

    kDebug() << "opening repository '" << name() << "' at '" << m_basePath << "'";


    // open storage
    // =================================
    m_model = backend->createModel( QList<Soprano::BackendSetting>() << Soprano::BackendSetting( Soprano::BackendOptionStorageDir, storagePath ) );
    if ( !m_model ) {
        kDebug() << "Unable to create model for repository" << name();
        m_state = CLOSED;
        emit opened( this, false );
        return;
    }

    kDebug() << "Successfully created new model for repository" << name();

#if defined(HAVE_SOPRANO_INDEX) && defined(HAVE_CLUCENE)
    m_analyzer = new CLuceneAnalyzer();
    m_index = new Soprano::Index::CLuceneIndex( m_analyzer );

    if ( m_index->open( indexPath, true ) ) {
        kDebug() << "Successfully created new index for repository" << name();
        m_indexModel = new Soprano::Index::IndexFilterModel( m_index, m_model );

        // FIXME: find a good value here
        m_indexModel->setTransactionCacheSize( 100 );

#if SOPRANO_IS_VERSION(2,1,64)
        m_indexModel->addForceIndexPredicate( Soprano::Vocabulary::RDF::type() );
#endif

        setParentModel( m_indexModel );
    }
    else {
        kDebug() << "Unable to open CLucene index for repo '" << name() << "': " << m_index->lastError();
        delete m_index;
        delete m_model;
        m_index = 0;
        m_model = 0;

        m_state = CLOSED;
        emit opened( this, false );
        return;
    }
#else
    setParentModel( m_model );
#endif

    // check if we have to convert
    // =================================
    bool convertingData = false;

    // if the backend changed we convert
    // in case only the storage dir changes we normally would not have to convert but
    // it is just simpler this way
    if ( oldBackendName != backend->pluginName() ||
         oldBasePath.isEmpty() ) {

        kDebug() << "Previous backend:" << oldBackendName << "- new backend:" << backend->pluginName();
        kDebug() << "Old path:" << oldBasePath << "- new path:" << m_basePath;

        if ( oldBasePath.isEmpty() ) {
            // backward comp: empty string means old storage path
            // and before we stored the data directly in the default basePath
            m_oldStoragePath = createStoragePath( name() );
        }
        else {
            m_oldStoragePath = m_basePath + "data/" + oldBackendName;
        }

        // try creating a model for the old storage
        Soprano::Model* oldModel = 0;
        m_oldStorageBackend = Soprano::discoverBackendByName( oldBackendName );
        if ( m_oldStorageBackend ) {
            // FIXME: even if there is no old data we still create a model here which results in a new empty db!
            oldModel = m_oldStorageBackend->createModel( QList<Soprano::BackendSetting>() << Soprano::BackendSetting( Soprano::BackendOptionStorageDir, m_oldStoragePath ) );
        }

        if ( oldModel ) {
            if ( !oldModel->isEmpty() ) {
                kDebug() << "Starting model conversion";

                convertingData = true;
                // No need to use the index filter as it already contains the data
                ModelCopyJob* copyJob = new ModelCopyJob( oldModel, m_model, this );
                connect( copyJob, SIGNAL( result( KJob* ) ), this, SLOT( copyFinished( KJob* ) ) );
                copyJob->start();
            }
            else {
                m_state = OPEN;
            }
        }
        else {
            // FIXME: inform the user
            kDebug( 300002 ) << "Unable to convert old model.";
            m_state = OPEN;
        }
    }
    else {
        kDebug() << "no need to convert" << name();
        m_state = OPEN;
    }

    // save the settings
    // =================================
    // do not save when converting yet. If converting is cancelled we would loose data.
    // this way conversion is restarted the next time
    if ( !convertingData ) {
        repoConfig.writeEntry( "Used Soprano Backend", backend->pluginName() );
        repoConfig.writePathEntry( "Storage Dir", m_basePath );
        repoConfig.sync(); // even if we crash the model has been created

        if( m_state == OPEN ) {
            if ( !rebuildIndexIfNecessary() ) {
                emit opened( this, true );
            }
        }
    }
    else {
        KNotification::event( "convertingNepomukData",
                              i18nc("@info - notification message",
                                    "Converting Nepomuk data to a new backend. This might take a while."),
                                    KIcon( "nepomuk" ).pixmap( 32, 32 ) );
    }
}


void Nepomuk::Repository::rebuildingIndexFinished()
{
#if defined(HAVE_SOPRANO_INDEX) && defined(HAVE_CLUCENE) && SOPRANO_IS_VERSION(2,1,64)
    KNotification::event( "rebuldingNepomukIndexDone",
                          i18nc("@info - notification message",
                                "Rebuilding Nepomuk full text search index for new features done."),
                          KIcon( "nepomuk" ).pixmap( 32, 32 ) );

    // save our new settings
    KConfigGroup repoConfig = KSharedConfig::openConfig( "nepomukserverrc" )->group( name() + " Settings" );
    repoConfig.writeEntry( "index version", s_indexVersion );

    // inform that we are open and done
    m_state = OPEN;
    emit opened( this, true );
#endif
}


void Nepomuk::Repository::copyFinished( KJob* job )
{
    if ( job->error() ) {
        KNotification::event( "convertingNepomukDataFailed",
                              i18nc("@info - notification message",
                                    "Converting Nepomuk data to the new backend failed. Data may still be recovered manually though."),
                                    KIcon( "nepomuk" ).pixmap( 32, 32 ) );

        kDebug( 300002 ) << "Converting old model failed.";
    }
    else {
        KNotification::event( "convertingNepomukDataDone",
                              i18nc("@info - notification message",
                                    "Successfully converted Nepomuk data to the new backend."),
                                    KIcon( "nepomuk" ).pixmap( 32, 32 ) );

        kDebug() << "Successfully converted model data for repo" << name();

        // delete the old model
        ModelCopyJob* copyJob = qobject_cast<ModelCopyJob*>( job );
        delete copyJob->source();

        // cleanup the actual data
        m_oldStorageBackend->deleteModelData( QList<Soprano::BackendSetting>() << Soprano::BackendSetting( Soprano::BackendOptionStorageDir, m_oldStoragePath ) );

        // save our new settings
        KConfigGroup repoConfig = KSharedConfig::openConfig( "nepomukserverrc" )->group( name() + " Settings" );
        repoConfig.writeEntry( "Used Soprano Backend", activeSopranoBackend()->pluginName() );
        repoConfig.writePathEntry( "Storage Dir", m_basePath );
        repoConfig.sync();

        if ( rebuildIndexIfNecessary() ) {
            // opened will be emitted in rebuildingIndexFinished
            return;
        }
    }

    // although converting might have failed, the new model is open anyway
    m_state = OPEN;
    emit opened( this, true );
}


void Nepomuk::Repository::optimize()
{
    QTimer::singleShot( 0, this, SLOT( slotDoOptimize() ) );
}


void Nepomuk::Repository::slotDoOptimize()
{
#ifdef HAVE_SOPRANO_INDEX
#if SOPRANO_IS_VERSION(2,1,60)
    m_index->optimize();
#endif
#endif
}


void Nepomuk::Repository::rebuildIndex()
{
#if defined(HAVE_SOPRANO_INDEX) && defined(HAVE_CLUCENE) && SOPRANO_IS_VERSION(2,1,64)
    RebuildIndexThread* rit = new RebuildIndexThread( m_indexModel );
    connect( rit, SIGNAL( finished() ), rit, SLOT( deleteLater() ) );
    rit->start();
#endif
}


bool Nepomuk::Repository::rebuildIndexIfNecessary()
{
#if defined(HAVE_SOPRANO_INDEX) && defined(HAVE_CLUCENE) && SOPRANO_IS_VERSION(2,1,64)
    KConfigGroup repoConfig = KSharedConfig::openConfig( "nepomukserverrc" )->group( name() + " Settings" );
    if( repoConfig.readEntry( "index version", 1 ) < s_indexVersion ) {
        KNotification::event( "rebuldingNepomukIndex",
                              i18nc("@info - notification message",
                                    "Rebuilding Nepomuk full text search index for new features. This will only be done once and might take a while."),
                              KIcon( "nepomuk" ).pixmap( 32, 32 ) );
        RebuildIndexThread* rit = new RebuildIndexThread( m_indexModel );
        connect( rit, SIGNAL( finished() ), this, SLOT( rebuildingIndexFinished() ) );
        connect( rit, SIGNAL( finished() ), rit, SLOT( deleteLater() ) );
        rit->start();
        return true;
    }
#endif
    return false;
}


const Soprano::Backend* Nepomuk::Repository::activeSopranoBackend()
{
    QString backendName = KSharedConfig::openConfig( "nepomukserverrc" )->group( "Basic Settings" ).readEntry( "Soprano Backend", "sesame2" );
    const Soprano::Backend* backend = ::Soprano::discoverBackendByName( backendName );
    if ( !backend ) {
        kDebug() << "(Nepomuk::Core::Core) could not find backend" << backendName << ". Falling back to default.";
        backend = ::Soprano::usedBackend();
    }
    if ( !backend ) {
        kDebug() << "(Nepomuk::Core::Core) could not find a backend.";
    }
    return backend;
}

#include "repository.moc"
