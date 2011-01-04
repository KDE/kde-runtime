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
#include "nepomuksearchurltools.h"
#include "resourcestat.h"
#include "queryutils.h"

#include <Soprano/Vocabulary/Xesam>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Node> // for qHash( QUrl )

#include <Nepomuk/Variant>
#include <Nepomuk/Thing>
#include <Nepomuk/Types/Class>
#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/QueryParser>
#include <Nepomuk/Query/ResourceTypeTerm>
#include <Nepomuk/Query/QueryServiceClient>
#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Vocabulary/PIMO>

#include <QtCore/QMutexLocker>
#include <QtGui/QTextDocument>

#include <KUrl>
#include <KDebug>
#include <KIO/Job>
#include <KIO/NetAccess>
#include <KUser>
#include <KMimeType>
#include <KConfig>
#include <KConfigGroup>


Nepomuk::SearchFolder::SearchFolder( const KUrl& url, KIO::SlaveBase* slave )
    : QThread(),
      m_url( url ),
      m_initialListingFinished( false ),
      m_slave( slave )
{
    kDebug() <<  url;

    qRegisterMetaType<QList<QUrl> >();

    // parse URL (this may fail in which case we fall back to pure SPARQL below)
    Query::parseQueryUrl( url, m_query, m_sparqlQuery );
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
    connect( m_client, SIGNAL( resultCount(int) ),
             this, SLOT( slotResultCount(int) ),
             Qt::DirectConnection );
    connect( m_client, SIGNAL( finishedListing() ),
             this, SLOT( slotFinishedListing() ),
             Qt::DirectConnection );
    connect( m_client, SIGNAL( error(QString) ),
             this, SLOT( slotFinishedListing() ),
             Qt::DirectConnection );

    if ( m_query.isValid() )
        m_client->query( m_query );
    else
        m_client->sparqlQuery( m_sparqlQuery );
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

    // shutdown and delete
    exit();
    deleteLater();
}


// always called in search thread
void Nepomuk::SearchFolder::slotNewEntries( const QList<Nepomuk::Query::Result>& results )
{
//    kDebug() << m_url;

    m_resultMutex.lock();
    m_resultsQueue += results;
    m_resultMutex.unlock();

    if ( !m_initialListingFinished ) {
        m_resultWaiter.wakeAll();
    }
}


void Nepomuk::SearchFolder::slotResultCount( int count )
{
    if ( !m_initialListingFinished ) {
        QMutexLocker lock( &m_slaveMutex );
        m_slave->totalSize( count );
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
//                kDebug() << "listing" << result.resource().resourceUri();
                QMutexLocker lock( &m_slaveMutex );
                m_slave->listEntries( KIO::UDSEntryList() << uds );
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
    bool statFile( const KUrl& url, const KUrl& fileUrl, KIO::UDSEntry& uds )
    {
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

        Nepomuk::Resource res( url );
        if ( res.exists() ) {
            uds = Nepomuk::statNepomukResource( res );
            return true;
        }

        kDebug() << "failed to stat" << url;
        return false;
    }
}


// always called in main thread
// This method tries to avoid loading the Nepomuk::Resource as long as possible by only using the
// request property nie:url in the Result for local files.
KIO::UDSEntry Nepomuk::SearchFolder::statResult( const Query::Result& result )
{
    Resource res( result.resource() );
    const KUrl uri( res.resourceUri() );
    KUrl nieUrl( result[Nepomuk::Vocabulary::NIE::url()].uri() );

    // the additional bindings that we only have on unix systems
    // Either all are bound or none of them.
    // see also parseQueryUrl (queryutils.h)
    const Soprano::BindingSet additionalVars = result.additionalBindings();

    // the UDSEntry that will contain the final result to list
    KIO::UDSEntry uds;

#ifdef Q_OS_UNIX
    if( !nieUrl.isEmpty() &&
            nieUrl.isLocalFile() &&
            additionalVars[QLatin1String("mtime")].isLiteral() ) {
        // make sure we have unique names for everything
        uds.insert( KIO::UDSEntry::UDS_NAME, resourceUriToUdsName( nieUrl ) );

        // set the name the user will see
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, nieUrl.fileName() );

        // set the basic file information which we got from Nepomuk
        uds.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, additionalVars[QLatin1String("mtime")].literal().toDateTime().toTime_t() );
        uds.insert( KIO::UDSEntry::UDS_SIZE, additionalVars[QLatin1String("size")].literal().toInt() );
        uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, additionalVars[QLatin1String("mode")].literal().toInt() & S_IFMT );
        uds.insert( KIO::UDSEntry::UDS_ACCESS, additionalVars[QLatin1String("mode")].literal().toInt() & 07777 );
        uds.insert( KIO::UDSEntry::UDS_USER, additionalVars[QLatin1String("user")].toString() );
        uds.insert( KIO::UDSEntry::UDS_GROUP, additionalVars[QLatin1String("group")].toString() );

        // since we change the UDS_NAME KFileItem cannot handle mimetype and such anymore
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, additionalVars[QLatin1String("mime")].toString() );
        if( uds.stringValue(KIO::UDSEntry::UDS_MIME_TYPE).isEmpty())
            uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, KMimeType::findByUrl(nieUrl)->name() );
    }
    else
