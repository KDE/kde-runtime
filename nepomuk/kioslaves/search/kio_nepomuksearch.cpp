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

#include <QtCore/QFile>

#include <KUser>
#include <KDebug>
#include <KAboutData>
#include <KApplication>
#include <KCmdLineArgs>
#include <kio/global.h>
#include <kio/job.h>
#include <KMimeType>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>
#include "queryparser.h"

#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/Xesam>

#include <sys/types.h>
#include <unistd.h>


namespace {
    KIO::UDSEntry statDefaultSearchFolder( const QString& name ) {
        KIO::UDSEntry uds;
        uds.insert( KIO::UDSEntry::UDS_NAME, name );
        uds.insert( KIO::UDSEntry::UDS_ACCESS, 0700 );
        uds.insert( KIO::UDSEntry::UDS_USER, KUser().loginName() );
        uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1( "inode/directory" ) );
        return uds;
    }

    // do not cache more than SEARCH_CACHE_MAX search folders at the same time
    const int SEARCH_CACHE_MAX = 5;
}


Nepomuk::SearchProtocol::SearchProtocol( const QByteArray& poolSocket, const QByteArray& appSocket )
    : KIO::ForwardingSlaveBase( "nepomuksearch", poolSocket, appSocket )
{
    // FIXME: load default searches from config
    // FIXME: allow icons

    // all music files
    Search::Term musicOrTerm;
    musicOrTerm.setType( Search::Term::OrTerm );
    musicOrTerm.addSubTerm( Search::Term( Soprano::Vocabulary::RDF::type(),
                                          Soprano::Vocabulary::Xesam::Music() ) );
    musicOrTerm.addSubTerm( Search::Term( Soprano::Vocabulary::RDF::type(),
                                          Nepomuk::Vocabulary::NFO::Audio() ) );
    addDefaultSearch( i18n( "All Music Files" ), musicOrTerm );

    // select the 10 most recent files:
    addDefaultSearch( i18n( "Recent Files" ),
                      Search::Query( QString( "select distinct ?r where { "
                                              "?r a %1 . "
                                              "?r %2 ?date . "
                                              "} ORDER BY DESC(?date) LIMIT 10" )
                                     .arg(Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NFO::FileDataObject() ))
                                     .arg(Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NFO::fileLastModified() )) ) );
}


Nepomuk::SearchProtocol::~SearchProtocol()
{
}


void Nepomuk::SearchProtocol::addDefaultSearch( const QString& name, const Search::Query& q )
{
    Search::Query query( q );
    query.addRequestProperty( Soprano::Vocabulary::Xesam::url(), true );
    query.addRequestProperty( Nepomuk::Vocabulary::NFO::fileUrl(), true );
    m_defaultSearches.insert( name, query );
}


Nepomuk::SearchFolder* Nepomuk::SearchProtocol::extractSearchFolder( const KUrl& url )
{
    QString name = url.path().section( '/', 0, 0, QString::SectionSkipEmpty );
    kDebug() << url << name;
    if ( SearchFolder* sf = getDefaultQueryFolder( name ) ) {
        kDebug() << "-----> is default search folder";
        return sf;
    }
    else if ( SearchFolder* sf = getQueryResults( name ) ) {
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
    kDebug() << url;

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

    if ( url.path() == "/" ) {
        listRoot();
    }
    else if ( url.directory() == "/" &&
              m_defaultSearches.contains( url.fileName() ) ) {
        // the default search name is the folder name
        listDefaultSearch( url.fileName() );
    }
    else {
        // lets create an on-the-fly search
        listQuery( url.fileName() );
    }
}


void Nepomuk::SearchProtocol::get( const KUrl& url )
{
    kDebug() << url;
    ForwardingSlaveBase::get( url );
}


void Nepomuk::SearchProtocol::put( const KUrl& url, int permissions, KIO::JobFlags flags )
{
    kDebug() << url << permissions << flags;
    // this will work only for existing files (ie. overwrite to allow saving of opened files)
    ForwardingSlaveBase::put( url, permissions, flags );
}


void Nepomuk::SearchProtocol::mimetype( const KUrl& url )
{
    kDebug() << url;

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

    if ( url.path() == "/" ) {
        if ( url.queryItems().isEmpty() ) {
            kDebug() << "/";
            //
            // stat the root path
            //
            KIO::UDSEntry uds;
            uds.insert( KIO::UDSEntry::UDS_NAME, QString::fromLatin1( "/" ) );
            uds.insert( KIO::UDSEntry::UDS_ICON_NAME, QString::fromLatin1( "nepomuk" ) );
            uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
            uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1( "inode/directory" ) );

            statEntry( uds );
            finished();
        }
        else {
            kDebug() << "query folder:" << url.queryItemValue("query");

            //
            // stat a query folder
            //
            KIO::UDSEntry uds;
            uds.insert( KIO::UDSEntry::UDS_NAME, url.fileName() );
            uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
            uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1( "inode/directory" ) );

            statEntry( uds );
            finished();
        }
    }
    else if ( url.directory() == "/" ) {
        if ( SearchFolder* sf = extractSearchFolder( url ) ) {
            KIO::UDSEntry uds = statDefaultSearchFolder( sf->name() );
            Q_ASSERT( !uds.stringValue( KIO::UDSEntry::UDS_NAME ).isEmpty() );
            statEntry( uds );
            finished();
        }
        else {
            error( KIO::ERR_DOES_NOT_EXIST, url.url() );
        }
    }
    else if ( SearchFolder* folder = extractSearchFolder( url ) ) {
        folder->stat( url.fileName() );
    }
    else {
        error( KIO::ERR_DOES_NOT_EXIST, url.url() );
    }
}


