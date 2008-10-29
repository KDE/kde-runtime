/*
   Copyright (C) 2008 by Sebastian Trueg <trueg at kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "searchfolder.h"

#include "queryserviceclient.h"

#include <Soprano/Vocabulary/Xesam>
#include <Soprano/Vocabulary/NAO>

#include <Nepomuk/Variant>

#include <QtCore/QMutexLocker>

#include <KUrl>
#include <KDebug>
#include <KIO/Job>
#include <KIO/NetAccess>
#include <KUser>
#include <kdirnotify.h>

uint qHash( const QUrl& url )
{
    return qHash( url.toString() );
}


namespace {
    QString addCounterToFileName( const QString& name, int i )
    {
        QString newName( name );

        int start = name.lastIndexOf('.');
        if (start != -1) {
            // has a . somewhere, e.g. it has an extension
            newName.insert(start, QString::number( i ));
        }
        else {
            // no extension, just tack it on to the end
            newName += QString::number( i );
        }
        return newName;
    }
}


Nepomuk::SearchEntry::SearchEntry( const QUrl& res,
                                   const KIO::UDSEntry& uds )
    : m_resource( res ),
      m_entry( uds )
{
}


Nepomuk::SearchFolder::SearchFolder()
{
    // try to force clients to invalidate their cache
    org::kde::KDirNotify::emitFilesAdded( "nepomuksearch:/" + m_name );
}


Nepomuk::SearchFolder::SearchFolder( const QString& name, const Search::Query& query, KIO::SlaveBase* slave )
    : QThread(),
      m_name( name ),
      m_query( query ),
      m_initialListingFinished( false ),
      m_slave( slave ),
      m_listEntries( false ),
      m_statingStarted( false )
{
    kDebug() << QThread::currentThread();
    Q_ASSERT( !name.isEmpty() );

    qRegisterMetaType<QList<QUrl> >();
}


Nepomuk::SearchFolder::~SearchFolder()
{
    kDebug() << m_name;

    // properly shut down the search thread
    quit();
    wait();

    qDeleteAll( m_entries );
}


void Nepomuk::SearchFolder::run()
{
    kDebug() << QThread::currentThread();
    m_client = new Nepomuk::Search::QueryServiceClient();

    // results signals are connected directly to update the results cache m_results
    // and the entries cache m_entries, as well as emitting KDirNotify signals
    // a queued connection is not possible since we have no event loop after the
    // initial listing which means that queued signals would never get delivered
    connect( m_client, SIGNAL( newEntries( const QList<Nepomuk::Search::Result>& ) ),
             this, SLOT( slotNewEntries( const QList<Nepomuk::Search::Result>& ) ),
             Qt::DirectConnection );
    connect( m_client, SIGNAL( entriesRemoved( const QList<QUrl>& ) ),
             this, SLOT( slotEntriesRemoved( const QList<QUrl>& ) ),
             Qt::DirectConnection );

    SearchFolderHelper helper( this );

    // finished signal is connected queued since it triggers the wrap method
    // which calls SlaveBase methods like listEntry and it is only called once
    // while we still have the event loop -> no problems there
    connect( m_client, SIGNAL( finishedListing() ),
             &helper, SLOT( slotFinishedListing() ),
             Qt::QueuedConnection );

    m_client->query( m_query );
    exec();
    delete m_client;
    kDebug() << "done";
}


void Nepomuk::SearchFolder::list()
{
    kDebug();

    m_listEntries = !m_initialListingFinished;
    m_statEntry = false;

    if ( !isRunning() ) {
        start();
        // we get queued signals from m_client, thus we need to be in the same thread
//        moveToThread( this );
    }
    else {
        // list all cached entries
        for ( QHash<QString, SearchEntry*>::const_iterator it = m_entries.constBegin();
              it != m_entries.constEnd(); ++it ) {
            m_slave->listEntry( ( *it )->entry(), false );
        }

        // if there is nothing more to list...
        if ( m_initialListingFinished &&
            m_results.isEmpty() ) {
            m_slave->listEntry( KIO::UDSEntry(), true );
            m_slave->finished();
        }
        else {
            m_listEntries = true;
        }
    }

    // if we have more to list
    if ( m_listEntries ) {
        if ( !m_statingStarted ) {
            QTimer::singleShot( 0, this, SLOT( slotStatNextResult() ) );
        }
        kDebug() << "entering loop";
        m_loop.exec();
    }
}


void Nepomuk::SearchFolder::stat( const QString& name )
{
    kDebug() << name;

    if ( SearchEntry* entry = findEntry( name ) ) {
        m_slave->statEntry( entry->entry() );
        m_slave->finished();
    }
    else if ( !isRunning() ||
              !m_results.isEmpty() ) {
        m_nameToStat = name;
        m_statEntry = true;
        m_listEntries = false;

        if ( !isRunning() ) {
            start();
            // we get queued signals from m_client, thus we need to be in the same thread
//            moveToThread( this );
        }

        if ( !m_statingStarted ) {
            QTimer::singleShot( 0, this, SLOT( slotStatNextResult() ) );
        }
        m_loop.exec();
    }
    else {
        m_slave->error( KIO::ERR_DOES_NOT_EXIST, "nepomuksearch:/" + m_name + '/' + name );
    }
}


Nepomuk::SearchEntry* Nepomuk::SearchFolder::findEntry( const QString& name ) const
{
    kDebug() << name;

    QHash<QString, SearchEntry*>::const_iterator it = m_entries.find( name );
    if ( it != m_entries.end() ) {
        kDebug() << "-----> found";
        return *it;
    }
    else {
        kDebug() << "-----> not found";
        return 0;
    }
}


Nepomuk::SearchEntry* Nepomuk::SearchFolder::findEntry( const KUrl& url ) const
{
    // FIXME
    return 0;
}


// always called in search thread
void Nepomuk::SearchFolder::slotNewEntries( const QList<Nepomuk::Search::Result>& results )
{
    kDebug() << QThread::currentThread();

    m_resultMutex.lock();
    m_results += results;
    m_resultMutex.unlock();

    if ( m_initialListingFinished ) {
        // inform everyone of the change
        kDebug() << ( "Informing about change in folder nepomuksearch:/" + m_name );
        org::kde::KDirNotify::emitFilesAdded( "nepomuksearch:/" + m_name );
    }
}


// always called in search thread
void Nepomuk::SearchFolder::slotEntriesRemoved( const QList<QUrl>& entries )
{
    kDebug() << QThread::currentThread();

    QMutexLocker lock( &m_resultMutex );

    foreach( const QUrl& uri, entries ) {
        QHash<QUrl, QString>::iterator it = m_resourceNameMap.find( uri );
        if ( it != m_resourceNameMap.end() ) {
            delete m_entries.take( it.value() );

            // inform everybody
            org::kde::KDirNotify::emitFilesRemoved( QStringList() << ( "nepomuksearch:/" + m_name + '/' + *it ) );

            m_resourceNameMap.erase( it );
        }
    }
}


// always called in main thread
void Nepomuk::SearchFolder::slotFinishedListing()
{
    kDebug() << QThread::currentThread();
    m_initialListingFinished = true;
    wrap();
}


// always called in main thread
void Nepomuk::SearchFolder::slotStatNextResult()
{
//    kDebug();
    m_statingStarted = true;

    while ( 1 ) {
        // never lock the mutex for the whole duration of the method
        // since it may start an event loop which can result in more
        // newEntries signals to be delivered which would result in
        // a deadlock
        m_resultMutex.lock();
        if( !m_results.isEmpty() ) {
            Search::Result result = m_results.dequeue();
            m_resultMutex.unlock();
            SearchEntry* entry = statResult( result );
            if ( entry ) {
                if ( m_listEntries ) {
                    kDebug() << "listing" << entry->resource();
                    m_slave->listEntry( entry->entry(), false );
                }
                else if ( m_statEntry ) {
                    if ( m_nameToStat == entry->entry().stringValue( KIO::UDSEntry::UDS_NAME ) ) {
                        kDebug() << "stating" << entry->resource();
                        m_nameToStat = QString();
                        m_slave->statEntry( entry->entry() );
                    }
                }
            }
        }
        else {
            m_resultMutex.unlock();
            break;
        }
    }

    if ( !m_results.isEmpty() ||
         !m_initialListingFinished ) {
        // we need to use the timer since statResource does only create an event loop
        // for files, not for arbitrary resources.
        QTimer::singleShot( 0, this, SLOT( slotStatNextResult() ) );
    }
    else {
        m_statingStarted = false;
        wrap();
    }
}


// always called in main thread
void Nepomuk::SearchFolder::wrap()
{
    if ( m_results.isEmpty() &&
         m_initialListingFinished &&
         m_loop.isRunning() ) {
        if ( m_listEntries ) {
            kDebug() << "listing done";
            m_slave->listEntry( KIO::UDSEntry(), true );
            m_slave->finished();
        }
        else if ( m_statEntry ) {
            if ( !m_nameToStat.isEmpty() ) {
                // if m_nameToStat is not empty the name was not found during listing which means that
                // it does not exist
                m_slave->error( KIO::ERR_DOES_NOT_EXIST, "nepomuksearch:/" + m_name + '/' + m_nameToStat );
                m_nameToStat = QString();
            }
            else
                m_slave->finished();
        }

        m_statingStarted = false;
        m_listEntries = false;
        m_statEntry = false;
        kDebug() << "exiting loop";
        m_loop.exit();
    }
}


// always called in main thread
Nepomuk::SearchEntry* Nepomuk::SearchFolder::statResult( const Search::Result& result )
{
    kDebug() << result.resourceUri();

    KIO::UDSEntry uds;

    KUrl url = result[Soprano::Vocabulary::Xesam::url()].toString();
    if ( url.isEmpty() ) {
        url = result.resourceUri();
    }
    bool isFile = false;
    if ( !url.isEmpty() && url.scheme() != "akonadi" ) { // do not stat akonadi resouces here, way too slow, even hangs if akonadi is not running
        kDebug() << "listing file" << url;
        if ( KIO::StatJob* job = KIO::stat( url, KIO::HideProgressInfo ) ) {
            job->setAutoDelete( false );
            if ( KIO::NetAccess::synchronousRun( job, 0 ) ) {
                uds = job->statResult();
                if ( url.isLocalFile() ) {
                    uds.insert( KIO::UDSEntry::UDS_LOCAL_PATH, url.toLocalFile() );
                }
                isFile = true;
            }
            else {
                kDebug() << "failed to stat" << url;
            }
            delete job;
        }
    }

    //
    // The nepomuk resource listing is the same as in the nepomuk kio slave.
    // So either only depend on that or let the nepomuk kio slave fail on each
    // stat.
    //
    if ( !isFile ) {
        kDebug() << "listing resource" << result.resourceUri();

        Nepomuk::Resource res( result.resourceUri() );

        QString name = res.genericLabel();

        // make sure name is not the URI (which is the fallback of genericLabel() and will lead to crashes in KDirModel)
        if ( name.contains( '/' ) ) {
            name = name.section( '/', -1 );
            if ( name.isEmpty() )
                name = res.resourceUri().fragment();
            if ( name.isEmpty() )
                name = res.resourceUri().toString().replace( '/', '_' );
        }

        uds.insert( KIO::UDSEntry::UDS_NAME, name );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, name );

        QString icon = res.genericIcon();
        if ( !icon.isEmpty() ) {
            uds.insert( KIO::UDSEntry::UDS_ICON_NAME, icon );
        }

        uds.insert( KIO::UDSEntry::UDS_CREATION_TIME, res.property( Soprano::Vocabulary::NAO::created() ).toDateTime().toTime_t() );

        uds.insert( KIO::UDSEntry::UDS_ACCESS, 0700 );
        uds.insert( KIO::UDSEntry::UDS_USER, KUser().loginName() );

//    uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, "application/x-nepomuk-resource" );
    }

    uds.insert( KIO::UDSEntry::UDS_TARGET_URL, result.resourceUri().toString() );

    //
    // make sure we have no duplicate names
    //
    QString name = uds.stringValue( KIO::UDSEntry::UDS_DISPLAY_NAME );
    if ( name.isEmpty() ) {
        name = uds.stringValue( KIO::UDSEntry::UDS_NAME );
    }

    // the name is empty if the resource could not be stated
    if ( !name.isEmpty() ) {
        int cnt = 0;
        if ( m_nameCntHash.contains( name ) ) {
            cnt = ++m_nameCntHash[name];
        }
        else {
            cnt = m_nameCntHash[name] = 0;
        }
        if ( cnt >= 1 ) {
            name = addCounterToFileName( name, cnt );
        }
        uds.insert( KIO::UDSEntry::UDS_NAME, name );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, name );

        SearchEntry* entry = new SearchEntry( result.resourceUri(), uds );
        m_entries.insert( name, entry );
        m_resourceNameMap.insert( result.resourceUri(), name );

        kDebug() << "Stating" << result.resourceUri() << "done";

        return entry;
    }
    else {
        // no valid name -> no valid uds
        kDebug() << "Stating" << result.resourceUri() << "failed";
        return 0;
    }
}
