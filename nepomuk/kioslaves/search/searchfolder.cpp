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
#include "nfo.h"
#include "nie.h"
#include "pimo.h"

#include "queryserviceclient.h"

#include <Soprano/Vocabulary/Xesam>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Node> // for qHash( QUrl )

#include <Nepomuk/Variant>
#include <Nepomuk/Thing>
#include <nepomuk/class.h>
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

    m_client = new Nepomuk::Search::QueryServiceClient();

    // results signals are connected directly to update the results cache m_resultsQueue
    // and the entries cache m_entries, as well as emitting KDirNotify signals
    // a queued connection is not possible since we have no event loop after the
    // initial listing which means that queued signals would never get delivered
    connect( m_client, SIGNAL( newEntries( const QList<Nepomuk::Search::Result>& ) ),
             this, SLOT( slotNewEntries( const QList<Nepomuk::Search::Result>& ) ),
             Qt::DirectConnection );
    connect( m_client, SIGNAL( entriesRemoved( const QList<QUrl>& ) ),
             this, SLOT( slotEntriesRemoved( const QList<QUrl>& ) ),
             Qt::DirectConnection );
    connect( m_client, SIGNAL( finishedListing() ),
             this, SLOT( slotFinishedListing() ),
             Qt::DirectConnection );

    m_client->query( m_query );
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
    // FIXME
    return 0;
}


// always called in search thread
void Nepomuk::SearchFolder::slotNewEntries( const QList<Nepomuk::Search::Result>& results )
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
        QHash<QUrl, QString>::iterator it = m_resourceNameMap.find( uri );
        if ( it != m_resourceNameMap.end() ) {
            delete m_entries.take( it.value() );

            // inform everybody
            org::kde::KDirNotify::emitFilesRemoved( QStringList() << ( "nepomuksearch:/" + m_name + '/' + *it ) );

            m_resourceNameMap.erase( it );
        }
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
            Search::Result result = m_resultsQueue.dequeue();
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
    /**
     * Stat a file.
     *
     * \param url The url of the file
     * \param success will be set to \p true if the stat was successful
     */
    KIO::UDSEntry statFile( const KUrl& url, bool& success )
    {
        success = false;
        KIO::UDSEntry uds;

        if ( !url.isEmpty() &&
             url.scheme() != "akonadi" &&
             url.scheme() != "nepomuk" ) { // do not stat akonadi resouces here, way too slow, even hangs if akonadi is not running
            kDebug() << "listing file" << url;
            if ( KIO::StatJob* job = KIO::stat( url, KIO::HideProgressInfo ) ) {
                job->setAutoDelete( false );
                if ( KIO::NetAccess::synchronousRun( job, 0 ) ) {
                    uds = job->statResult();
                    if ( url.isLocalFile() ) {
                        uds.insert( KIO::UDSEntry::UDS_LOCAL_PATH, url.toLocalFile() );
                    }
                    success = true;
                }
                else {
                    kDebug() << "failed to stat" << url;
                }
                delete job;
            }
        }

        return uds;
    }


    /**
     * Workaround a missing Nepomuk::Variant feature which is in trunk but not in 4.3.0.
     */
    QUrl extractUrl( const Nepomuk::Variant& v ) {
        QList<Nepomuk::Resource> rl = v.toResourceList();
        if ( !rl.isEmpty() )
            return rl.first().resourceUri();
        return QUrl();
    }
}

