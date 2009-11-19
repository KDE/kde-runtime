/*
   Copyright (C) 2008-2009 by Sebastian Trueg <trueg at kde.org>

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
#include "nfo.h"
#include "nie.h"
#include "pimo.h"

#include <Soprano/Vocabulary/Xesam>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Node> // for qHash( QUrl )

#include <Nepomuk/Variant>
#include <Nepomuk/Thing>
#include <Nepomuk/Types/Class>
#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/QueryServiceClient>

#include <QtCore/QMutexLocker>

#include <KUrl>
#include <KDebug>
#include <KIO/Job>
#include <KIO/NetAccess>
#include <KUser>
#include <kdirnotify.h>


namespace {
    QString addCounterToFileName( const QString& name, int i )
    {
        QString newName( name );

        int start = name.lastIndexOf('.');
        if (start != -1) {
            // has a . somewhere, e.g. it has an extension
            newName.insert(start, QString( " (%1)" ).arg( i ));
        }
        else {
            // no extension, just tack it on to the end
            newName += QString( " (%1)" ).arg( i );
        }
        return newName;
    }
}


Nepomuk::SearchEntry::SearchEntry( const QUrl& res,
                                   const KIO::UDSEntry& uds)
    : m_resource( res ),
      m_entry( uds )
{
}


Nepomuk::SearchFolder::SearchFolder()
{
    // try to force clients to invalidate their cache
    org::kde::KDirNotify::emitFilesAdded( "nepomuksearch:/" + m_name );
}


Nepomuk::SearchFolder::SearchFolder( const QString& name, const QString& query, KIO::SlaveBase* slave )
    : QThread(),
      m_name( name ),
      m_query( query ),
      m_initialListingFinished( false ),
      m_slave( slave ),
      m_listEntries( false )
{
    kDebug() << name << QThread::currentThread();
    Q_ASSERT( !name.isEmpty() );

    qRegisterMetaType<QList<QUrl> >();
}


Nepomuk::SearchFolder::~SearchFolder()
{
    kDebug() << m_name << QThread::currentThread();

    // properly shut down the search thread
    quit();
    wait();

    qDeleteAll( m_entries );
}


void Nepomuk::SearchFolder::run()
{
    kDebug() << m_name << QThread::currentThread();

    m_client = new Nepomuk::Query::QueryServiceClient();

    // results signals are connected directly to update the results cache m_resultsQueue
    // and the entries cache m_entries, as well as emitting KDirNotify signals
    // a queued connection is not possible since we have no event loop after the
    // initial listing which means that queued signals would never get delivered
    connect( m_client, SIGNAL( newEntries( const QList<Nepomuk::Query::Result>& ) ),
             this, SLOT( slotNewEntries( const QList<Nepomuk::Query::Result>& ) ),
             Qt::DirectConnection );
    connect( m_client, SIGNAL( entriesRemoved( const QList<QUrl>& ) ),
             this, SLOT( slotEntriesRemoved( const QList<QUrl>& ) ),
             Qt::DirectConnection );
    connect( m_client, SIGNAL( finishedListing() ),
             this, SLOT( slotFinishedListing() ),
             Qt::DirectConnection );

    Query::Query q;
    q.addRequestProperty( Query::Query::RequestProperty( Nepomuk::Vocabulary::NIE::url() ) );
    m_client->sparqlQuery( m_query, q.requestPropertyMap() );
    exec();
    delete m_client;

    kDebug() << m_name << "done";
}


void Nepomuk::SearchFolder::list()
{
    kDebug() << m_name << QThread::currentThread();

    m_listEntries = true;

    if ( !isRunning() ) {
        start();
    }

    // list all cached entries
    kDebug() << "listing" << m_entries.count() << "cached entries";
    for ( QHash<QString, SearchEntry*>::const_iterator it = m_entries.constBegin();
          it != m_entries.constEnd(); ++it ) {
        m_slave->listEntry( ( *it )->entry(), false );
    }

    // list all results
    statResults();

    kDebug() << "listing done";

    m_listEntries = false;

    m_slave->listEntry( KIO::UDSEntry(), true );
    m_slave->finished();
}


void Nepomuk::SearchFolder::stat( const QString& name )
{
    kDebug() << name;

    m_listEntries = false;

    if ( SearchEntry* entry = findEntry( name ) ) {
        m_slave->statEntry( entry->entry() );
        m_slave->finished();
    }
    else {
        m_slave->error( KIO::ERR_DOES_NOT_EXIST, "nepomuksearch:/" + m_name + '/' + name );
    }
}


Nepomuk::SearchEntry* Nepomuk::SearchFolder::findEntry( const QString& name )
{
    kDebug() << name;

    //
    // get all results in case we do not have them already
    //
    if ( !isRunning() )
        start();
    statResults();

    //
    // search for the one we need
    //
    QHash<QString, SearchEntry*>::const_iterator it = m_entries.constFind( name );
    if ( it != m_entries.constEnd() ) {
        kDebug() << "-----> found";
        return *it;
    }
    else {
        kDebug() << "-----> not found";
        return 0;
    }
}


Nepomuk::SearchEntry* Nepomuk::SearchFolder::findEntry( const KUrl& url )
{
    Q_UNUSED(url);
    // FIXME
    return 0;
}


// always called in search thread
void Nepomuk::SearchFolder::slotNewEntries( const QList<Nepomuk::Query::Result>& results )
{
    kDebug() << m_name << QThread::currentThread();

    m_resultMutex.lock();
    m_resultsQueue += results;
    m_resultMutex.unlock();

    if ( m_initialListingFinished ) {
        // inform everyone of the change
        kDebug() << ( "Informing about change in folder nepomuksearch:/" + m_name );
        org::kde::KDirNotify::emitFilesAdded( "nepomuksearch:/" + m_name );
    }
    else {
        kDebug() << "Waking main thread";
        m_resultWaiter.wakeAll();
    }
}


// always called in search thread
void Nepomuk::SearchFolder::slotEntriesRemoved( const QList<QUrl>& entries )
{
    kDebug() << QThread::currentThread();

    QMutexLocker lock( &m_resultMutex );

    foreach( const QUrl& uri, entries ) {
        // inform everybody
        org::kde::KDirNotify::emitFilesRemoved( QStringList() << ( "nepomuksearch:/" + m_name + '/' + uri.toEncoded().toPercentEncoding() ) );
    }
}


// always called in search thread
void Nepomuk::SearchFolder::slotFinishedListing()
{
    kDebug() << m_name << QThread::currentThread();
    QMutexLocker lock( &m_resultMutex );
    m_initialListingFinished = true;
    m_resultWaiter.wakeAll();
}


// always called in main thread
void Nepomuk::SearchFolder::statResults()
{
    while ( 1 ) {
        m_resultMutex.lock();
        if ( !m_resultsQueue.isEmpty() ) {
            Query::Result result = m_resultsQueue.dequeue();
            m_resultMutex.unlock();
            SearchEntry* entry = statResult( result );
            if ( entry ) {
                if ( m_listEntries ) {
                    kDebug() << "listing" << entry->resource();
                    m_slave->listEntry( entry->entry(), false );
                }
            }
        }
        else if ( !m_initialListingFinished ) {
            m_resultWaiter.wait( &m_resultMutex );
            m_resultMutex.unlock();
        }
        else {
            m_resultMutex.unlock();
            break;
        }
    }
}


namespace {
    bool statFile( const KUrl& url, KIO::UDSEntry& uds )
    {
        // the akonadi kio slave is just way too slow and
        // in KDE 4.4 akonadi items should have nepomuk:/res/<uuid> URIs anyway
        if ( url.scheme() == QLatin1String( "akonadi" ) )
            return false;

        bool success = false;

        if ( KIO::StatJob* job = KIO::stat( url, KIO::HideProgressInfo ) ) {
            job->setAutoDelete( false );
            if ( KIO::NetAccess::synchronousRun( job, 0 ) ) {
                uds = job->statResult();
                success = true;
            }
            else {
                kDebug() << "failed to stat" << url;
            }
            delete job;
        }

        return success;
    }
}


// always called in main thread
Nepomuk::SearchEntry* Nepomuk::SearchFolder::statResult( const Query::Result& result )
{
    KUrl url = result.resource().resourceUri();
    KIO::UDSEntry uds;
    if ( statFile( url, uds ) ) {
        uds.insert( KIO::UDSEntry::UDS_NEPOMUK_URI, url.url() );
        // be backwards compatible for the pre KDE 4.4 times where we actually used file:/ URLs for nepomuk resources
        if ( url.scheme() != QLatin1String( "nepomuk" ) ) {
            uds.insert( KIO::UDSEntry::UDS_URL, url.url() );
        }
        SearchEntry* entry = new SearchEntry( url, uds );
        m_entries.insert( uds.stringValue( KIO::UDSEntry::UDS_NAME ), entry );
        return entry;
    }
    else {
        kDebug() << "Stating" << result.resource().resourceUri() << "failed";
        return 0;
    }
}