#endif // Q_OS_UNIX
    {
        // not a simple local file result

        // check if we have a pimo thing relating to a file
        if ( nieUrl.isEmpty() )
            nieUrl = Nepomuk::nepomukToFileUrl( uri );

        // try to stat the file
        if ( statFile( uri, nieUrl, uds ) ) {
            // make sure we have unique names for everything
            // We encode the resource URL or URI into the name so subsequent calls to stat or
            // other non-listing commands can easily forward to the appropriate slave.
            if ( !nieUrl.isEmpty() ) {
                uds.insert( KIO::UDSEntry::UDS_NAME, resourceUriToUdsName( nieUrl ) );
            }
            else {
                uds.insert( KIO::UDSEntry::UDS_NAME, resourceUriToUdsName( uri ) );
            }

            // make sure we do not use these ugly names for display
            if ( !uds.contains( KIO::UDSEntry::UDS_DISPLAY_NAME ) ) {
                if ( nieUrl.isEmpty() &&
                        res.hasType( Nepomuk::Vocabulary::PIMO::Thing() ) ) {
                    if ( !res.pimoThing().groundingOccurrences().isEmpty() ) {
                        res = res.pimoThing().groundingOccurrences().first();
                        nieUrl = res.property(Nepomuk::Vocabulary::NIE::url()).toUrl();
                    }
                }

                if ( !nieUrl.isEmpty() ) {
                    uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, nieUrl.fileName() );

                    // since we change the UDS_NAME KFileItem cannot handle mimetype and such anymore
                    QString mimetype = uds.stringValue( KIO::UDSEntry::UDS_MIME_TYPE );
                    if ( mimetype.isEmpty() ) {
                        mimetype = KMimeType::findByUrl(nieUrl)->name();
                        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, mimetype );
                    }
                }
                else {
                    uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, res.genericLabel() );
                }
            }
        }
        else {
            kDebug() << "Stating" << result.resource().resourceUri() << "failed";
            return KIO::UDSEntry();
        }
    }

    if( !nieUrl.isEmpty() ) {
        // There is a trade-off between using UDS_URL or not. The advantage is that we get proper
        // file names in opening applications and non-KDE apps can handle the URLs properly. The downside
        // is that we lose the context information, i.e. query results cannot be browsed in the opening
        // application. We decide pro-filenames and pro-non-kde-apps here.
        if( !uds.isDir() ) {
            uds.insert( KIO::UDSEntry::UDS_TARGET_URL, nieUrl.url() );
        }

        // set the local path so that KIO can handle the rest
        if( nieUrl.isLocalFile() )
            uds.insert( KIO::UDSEntry::UDS_LOCAL_PATH, nieUrl.toLocalFile() );
    }
    else {
        uds.insert( KIO::UDSEntry::UDS_TARGET_URL, uri.url() );
    }

    // Tell KIO which Nepomuk resource this actually is
    uds.insert( KIO::UDSEntry::UDS_NEPOMUK_URI, uri.url() );

    // add optional full-text search excerpts
    QString excerpt = result.excerpt();
    if( !excerpt.isEmpty() ) {
        // KFileItemDelegate cannot handle rich text yet. Thus we need to remove the formatting.
        QTextDocument doc;
        doc.setHtml(excerpt);
        excerpt = doc.toPlainText();
        uds.insert( KIO::UDSEntry::UDS_COMMENT, i18n("Search excerpt: %1", excerpt) );
    }

    return uds;
}