// always called in main thread
Nepomuk::SearchEntry* Nepomuk::SearchFolder::statResult( const Search::Result& result )
{
    //
    // First we check if the resource is a file itself. For that we first get
    // the URL (being backwards compatible with Xesam data) and then stat that URL
    //
    KUrl url = result[Nepomuk::Vocabulary::NIE::url()].uri();
    if ( url.isEmpty() ) {
        url = result[Soprano::Vocabulary::Xesam::url()].uri();
        if ( url.isEmpty() )
            url = result.resourceUri();
    }
    bool isFile = false;
    KIO::UDSEntry uds = statFile( url, isFile );


    if ( isFile ) {
        uds.insert( KIO::UDSEntry::UDS_NEPOMUK_URI, url.url() );
    }

    //
    // If it is not a file we get inventive:
    // In case it is a pimo thing, we see if it has a grounding occurrence
    // which is a file. If so, we merge the two by taking the file URL and the thing's
    // label and icon if set
    //
    else {
        kDebug() << "listing resource" << result.resourceUri();

        //
        // We only create a resource here since this is a rather slow process and
        // a lot of file results could become slow then.
        //
        Nepomuk::Resource res( result.resourceUri() );

        //
        // let's see if it is a pimo thing which refers to a file
        //
        bool isPimoThingLinkedFile = false;
        if ( res.pimoThing() == res ) {
            if ( !res.pimoThing().groundingOccurrences().isEmpty() ) {
                Nepomuk::Resource fileRes = res.pimoThing().groundingOccurrences().first();
                url = extractUrl( fileRes.property( Nepomuk::Vocabulary::NIE::url() ) );
                if ( url.isEmpty() ) {
                    url = extractUrl( fileRes.property( Soprano::Vocabulary::Xesam::url() ) );
                    if ( url.isEmpty() )
                        url = result.resourceUri();
                }
                uds = statFile( url, isPimoThingLinkedFile );
            }
        }

        QString name = res.label();
        if ( name.isEmpty() && !isPimoThingLinkedFile )
            name = res.genericLabel();

        // make sure name is not the URI (which is the fallback of genericLabel() and will lead to crashes in KDirModel)
        if ( name.contains( '/' ) ) {
            name = name.section( '/', -1 );
            if ( name.isEmpty() )
                name = res.resourceUri().fragment();
            if ( name.isEmpty() )
                name = res.resourceUri().toString().replace( '/', '_' );
        }

        //
        // We always use the pimo things label, even if it points to a file
        //
        if ( !name.isEmpty() ) {
            uds.insert( KIO::UDSEntry::UDS_NAME, name );
            uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, name );
        }

        //
        // An icon set on the pimo thing overrides the file's icon
        //
        QString icon = res.genericIcon();
        if ( !icon.isEmpty() ) {
            uds.insert( KIO::UDSEntry::UDS_ICON_NAME, icon );
        }
        else if ( !isPimoThingLinkedFile ) {
            uds.insert( KIO::UDSEntry::UDS_ICON_NAME, "nepomuk" );
        }

        //
        // Generate some dummy values
        //
        if ( !isPimoThingLinkedFile ) {
            uds.insert( KIO::UDSEntry::UDS_CREATION_TIME, res.property( Soprano::Vocabulary::NAO::created() ).toDateTime().toTime_t() );
            uds.insert( KIO::UDSEntry::UDS_ACCESS, 0700 );
            uds.insert( KIO::UDSEntry::UDS_USER, KUser().loginName() );
//        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, "application/x-nepomuk-resource" );
        }

        //
        // We always want the display type, even for pimo thing linked files, in the
        // end showing "Invoice" or "Letter" is better than "text file"
        // However, mimetypes are better than generic stuff like pimo:Thing and pimo:Document
        //
        Nepomuk::Types::Class type( res.pimoThing().isValid() ? res.pimoThing().resourceType() : res.resourceType() );
        if ( !isPimoThingLinkedFile ||
             type.uri() != Nepomuk::Vocabulary::PIMO::Thing() ) {
            if (!type.label().isEmpty())
                uds.insert( KIO::UDSEntry::UDS_DISPLAY_TYPE, type.label() );
        }

        //
        // Starting with KDE 4.4 we have the pretty UDS_NEPOMUK_URI which makes
        // everything much cleaner since kio slaves can decide if the resources can be
        // annotated or not.
        //
        if ( isPimoThingLinkedFile )
            uds.insert( KIO::UDSEntry::UDS_NEPOMUK_URI, KUrl( url ).url() );
        else
            uds.insert( KIO::UDSEntry::UDS_NEPOMUK_URI, KUrl( res.resourceUri() ).url() );
    }

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
