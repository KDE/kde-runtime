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

#include "kio_nepomuksearch.h"
#include "searchfolder.h"
#include "nfo.h"
#include "nie.h"
#include "pimo.h"

#include <QtCore/QFile>

#include <KUser>
#include <KDebug>
#include <KAboutData>
#include <KApplication>
#include <KConfig>
#include <KConfigGroup>
#include <KCmdLineArgs>
#include <kio/global.h>
#include <kio/job.h>
#include <KMimeType>
#include <KStandardDirs>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>
#include <Nepomuk/Query/QueryParser>
#include <Nepomuk/Query/QueryServiceClient>

#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/XMLSchema>

#include <sys/types.h>
#include <unistd.h>


namespace {
    KIO::UDSEntry statDefaultSearchFolder( const QString& name ) {
        KIO::UDSEntry uds;
        uds.insert( KIO::UDSEntry::UDS_NAME, name );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, i18n("Query Results") );
        uds.insert( KIO::UDSEntry::UDS_ACCESS, 0700 );
        uds.insert( KIO::UDSEntry::UDS_USER, KUser().loginName() );
        uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1( "inode/directory" ) );
        return uds;
    }


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
     * Empty if the path only contains the query.
     */
    QString fileNameFromUrl( const KUrl& url ) {
        QString fn;
        if ( url.hasQueryItem( QLatin1String( "sparql" ) ) ||
             url.hasQueryItem( QLatin1String( "query" ) ) ||
             url.directory() != QLatin1String( "/" ) ) {
            return url.fileName();
        }
        else {
            return QString();
        }
    }

    // do not cache more than SEARCH_CACHE_MAX search folders at the same time
    const int SEARCH_CACHE_MAX = 5;
}


Nepomuk::SearchProtocol::SearchProtocol( const QByteArray& poolSocket, const QByteArray& appSocket )
    : KIO::ForwardingSlaveBase( "nepomuksearch", poolSocket, appSocket )
{
    // FIXME: trueg: install a file watch on this file and update it whenever the queries change.
    // FIXME: trueg: also emit a KDirNotify signal to inform KIO about that change
    KConfig config("kionepomukuserqueriesrc" );

    foreach( QString search, config.group("Searches").readEntry("All searches", QStringList() ) )
    {
        search = search.simplified();
        KConfigGroup grp = config.group(search);
        KUrl url( grp.readEntry("Query", QString() ) );
        url.setScheme( QLatin1String( "nepomuksearch" ) );
        QString name = grp.readEntry( "Name", QString() );
        if ( name.isEmpty() )
            name = queryFromUrl(url);

        addDefaultSearch( name, url );
    }
}


Nepomuk::SearchProtocol::~SearchProtocol()
{
}


bool Nepomuk::SearchProtocol::ensureNepomukRunning()
{
    if ( Nepomuk::ResourceManager::instance()->init() ) {
        error( KIO::ERR_SLAVE_DEFINED, i18n( "The Nepomuk system is not activated. Unable to answer queries without it." ) );
        return false;
    }
    else if ( !Nepomuk::Query::QueryServiceClient::serviceAvailable() ) {
        error( KIO::ERR_SLAVE_DEFINED, i18n( "The Nepomuk query service is not running. Unable to answer queries without it." ) );
        return false;
    }
    else {
        return true;
    }
}


void Nepomuk::SearchProtocol::addDefaultSearch( const QString& name, const KUrl& url )
{
    m_defaultSearches.insert( name, url );
}


Nepomuk::SearchFolder* Nepomuk::SearchProtocol::extractSearchFolder( const KUrl& url )
{
    bool sparql;
    QString name = queryFromUrl( url, &sparql );
    kDebug() << url << name;
    if ( SearchFolder* sf = getDefaultQueryFolder( name ) ) {
        kDebug() << "-----> is default search folder";
        return sf;
    }
    else if ( SearchFolder* sf = getQueryFolder( name, sparql ) ) {
        kDebug() << "-----> is on-the-fly search folder";
        return sf;
    }
    else {
        kDebug() << "-----> does not exist.";
        return 0;
    }
}


