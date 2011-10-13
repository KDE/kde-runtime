/*
   Copyright (C) 2008-2010 by Sebastian Trueg <trueg at kde.org>

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
#include "nepomuksearchurltools.h"
#include "standardqueries.h"
#include "resourcestat.h"

#include <QtCore/QFile>

#include <KUser>
#include <KDebug>
#include <KAboutData>
#include <KApplication>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KCmdLineArgs>
#include <kio/global.h>
#include <kio/job.h>
#include <KMimeType>
#include <KStandardDirs>
#include <KFileItem>

#include <Nepomuk/Thing>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>
#include <Nepomuk/Query/QueryServiceClient>
#include <Nepomuk/Query/ComparisonTerm>
#include <Nepomuk/Query/ResourceTypeTerm>
#include <Nepomuk/Query/AndTerm>
#include <Nepomuk/Query/NegationTerm>
#include <Nepomuk/Query/Query>

#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/XMLSchema>
#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Vocabulary/PIMO>

#include <sys/types.h>
#include <unistd.h>


namespace {
    KIO::UDSEntry statSearchFolder( const KUrl& url ) {
        KIO::UDSEntry uds;
        uds.insert( KIO::UDSEntry::UDS_ACCESS, 0700 );
        uds.insert( KIO::UDSEntry::UDS_USER, KUser().loginName() );
        uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1( "inode/directory" ) );
        uds.insert( KIO::UDSEntry::UDS_ICON_OVERLAY_NAMES, QLatin1String( "nepomuk" ) );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_TYPE, i18n( "Query folder" ) );
        uds.insert( KIO::UDSEntry::UDS_NAME, Nepomuk::Query::Query::titleFromQueryUrl( url ) );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, Nepomuk::Query::Query::titleFromQueryUrl( url ) );
        if ( url.hasQueryItem( QLatin1String( "resource" ) ) ) {
            Nepomuk::addGenericNepomukResourceData( Nepomuk::Resource( KUrl( url.queryItemValue( QLatin1String( "resource" ) ) ) ), uds );
        }
        Nepomuk::Query::Query query = Nepomuk::Query::Query::fromQueryUrl( url );
        if ( query.isValid() )
            uds.insert( KIO::UDSEntry::UDS_NEPOMUK_QUERY, query.toString() );
        return uds;
    }

    bool isRootUrl( const KUrl& url ) {
        const QString path = url.path(KUrl::RemoveTrailingSlash);
        return( !url.hasQuery() &&
                ( path.isEmpty() || path == QLatin1String("/") ) );
    }

    // a query folder has a non-empty path with a single section and a query parameter
    // Example: nepomuksearch:/My Query?query=foobar
    bool isQueryFolder( const KUrl& url ) {
        return( url.hasQuery() &&
                url.directory() == QLatin1String("/") );
    }

    // Legacy query URLs look like: nepomuksearch:/?query=xyz&title=foobar
    // i.e. an empty path and a query, new URLs have their title as path
    bool isLegacyQueryUrl( const KUrl& url ) {
        const QString path = url.path(KUrl::RemoveTrailingSlash);
        return( url.hasQuery() &&
                ( path.isEmpty() || path == QLatin1String("/") ) );
    }

    KUrl convertLegacyQueryUrl( const KUrl& url ) {
        KUrl newUrl(QLatin1String("nepomuksearch:/") + Nepomuk::Query::Query::titleFromQueryUrl(url));
        Nepomuk::Query::Query query = Nepomuk::Query::Query::fromQueryUrl(url);
        if(query.isValid())
            newUrl.addQueryItem(QLatin1String("encodedquery"), query.toString());
        else
            newUrl.addQueryItem(QLatin1String("sparql"), Nepomuk::Query::Query::sparqlFromQueryUrl(url));
        return newUrl;
    }

    Nepomuk::Query::Query rootQuery() {
        KConfig config( "kio_nepomuksearchrc" );
        QString queryStr = config.group( "General" ).readEntry( "Root query", QString() );
        Nepomuk::Query::Query query;
        if ( queryStr.isEmpty() )
            query = Nepomuk::lastModifiedFilesQuery();
        else
            query = Nepomuk::Query::Query::fromString( queryStr );
        query.setLimit( config.group( "General" ).readEntry( "Root query limit", 10 ) );
        return query;
    }
    const int s_historyMax = 10;
}


Nepomuk::SearchProtocol::SearchProtocol( const QByteArray& poolSocket, const QByteArray& appSocket )
    : KIO::ForwardingSlaveBase( "nepomuksearch", poolSocket, appSocket )
{
}


Nepomuk::SearchProtocol::~SearchProtocol()
{
}


bool Nepomuk::SearchProtocol::ensureNepomukRunning( bool emitError )
{
    if ( Nepomuk::ResourceManager::instance()->init() ) {
        kDebug() << "Failed to init Nepomuk";
        if ( emitError )
            error( KIO::ERR_SLAVE_DEFINED, i18n( "The desktop search service is not activated. Unable to answer queries without it." ) );
        return false;
    }
    else if ( !Nepomuk::Query::QueryServiceClient::serviceAvailable() ) {
        kDebug() << "Nepomuk Query service is not running.";
        if ( emitError )
            error( KIO::ERR_SLAVE_DEFINED, i18n( "The desktop search query service is not running. Unable to answer queries without it." ) );
        return false;
    }
    else {
        return true;
    }
}


void Nepomuk::SearchProtocol::listDir( const KUrl& url )
{
    kDebug() << url;

    // list the root folder
    if ( isRootUrl( url ) ) {
        listRoot();
    }

    // backwards compatibility with pre-4.6 query URLs
    else if( isLegacyQueryUrl( url ) ) {
        redirection( convertLegacyQueryUrl(url) );
        finished();
    }

    // list the actual query folders
    else if( isQueryFolder( url ) ) {
        if ( !ensureNepomukRunning(false) ) {
            // we defer the listing to later when Nepomuk is up and running
            listEntry( KIO::UDSEntry(),  true);
            finished();
        }
        else if ( SearchFolder* folder = getQueryFolder( url ) ) {
            updateQueryUrlHistory( url );
            folder->list();
            listEntry( KIO::UDSEntry(), true );
            finished();
        }
        else {
            error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
        }
    }

    // listing of query results that are folders
    else {
        ForwardingSlaveBase::listDir(url);
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

    // the root url is always a folder
    if ( isRootUrl( url ) ) {
        mimeType( QString::fromLatin1( "inode/directory" ) );
        finished();
    }

    // Query result URLs in the root folder do not include a query
    // while all query folders do. The latter ones are what we check
    // for here.
    else if ( url.directory() == QLatin1String( "/" ) &&
              url.hasQuery() ) {
        mimeType( QString::fromLatin1( "inode/directory" ) );
        finished();
    }

    // results are forwarded
    else {
        ForwardingSlaveBase::mimetype( url );
    }
}


void Nepomuk::SearchProtocol::stat( const KUrl& url )
{
    kDebug() << url;

    // the root folder
    if ( isRootUrl( url ) ) {
        kDebug() << "Stat root" << url;
        //
        // stat the root path
        //
        KIO::UDSEntry uds;
        uds.insert( KIO::UDSEntry::UDS_NAME, QString::fromLatin1( "/" ) );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, i18n("Desktop Queries") );
        uds.insert( KIO::UDSEntry::UDS_ICON_NAME, QString::fromLatin1( "nepomuk" ) );
        uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1( "inode/directory" ) );
        uds.insert( KIO::UDSEntry::UDS_NEPOMUK_QUERY, rootQuery().toString() );

        statEntry( uds );
        finished();
    }

    // query folders
    else if( isQueryFolder( url ) ) {
        kDebug() << "Stat search folder" << url;
        statEntry( statSearchFolder( url ) );
        finished();
    }

    // results are forwarded
    else {
        kDebug() << "Stat forward" << url;
        ForwardingSlaveBase::stat(url);
    }
}


void Nepomuk::SearchProtocol::del(const KUrl& url, bool isFile)
{
    ForwardingSlaveBase::del( url, isFile );
}


bool Nepomuk::SearchProtocol::rewriteUrl( const KUrl& url, KUrl& newURL )
{
    // we do it the speedy but slightly umpf way: decode the encoded URI from the filename
    newURL = Nepomuk::udsNameToResourceUri( url.fileName() );
    kDebug() << "URL:" << url << "NEW URL:" << newURL << newURL.protocol() << newURL.path() << newURL.fileName();
    return !newURL.isEmpty();
}


void Nepomuk::SearchProtocol::prepareUDSEntry( KIO::UDSEntry& uds, bool listing ) const
{
    // do nothing - we do everything in SearchFolder::statResult
    Q_UNUSED(uds);
    Q_UNUSED(listing);
}


void Nepomuk::SearchProtocol::listRoot()
{
    kDebug();

    // flush
    listEntry( KIO::UDSEntry(), true );

    Query::Query query = rootQuery();
    if ( query.isValid() ) {
        getQueryFolder( query.toSearchUrl() )->list();
    }

    listEntry( KIO::UDSEntry(), true );
    finished();
}


Nepomuk::SearchFolder* Nepomuk::SearchProtocol::getQueryFolder( const KUrl& url )
{
    return new SearchFolder( url, this );
}


void Nepomuk::SearchProtocol::updateQueryUrlHistory( const KUrl& url )
{
    //
    // if the url is already in the history update its timestamp
    // otherwise remove the last item if we reached the max and then
    // add the url along with its timestamp
    //
    KSharedConfigPtr cfg = KSharedConfig::openConfig( "kio_nepomuksearchrc" );
    KConfigGroup grp = cfg->group( "Last Queries" );

    // read config
    const int cnt = grp.readEntry( "count", 0 );
    QList<QPair<KUrl, QDateTime> > entries;
    for ( int i = 0; i < cnt; ++i ) {
        KUrl u = grp.readEntry( QString::fromLatin1( "query_%1_url" ).arg( i ), QString() );
        QDateTime t = grp.readEntry( QString::fromLatin1( "query_%1_timestamp" ).arg( i ), QDateTime() );
        if ( !u.isEmpty() &&
             t.isValid() &&
             u != url ) {
            int pos = 0;
            while ( entries.count() > pos &&
                    entries[pos].second < t ) {
                ++pos;
            }
            entries.insert( pos, qMakePair( u, t ) );
        }
    }
    if ( entries.count() >= s_historyMax ) {
        entries.removeFirst();
    }
    entries.append( qMakePair( url, QDateTime::currentDateTime() ) );

    // write config back
    grp.deleteGroup();
    grp = cfg->group( "Last Queries" );

    for ( int i = 0; i < entries.count(); ++i ) {
        KUrl u = entries[i].first;
        QDateTime t = entries[i].second;
        grp.writeEntry( QString::fromLatin1( "query_%1_url" ).arg( i ), u.url() );
        grp.writeEntry( QString::fromLatin1( "query_%1_timestamp" ).arg( i ), t );
    }
    grp.writeEntry( QLatin1String( "count" ), entries.count() );

    cfg->sync();
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


#if 0
void Nepomuk::SearchProtocol::listUserQueries()
{
    UserQueryUrlList userQueries;
    Q_FOREACH( const KUrl& url, userQueries ) {
        KIO::UDSEntry uds = statSearchFolder( url );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_TYPE, i18n( "Saved Query" ) );
        listEntry( uds, false );
    }
}
void Nepomuk::SearchProtocol::listLastQueries()
{
    KSharedConfigPtr cfg = KSharedConfig::openConfig( "kio_nepomuksearchrc" );
    KConfigGroup grp = cfg->group( "Last Queries" );

    // read config
    const int cnt = grp.readEntry( "count", 0 );
    QList<QPair<KUrl, QDateTime> > entries;
    for ( int i = 0; i < cnt; ++i ) {
        KUrl u = grp.readEntry( QString::fromLatin1( "query_%1_url" ).arg( i ), QString() );
        QDateTime t = grp.readEntry( QString::fromLatin1( "query_%1_timestamp" ).arg( i ), QDateTime() );
        if ( !u.isEmpty() && t.isValid() )
            listEntry( statLastQuery( u, t ), false );
    }

    listEntry( KIO::UDSEntry(), true );
    finished();
}
#endif

#include "kio_nepomuksearch.moc"
