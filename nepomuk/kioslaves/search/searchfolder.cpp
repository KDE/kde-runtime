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
#include "nepomuksearchurltools.h"
#include "resourcestat.h"

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
#include <KMimeType>


Nepomuk::SearchFolder::SearchFolder( const KUrl& url, KIO::SlaveBase* slave )
    : QThread(),
      m_url( url ),
      m_initialListingFinished( false ),
      m_slave( slave )
{
    kDebug() <<  url;

    qRegisterMetaType<QList<QUrl> >();

    // parse URL
    m_query = queryFromUrl( url );
}


Nepomuk::SearchFolder::~SearchFolder()
{
    kDebug() << m_url << QThread::currentThread();

    // properly shut down the search thread
    quit();
    wait();
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

    // start the search thread
    start();

    // list all results
    statResults();

    kDebug() << "listing done";

    m_slave->listEntry( KIO::UDSEntry(), true );
    m_slave->finished();

    // shutdown and delete
    exit();
    deleteLater();
}


// always called in search thread
void Nepomuk::SearchFolder::slotNewEntries( const QList<Nepomuk::Query::Result>& results )
{
    kDebug() << m_url;

    m_resultMutex.lock();
    m_resultsQueue += results;
    m_resultMutex.unlock();

    if ( !m_initialListingFinished ) {
        kDebug() << "Waking main thread";
        m_resultWaiter.wakeAll();
    }
}


// always called in search thread
void Nepomuk::SearchFolder::slotFinishedListing()
{
    kDebug() << m_url;
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
            KIO::UDSEntry uds = statResult( result );
            if ( uds.count() ) {
                kDebug() << "listing" << result.resource().resourceUri();
                m_slave->listEntry( uds, false );
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

        const KUrl fileUrl = Nepomuk::nepomukToFileUrl( url );
        if ( !fileUrl.isEmpty() ) {
            if ( KIO::StatJob* job = KIO::stat( fileUrl, KIO::HideProgressInfo ) ) {
                // we do not want to wait for the event loop to delete the job
                QScopedPointer<KIO::StatJob> sp( job );
                job->setAutoDelete( false );
                if ( KIO::NetAccess::synchronousRun( job, 0 ) ) {
                    uds = job->statResult();
                    return true;
                }
            }
        }
        else {
            Nepomuk::Resource res( url );
            if ( res.exists() ) {
                uds = Nepomuk::statNepomukResource( res );
                return true;
            }
        }

        kDebug() << "failed to stat" << url;
        return false;
    }
}


// always called in main thread
KIO::UDSEntry Nepomuk::SearchFolder::statResult( const Query::Result& result )
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

        return uds;
    }
    else {
        kDebug() << "Stating" << result.resource().resourceUri() << "failed";
        return KIO::UDSEntry();
    }
}