void Nepomuk::SearchProtocol::listDir( const KUrl& url )
{
    bool sparql = false;
    QString name = queryFromUrl( url, &sparql );
    kDebug() << url << name;

    if ( !ensureNepomukRunning() )
        return;

    //
    // Root dir: * list default searches: "all music files", "recent files"
    //           * list configuration entries: "create new default search"
    //
    // Root dir with query:
    //           * execute the query (cached) and list its results
    //
    // some folder:
    //           * Look for a default search and execute that
    //

    if ( name.isEmpty() ) {
        listRoot();
    }
    else if ( m_defaultSearches.contains( name ) ) {
        // the default search name is the folder name
        listDefaultSearch( name );
    }
    else {
        // lets create an on-the-fly search
        listQuery( name, sparql );
    }
}


void Nepomuk::SearchProtocol::get( const KUrl& url )
{
    kDebug() << url;

    if ( !ensureNepomukRunning() )
        return;

    ForwardingSlaveBase::get( url );
}


void Nepomuk::SearchProtocol::put( const KUrl& url, int permissions, KIO::JobFlags flags )
{
    kDebug() << url << permissions << flags;

    if ( !ensureNepomukRunning() )
        return;

    // this will work only for existing files (ie. overwrite to allow saving of opened files)
    ForwardingSlaveBase::put( url, permissions, flags );
}


void Nepomuk::SearchProtocol::mimetype( const KUrl& url )
{
    kDebug() << url;

    if ( !ensureNepomukRunning() )
        return;

    if ( url.path() == "/" ) {
        mimeType( QString::fromLatin1( "inode/directory" ) );
        finished();
    }
    else if ( url.directory() == "/" &&
              m_defaultSearches.contains( url.fileName() ) ) {
        mimeType( QString::fromLatin1( "inode/directory" ) );
        finished();
    }
    else {
        ForwardingSlaveBase::mimetype( url );
    }
}


void Nepomuk::SearchProtocol::stat( const KUrl& url )
{
    kDebug() << url;

    QString name = queryFromUrl( url );
    kDebug() << url << name;

    if ( !ensureNepomukRunning() )
        return;

    if ( name.isEmpty() ) {
        kDebug() << "Stat root" << url;
        //
        // stat the root path
        //
        KIO::UDSEntry uds;
        uds.insert( KIO::UDSEntry::UDS_NAME, QString::fromLatin1( "/" ) );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, i18n("Query Results") );
        uds.insert( KIO::UDSEntry::UDS_ICON_NAME, QString::fromLatin1( "nepomuk" ) );
        uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1( "inode/directory" ) );

        statEntry( uds );
        finished();
    }
    else if ( fileNameFromUrl( url ).isEmpty() ) {
        kDebug() << "Stat search folder" << url;
        statEntry( statDefaultSearchFolder( name ) );
        finished();
    }
    else {
        kDebug() << "Stat forward" << url;
        ForwardingSlaveBase::stat(url);
    }
}


void Nepomuk::SearchProtocol::del(const KUrl& url, bool isFile)
{
    if ( !ensureNepomukRunning() )
        return;

    Nepomuk::SearchFolder* folder = extractSearchFolder( url );

    if (folder) {
        if ( folder->findEntry( url.fileName() ) ) {
            kDebug() << "findEntry returned something";
            KIO::ForwardingSlaveBase::del(url, isFile);
        }
        else {
            kDebug() << "findEntry returned nothing";
            error( KIO::ERR_DOES_NOT_EXIST, url.fileName() ); // not in m_entries
        }
    }
    else {
        kDebug() << "ERROR : extractSearchFolder returned NOTHING";
        error( KIO::ERR_DOES_NOT_EXIST, url.fileName() );
    }
}