bool Nepomuk::SearchProtocol::rewriteUrl( const KUrl& url, KUrl& newURL )
{
    kDebug() << url << newURL;

    if ( SearchFolder* folder = extractSearchFolder( url ) ) {
        if ( SearchEntry* entry = folder->findEntry( url.fileName() ) ) {
            QString localPath = entry->entry().stringValue( KIO::UDSEntry::UDS_LOCAL_PATH );
            if ( localPath.isEmpty() ) {
                newURL = localPath;
            }
            else {
                newURL = entry->resource();
            }
            return true;
        }
    }

    return false;
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


Nepomuk::SearchFolder* Nepomuk::SearchProtocol::getQueryResults( const QString& query )
{
    if ( m_searchCache.contains( query ) ) {
        return m_searchCache[query];
    }
    else {
        if ( m_searchCache.count() >= SEARCH_CACHE_MAX ) {
            QString oldestQuery = m_searchCacheNameQueue.dequeue();
            delete m_searchCache.take( oldestQuery );
        }

        Search::Query q = Nepomuk::Search::QueryParser::parseQuery( query );
        q.addRequestProperty( Soprano::Vocabulary::Xesam::url(), true );
        q.addRequestProperty( Nepomuk::Vocabulary::NFO::fileUrl(), true );
        SearchFolder* folder = new SearchFolder( query, q, this );
        m_searchCacheNameQueue.enqueue( query );
        m_searchCache.insert( query, folder );
        return folder;
    }
}


Nepomuk::SearchFolder* Nepomuk::SearchProtocol::getDefaultQueryFolder( const QString& name )
{
    if ( m_defaultSearchCache.contains( name ) ) {
        return m_defaultSearchCache[name];
    }
    else if ( m_defaultSearches.contains( name ) ) {
        SearchFolder* folder = new SearchFolder( name, m_defaultSearches[name], this );
        m_defaultSearchCache.insert( name, folder );
        return folder;
    }
    else {
        return 0;
    }
}


void Nepomuk::SearchProtocol::listQuery( const QString& query )
{
    kDebug() << query;
    getQueryResults( query )->list();
}


void Nepomuk::SearchProtocol::listDefaultSearches()
{
    for ( QHash<QString, Nepomuk::Search::Query>::const_iterator it = m_defaultSearches.constBegin();
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
        KComponentData( "kio_nepomuksearch" );
        QCoreApplication app( argc, argv );

        if ( Nepomuk::ResourceManager::instance()->init() ) {
            kError() << "Unable to initialized Nepomuk.";
            return -1;
        }

        kDebug(7102) << "Starting nepomuksearch slave " << getpid();

        Nepomuk::SearchProtocol slave( argv[2], argv[3] );
        slave.dispatchLoop();

        kDebug(7102) << "Nepomuksearch slave Done";

        return 0;
    }
}

#include "kio_nepomuksearch.moc"
