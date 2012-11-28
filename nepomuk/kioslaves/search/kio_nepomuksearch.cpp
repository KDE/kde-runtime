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

#include <Nepomuk2/ResourceManager>
#include <Nepomuk2/Variant>
#include <Nepomuk2/Query/QueryServiceClient>
#include <Nepomuk2/Query/ComparisonTerm>
#include <Nepomuk2/Query/ResourceTypeTerm>
#include <Nepomuk2/Query/AndTerm>
#include <Nepomuk2/Query/NegationTerm>
#include <Nepomuk2/Query/Query>

#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/XMLSchema>
#include <Nepomuk2/Vocabulary/NFO>
#include <Nepomuk2/Vocabulary/NIE>
#include <Nepomuk2/Vocabulary/PIMO>

#include <sys/types.h>
#include <unistd.h>

using namespace Nepomuk2::Vocabulary;

namespace {
    void addGenericNepomukResourceData( const Nepomuk2::Resource& res, KIO::UDSEntry& uds, bool includeMimeType = true )
    {
        //
        // Add some random values
        //
        uds.insert( KIO::UDSEntry::UDS_ACCESS, 0700 );
        uds.insert( KIO::UDSEntry::UDS_USER, KUser().loginName() );
        if ( res.hasProperty( NIE::lastModified() ) ) {
            // remotely stored files
            uds.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, res.property( NIE::lastModified() ).toDateTime().toTime_t() );
        }
        else {
            // all nepomuk resources
            uds.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, res.property( Soprano::Vocabulary::NAO::lastModified() ).toDateTime().toTime_t() );
            uds.insert( KIO::UDSEntry::UDS_CREATION_TIME, res.property( Soprano::Vocabulary::NAO::created() ).toDateTime().toTime_t() );
        }

        if ( res.hasProperty( NIE::contentSize() ) ) {
            // remotely stored files
            uds.insert( KIO::UDSEntry::UDS_SIZE, res.property( NIE::contentSize() ).toInt() );
        }


        //
        // Starting with KDE 4.4 we have the pretty UDS_NEPOMUK_URI which makes
        // everything much cleaner since kio slaves can decide if the resources can be
        // annotated or not.
        //
        uds.insert( KIO::UDSEntry::UDS_NEPOMUK_URI, KUrl( res.uri() ).url() );

        if ( includeMimeType ) {
            // Use nice display types like "Person", "Project" and so on
            Nepomuk2::Types::Class type( res.type() );
            if (!type.label().isEmpty())
                uds.insert( KIO::UDSEntry::UDS_DISPLAY_TYPE, type.label() );

            QString icon = res.genericIcon();
            if( icon.isEmpty() )
                icon = QLatin1String("nepomuk");

            uds.insert( KIO::UDSEntry::UDS_ICON_NAME, icon );
            if ( icon != QLatin1String( "nepomuk" ) )
                uds.insert( KIO::UDSEntry::UDS_ICON_OVERLAY_NAMES, QLatin1String( "nepomuk" ) );
        }
    }

    KIO::UDSEntry statSearchFolder( const KUrl& url ) {
        KIO::UDSEntry uds;
        uds.insert( KIO::UDSEntry::UDS_ACCESS, 0700 );
        uds.insert( KIO::UDSEntry::UDS_USER, KUser().loginName() );
        uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1( "inode/directory" ) );
        uds.insert( KIO::UDSEntry::UDS_ICON_OVERLAY_NAMES, QLatin1String( "nepomuk" ) );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_TYPE, i18n( "Query folder" ) );
        uds.insert( KIO::UDSEntry::UDS_NAME, Nepomuk2::Query::Query::titleFromQueryUrl( url ) );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, Nepomuk2::Query::Query::titleFromQueryUrl( url ) );
        if ( url.hasQueryItem( QLatin1String( "resource" ) ) ) {
            Nepomuk2::Resource resource( url.queryItemValue( QLatin1String("resource") ) );
            addGenericNepomukResourceData( resource, uds );
        }
        Nepomuk2::Query::Query query = Nepomuk2::Query::Query::fromQueryUrl( url );
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
        KUrl newUrl(QLatin1String("nepomuksearch:/") + Nepomuk2::Query::Query::titleFromQueryUrl(url));
        Nepomuk2::Query::Query query = Nepomuk2::Query::Query::fromQueryUrl(url);
        if(query.isValid())
            newUrl.addQueryItem(QLatin1String("encodedquery"), query.toString());
        else
            newUrl.addQueryItem(QLatin1String("sparql"), Nepomuk2::Query::Query::sparqlFromQueryUrl(url));
        return newUrl;
    }

    Nepomuk2::Query::Query rootQuery() {
        KConfig config( "kio_nepomuksearchrc" );
        QString queryStr = config.group( "General" ).readEntry( "Root query", QString() );
        Nepomuk2::Query::Query query;
        if ( queryStr.isEmpty() )
            query = Nepomuk2::lastModifiedFilesQuery();
        else
            query = Nepomuk2::Query::Query::fromString( queryStr );
        query.setLimit( config.group( "General" ).readEntry( "Root query limit", 10 ) );
        return query;
    }
    const int s_historyMax = 10;
}