bool Nepomuk::SearchProtocol::rewriteUrl( const KUrl& url, KUrl& newURL )
{
    if ( SearchFolder* folder = extractSearchFolder( url ) ) {
        if ( SearchEntry* entry = folder->findEntry( url.fileName() ) ) {
            newURL = entry->resource();
            kDebug() << url << newURL;
            return true;
        }
    }

    return false;
}


void Nepomuk::SearchProtocol::prepareUDSEntry( KIO::UDSEntry&, bool ) const
{
    // we already handle UDS_URL in SearchFolder. No need to do anything more here.
}


void Nepomuk::SearchProtocol::listRoot()
{
    kDebug();

    listDefaultSearches();
    listActions();

    listEntry( KIO::UDSEntry(), true );
    finished();
}


void Nepomuk::SearchProtocol::listActions()
{
    // FIXME: manage default searches
}


Nepomuk::SearchFolder* Nepomuk::SearchProtocol::getQueryFolder( const QString& queryString, bool sparql )
{
    if ( m_searchCache.contains( queryString ) ) {
        return m_searchCache[queryString];
    }
    else {
        if ( m_searchCache.count() >= SEARCH_CACHE_MAX ) {
            QString oldestQuery = m_searchCacheNameQueue.dequeue();
            delete m_searchCache.take( oldestQuery );
        }

        QString q( queryString );
        if ( !sparql ) {
            Query::Query query = Query::QueryParser::parseQuery( queryString );
            query.addRequestProperty( Query::Query::RequestProperty( Nepomuk::Vocabulary::NIE::url(), true ) );
            q = query.toSparqlQuery();
        }

        SearchFolder* folder = new SearchFolder( queryString, q, this );
        m_searchCacheNameQueue.enqueue( queryString );
        m_searchCache.insert( queryString, folder );
        return folder;
    }
}


Nepomuk::SearchFolder* Nepomuk::SearchProtocol::getDefaultQueryFolder( const QString& name )
{
    if ( m_defaultSearchCache.contains( name ) ) {
        return m_defaultSearchCache[name];
    }
    else if ( m_defaultSearches.contains( name ) ) {
        bool sparql = false;
        QString query = queryFromUrl( m_defaultSearches[name], &sparql );
        SearchFolder* folder = getQueryFolder( query, sparql );
        m_defaultSearchCache.insert( name, folder );
        return folder;
    }
    else {
        return 0;
    }
}


void Nepomuk::SearchProtocol::listQuery( const QString& query, bool sparql )
{
    kDebug() << query;
    getQueryFolder( query, sparql )->list();
}


void Nepomuk::SearchProtocol::listDefaultSearches()
{
    for ( QHash<QString, KUrl>::const_iterator it = m_defaultSearches.constBegin();
          it != m_defaultSearches.constEnd(); ++it ) {
        listEntry( statDefaultSearchFolder( it.key() ), false );
    }
}


void Nepomuk::SearchProtocol::listDefaultSearch( const QString& name )
{
    kDebug() << name;
    if ( m_defaultSearches.contains( name ) ) {
        getDefaultQueryFolder( name )->list();
    }
    else {
        error( KIO::ERR_CANNOT_ENTER_DIRECTORY, "Unknown default search: " + name );
        finished();
    }
}

extern "C"
{
    KDE_EXPORT int kdemain( int argc, char **argv )
    {
        // necessary to use other kio slaves
        KComponentData comp( "kio_nepomuksearch" );
        QCoreApplication app( argc, argv );

        kDebug(7102) << "Starting nepomuksearch slave " << getpid();

        Nepomuk::SearchProtocol slave( argv[2], argv[3] );
        slave.dispatchLoop();

        kDebug(7102) << "Nepomuksearch slave Done";

        return 0;
    }
}

#include "kio_nepomuksearch.moc"
