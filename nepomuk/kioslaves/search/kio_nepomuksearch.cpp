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
#include "nfo.h"
#include "nie.h"
#include "pimo.h"
#include "nepomuksearchurltools.h"

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
#include <KDirNotify>

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
        uds.insert( KIO::UDSEntry::UDS_NAME, Nepomuk::resourceUriToUdsName( url ) );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, Nepomuk::Query::Query::titleFromQueryUrl( url ) );
        uds.insert( KIO::UDSEntry::UDS_URL, url.url() );
        return uds;
    }


    /**
     * Empty if the path only contains the query.
     */
    QString fileNameFromUrl( const KUrl& url ) {
        if ( url.hasQueryItem( QLatin1String( "sparql" ) ) ||
             url.hasQueryItem( QLatin1String( "query" ) ) ||
             url.hasQueryItem( QLatin1String( "encodedquery" ) ) ||
             url.directory() != QLatin1String( "/" ) ) {
            return url.fileName();
        }
        else {
            return QString();
        }
    }

    bool isRootUrl( const KUrl& url ) {
        const QString path = url.path(KUrl::RemoveTrailingSlash);
        return( !url.hasQuery() &&
                ( path.isEmpty() || path == QLatin1String("/") ) );
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
            error( KIO::ERR_SLAVE_DEFINED, i18n( "The Nepomuk system is not activated. Unable to answer queries without it." ) );
        return false;
    }
    else if ( !Nepomuk::Query::QueryServiceClient::serviceAvailable() ) {
        kDebug() << "Nepomuk Query service is not running.";
        if ( emitError )
            error( KIO::ERR_SLAVE_DEFINED, i18n( "The Nepomuk query service is not running. Unable to answer queries without it." ) );
        return false;
    }
    else {
        return true;
    }
}


void Nepomuk::SearchProtocol::listDir( const KUrl& url )
{
    kDebug() << url;

    if ( isRootUrl( url ) ) {
        listRoot();
    }
    else {
        if ( !ensureNepomukRunning(false) ) {
            // we defer the listing to later when Nepomuk is up and running
            listEntry( KIO::UDSEntry(),  true);
            finished();
        }
        else if ( SearchFolder* folder = getQueryFolder( url ) ) {
            folder->list();
            listEntry( KIO::UDSEntry(), true );
            finished();
        }
        else {
            error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
        }
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

    if ( isRootUrl( url ) ) {
        mimeType( QString::fromLatin1( "inode/directory" ) );
        finished();
    }
    else if ( url.directory() == QLatin1String( "/" ) ) {
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

        statEntry( uds );
        finished();
    }
    else if ( fileNameFromUrl( url ).isEmpty() ) {
        kDebug() << "Stat search folder" << url;
        statEntry( statSearchFolder( url ) );
        finished();
    }
    else {
        kDebug() << "Stat forward" << url;
        ForwardingSlaveBase::stat(url);
    }
}


void Nepomuk::SearchProtocol::del(const KUrl& url, bool isFile)
{
    if ( isFile ) {
        ForwardingSlaveBase::del( url, isFile );
    }
    else {
        error( KIO::ERR_UNSUPPORTED_ACTION, url.prettyUrl() );
    }
}


bool Nepomuk::SearchProtocol::rewriteUrl( const KUrl& url, KUrl& newURL )
{
    // we do it the speedy but slightly umpf way: decode the encoded URI from the filename
    newURL = Nepomuk::udsNameToResourceUri( url.fileName() );
    kDebug() << "URL:" << url << "NEW URL:" << newURL << newURL.protocol() << newURL.path() << newURL.fileName();
    return !newURL.isEmpty();
}


void Nepomuk::SearchProtocol::prepareUDSEntry( KIO::UDSEntry&, bool ) const
{
    // we already handle UDS_URL in SearchFolder. No need to do anything more here.
}


void Nepomuk::SearchProtocol::listRoot()
{
    kDebug();

    listEntry( KIO::UDSEntry(), true );
    finished();
}


Nepomuk::SearchFolder* Nepomuk::SearchProtocol::getQueryFolder( const KUrl& url )
{
    // this is necessary to properly handle user queries which are encoded in the filename in
    // statSearchFolder(). This is necessary for cases in which UDS_URL is ignored like in
    // KUrlNavigator's popup menus
    KUrl normalizedUrl = Nepomuk::udsNameToResourceUri( url.fileName() );
    if ( normalizedUrl.protocol() != QLatin1String( "nepomuksearch" ) ) {
        normalizedUrl = url;
    }

    // here we strip off the entry's name since that is not part of the query URL
    if ( url.hasQuery() ) {
        normalizedUrl.setPath( QLatin1String( "/" ) );
    }
    else if ( url.directory() != QLatin1String( "/" ) ) {
        normalizedUrl.setPath( QLatin1String( "/" ) + url.path().section( '/', 0, 0 ) );
    }

    SearchFolder* folder = new SearchFolder( normalizedUrl, this );
    return folder;
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