Nepomuk2::SearchProtocol::SearchProtocol( const QByteArray& poolSocket, const QByteArray& appSocket )
    : KIO::SlaveBase( "nepomuksearch", poolSocket, appSocket )
{
}


Nepomuk2::SearchProtocol::~SearchProtocol()
{
}


bool Nepomuk2::SearchProtocol::ensureNepomukRunning( bool emitError )
{
    ResourceManager* rm = ResourceManager::instance();
    if ( !rm->initialized() ) {
        if( rm->init() ) {
            if( emitError )
                error( KIO::ERR_SLAVE_DEFINED, i18n( "The desktop search service is not activated. "
                                                      "Unable to answer queries without it." ) );
            return false;
        }
        return true;
    }

    return true;
}


void Nepomuk2::SearchProtocol::listDir( const KUrl& url )
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
        else {
            SearchFolder folder( url, this );
            updateQueryUrlHistory( url );
            folder.list();
            listEntry( KIO::UDSEntry(), true );
            finished();
        }
    }

    else {
        error( KIO::ERR_CANNOT_ENTER_DIRECTORY, url.prettyUrl() );
        return;
    }
}


void Nepomuk2::SearchProtocol::mimetype( const KUrl& url )
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

    else {
        error( KIO::ERR_CANNOT_ENTER_DIRECTORY, url.prettyUrl() );
        return;
    }
}


void Nepomuk2::SearchProtocol::stat( const KUrl& url )
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

    else {
        error( KIO::ERR_CANNOT_ENTER_DIRECTORY, url.prettyUrl() );
        return;
    }
}


void Nepomuk2::SearchProtocol::listRoot()
{
    kDebug();

    Query::Query query = rootQuery();
    if ( query.isValid() ) {
        // FIXME: Avoid this useless conversion to searchUrl and back
        SearchFolder folder( query.toSearchUrl(), this );
        folder.list();
    }

    listEntry( KIO::UDSEntry(), true );
    finished();
}


void Nepomuk2::SearchProtocol::updateQueryUrlHistory( const KUrl& url )
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

        Nepomuk2::SearchProtocol slave( argv[2], argv[3] );
        slave.dispatchLoop();

        kDebug(7102) << "Nepomuksearch slave Done";

        return 0;
    }
}


#if 0
void Nepomuk2::SearchProtocol::listUserQueries()
{
    UserQueryUrlList userQueries;
    Q_FOREACH( const KUrl& url, userQueries ) {
        KIO::UDSEntry uds = statSearchFolder( url );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_TYPE, i18n( "Saved Query" ) );
        listEntry( uds, false );
    }
}
void Nepomuk2::SearchProtocol::listLastQueries()
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
