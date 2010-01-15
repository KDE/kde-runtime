/*
   Copyright 2008-2010 Sebastian Trueg <trueg@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include <Nepomuk/Query/QueryParser>
#include <Nepomuk/Query/QueryServiceClient>

#include <QtCore/QMutexLocker>

#include <KUrl>
#include <KDebug>
#include <KIO/Job>
#include <KIO/NetAccess>
#include <KUser>
#include <kdirnotify.h>
#include <KMimeType>

namespace {
    QString queryFromUrl( const KUrl& url, bool* sparql = 0 ) {
        if(url.queryItems().contains( "sparql" ) ) {
            if( sparql )
                *sparql = true;
            return url.queryItem( "sparql" );
        }
        else if(url.queryItems().contains( "query" ) ) {
            if( sparql )
                *sparql = false;
            return url.queryItem( "query" );
        }
        else {
            if( sparql )
                *sparql = false;
            return url.path().section( '/', 0, 0, QString::SectionSkipEmpty );
        }
    }

    /**
     * Encode the resource URI into the UDS_NAME to make it unique.
     * It is important that we do not use the % for percent-encoding. Otherwise KUrl::url will
     * re-encode them, thus, destroying our name.
     *
     * This is (and should always be) the exact same as in ../nepomuk/kio_nepomuk.cpp.
     */
    QString resourceUriToUdsName( const KUrl& url )
    {
        return QString::fromAscii( url.toEncoded().toPercentEncoding( QByteArray(), QByteArray(), '_' ) );
    }
}


Nepomuk::SearchEntry::SearchEntry( const QUrl& res,
                                   const KIO::UDSEntry& uds)
    : m_resource( res ),
      m_entry( uds )
{
}


Nepomuk::SearchFolder::SearchFolder( const KUrl& url, KIO::SlaveBase* slave )
    : QThread(),
      m_url( url ),
      m_initialListingFinished( false ),
      m_slave( slave ),
      m_listEntries( false )
{
    kDebug() <<  url;

    qRegisterMetaType<QList<QUrl> >();

    // parse URL
    bool sparql = false;
    m_query = queryFromUrl( url, &sparql );
    if ( !sparql ) {
        Query::Query query = Query::QueryParser::parseQuery( m_query );
        query.addRequestProperty( Query::Query::RequestProperty( Nepomuk::Vocabulary::NIE::url(), true ) );
        m_query = query.toSparqlQuery();
    }
}


Nepomuk::SearchFolder::~SearchFolder()
{
    kDebug() << m_url << QThread::currentThread();

    // properly shut down the search thread
    quit();
    wait();

    qDeleteAll( m_entries );
}


void Nepomuk::SearchFolder::run()
{
    kDebug() << m_url << QThread::currentThread();

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

    kDebug() << m_url << "done";
}


void Nepomuk::SearchFolder::list()
{
    kDebug() << m_url << QThread::currentThread();

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
    kDebug() << m_url << QThread::currentThread();

    m_resultMutex.lock();
    m_resultsQueue += results;
    m_resultMutex.unlock();

    if ( m_initialListingFinished ) {
        // inform everyone of the change
        kDebug() << "Informing about change in folder" << m_url.url();
        org::kde::KDirNotify::emitFilesAdded( m_url.url() );
    }
    else {
        kDebug() << "Waking main thread";
        m_resultWaiter.wakeAll();
    }
}


// always called in search thread
void Nepomuk::SearchFolder::slotEntriesRemoved( const QList<QUrl>& entries )
{
    QMutexLocker lock( &m_resultMutex );

    QStringList urls;
    foreach( const QUrl& uri, entries ) {
        QString name = resourceUriToUdsName( uri );

        // update cache
        delete m_entries.take( name );

        KUrl resultUrl( m_url );
        resultUrl.addPath( resourceUriToUdsName( uri ) );
        urls << resultUrl.url();
    }

    // inform everybody
    kDebug() << m_url << "Informing about removed files:" << urls;
    org::kde::KDirNotify::emitFilesRemoved( urls );
}


// always called in search thread
void Nepomuk::SearchFolder::slotFinishedListing()
{
    kDebug() << m_url << QThread::currentThread();
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
    Resource res( result.resource() );
    KUrl url = res.resourceUri();

    KIO::UDSEntry uds;
    if ( statFile( url, uds ) ) {
        if ( result.resource().hasProperty( Nepomuk::Vocabulary::NIE::url() ) ) {
            KUrl fileUrl( res.property( Nepomuk::Vocabulary::NIE::url() ).toUrl() );

            // needed since the nepomuk:/ KIO slave does not do stating of files in its own
            // subdirs (tags and filesystems), and neither do we with real subdirs
            if ( uds.isDir() )
                uds.insert( KIO::UDSEntry::UDS_URL, fileUrl.url() );

            if ( fileUrl.isLocalFile() ) {
                uds.insert( KIO::UDSEntry::UDS_LOCAL_PATH, fileUrl.toLocalFile() );
            }
        }

        // needed since the file:/ KIO slave does not create them and KFileItem::nepomukUri()
        // cannot know that it is a local file since it is forwarded
        uds.insert( KIO::UDSEntry::UDS_NEPOMUK_URI, url.url() );

        // make sure we have unique names for everything
        uds.insert( KIO::UDSEntry::UDS_NAME, resourceUriToUdsName( url ) );

        // make sure we do not use these ugly names for display
        if ( !uds.contains( KIO::UDSEntry::UDS_DISPLAY_NAME ) ) {
            if ( res.hasType( Nepomuk::Vocabulary::PIMO::Thing() ) ) {
                if ( !res.pimoThing().groundingOccurrences().isEmpty() ) {
                    res = res.pimoThing().groundingOccurrences().first();
                }
            }

            if ( res.hasProperty( Nepomuk::Vocabulary::NIE::url() ) ) {
                KUrl fileUrl( res.property( Nepomuk::Vocabulary::NIE::url() ).toUrl() );
                uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, fileUrl.fileName() );

                // since we change the UDS_NAME KFileItem cannot handle mimetype and such anymore
                QString mimetype = uds.stringValue( KIO::UDSEntry::UDS_MIME_TYPE );
                if ( mimetype.isEmpty() ) {
                    mimetype = KMimeType::findByUrl(fileUrl)->name();
                    uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, mimetype );
                }
            }
            else {
                uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, result.resource().genericLabel() );
            }
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
