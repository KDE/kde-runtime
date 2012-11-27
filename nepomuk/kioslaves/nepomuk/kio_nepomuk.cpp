/*
   Copyright 2008-2010 Sebastian Trueg <trueg@kde.org>
   Copyright 2012 Vishesh Handa <me@vhanda.in>

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

#include "kio_nepomuk.h"
#include "resourcepagegenerator.h"
#include "nepomuksearchurltools.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>

#include <KComponentData>
#include <KDebug>
#include <KUser>
#include <kdeversion.h>
#include <KMessageBox>
#include <KMimeType>

#include <nepomuk2/resourcemanager.h>
#include <nepomuk2/variant.h>
#include <nepomuk2/class.h>
#include <nepomuk2/property.h>
#include <Nepomuk2/Query/Query>
#include <Nepomuk2/Query/ComparisonTerm>
#include <Nepomuk2/Query/ResourceTerm>
#include <Nepomuk2/Vocabulary/NFO>
#include <Nepomuk2/Vocabulary/NIE>
#include <Nepomuk2/Vocabulary/PIMO>
#include <Nepomuk2/File>

#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/QueryResultIterator>
#include <Soprano/NodeIterator>
#include <Soprano/Node>
#include <Soprano/Model>

#include <Solid/Device>
#include <Solid/StorageAccess>

using namespace Nepomuk2::Vocabulary;
using namespace Soprano::Vocabulary;

namespace {
    bool noFollowSet( const KUrl& url ) {
        return( url.encodedQueryItemValue( "noFollow" ) == "true" );
    }

    // The nepomuk kio slave accepts urls of the form - nepomuk:/res/uuid along with some additional
    // query parameters in order to display a web page instead
    KUrl getNepomukUri(const KUrl& url) {
        KUrl newUrl( url );
        newUrl.setEncodedQuery( QByteArray() );
        return newUrl;
    }

    KUrl redirectionUrl( const Nepomuk2::Resource& res ) {
        using namespace Nepomuk2;

        // list folders by forwarding to the actual folder on disk
        if ( res.hasType( NFO::Folder() ) ) {
            return res.property( NIE::url() ).toUrl();
        }

        // list tags by listing everything tagged with that tag
        else if ( res.hasType( NAO::Tag() ) ) {
            Query::ComparisonTerm term( NAO::hasTag(), Query::ResourceTerm( res ), Query::ComparisonTerm::Equal );
            KUrl url = Query::Query( term ).toSearchUrl( i18n( "Things tagged '%1'", res.genericLabel() ) );
            url.addQueryItem( QLatin1String( "resource" ), KUrl( res.uri() ).url() );
            return url;
        }

        // list everything else besides files by querying things related to the resource in some way
        // this works for music albums or artists but it would also work for tags
        else if ( !res.hasType( NFO::FileDataObject() ) ) {
            Query::ComparisonTerm term( QUrl(), Query::ResourceTerm( res ), Query::ComparisonTerm::Equal );
            KUrl url = Query::Query( term ).toSearchUrl( res.genericLabel() );
            url.addQueryItem( QLatin1String( "resource" ), KUrl( res.uri() ).url() );
            return url;
        }

        // no forwarding done
        return KUrl();
    }
}

Nepomuk2::NepomukProtocol::NepomukProtocol( const QByteArray& poolSocket, const QByteArray& appSocket )
    : KIO::ForwardingSlaveBase( "nepomuk", poolSocket, appSocket )
{
}


Nepomuk2::NepomukProtocol::~NepomukProtocol()
{
}


void Nepomuk2::NepomukProtocol::listDir( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    const Resource res = getNepomukUri( url );
    if( !res.exists() ) {
        error( KIO::ERR_DOES_NOT_EXIST, res.uri().toString() );
        return;
    }

    const bool noFollow = noFollowSet( url );
    if( noFollow ) {
        listEntry( KIO::UDSEntry(), true );
        finished();
        return;
    }

    ForwardingSlaveBase::listDir( url );
}


void Nepomuk2::NepomukProtocol::get( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    const Resource res = getNepomukUri( url );
    if( !res.exists() ) {
        error( KIO::ERR_DOES_NOT_EXIST, res.uri().toString() );
        return;
    }

    const bool noFollow = noFollowSet( url );
    if( noFollow ) {
        mimeType( "text/html" );

        ResourcePageGenerator gen( res );
        gen.setFlagsFromUrl( url );
        data( gen.generatePage() );
        finished();
        return;
    }

    ForwardingSlaveBase::get( url );
}


void Nepomuk2::NepomukProtocol::put( const KUrl& url, int permissions, KIO::JobFlags flags )
{
    if ( !ensureNepomukRunning() )
        return;

    ForwardingSlaveBase::put( url, permissions, flags );
}


void Nepomuk2::NepomukProtocol::stat( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    const Resource res = getNepomukUri( url );
    if( !res.exists() ) {
        error( KIO::ERR_DOES_NOT_EXIST, res.uri().toString() );
        return;
    }

    const bool noFollow = noFollowSet( url );
    if( noFollow ) {
        KIO::UDSEntry uds;

        uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, res.genericLabel() );
        uds.insert( KIO::UDSEntry::UDS_NAME, resourceUriToUdsName( res.uri() ) );
        uds.insert( KIO::UDSEntry::UDS_NEPOMUK_URI, KUrl( res.uri() ).url() );

        long long modified = res.property( NAO::lastModified() ).toDateTime().toTime_t();
        long long created = res.property( NAO::created() ).toDateTime().toTime_t();

        uds.insert( KIO::UDSEntry::UDS_CREATION_TIME, created );
        uds.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, modified );

        uds.insert( KIO::UDSEntry::UDS_ACCESS, 0700 );
        uds.insert( KIO::UDSEntry::UDS_USER, KUser().loginName() );

        statEntry( uds );
        finished();
        return;
    }

    ForwardingSlaveBase::stat( url );
}


void Nepomuk2::NepomukProtocol::mimetype( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    const Resource res = getNepomukUri( url );
    if( !res.exists() ) {
        error( KIO::ERR_DOES_NOT_EXIST, res.uri().toString() );
        return;
    }

    if ( noFollowSet( url ) ) {
        mimeType( "text/html" );
        finished();
        return;
    }

    ForwardingSlaveBase::mimetype( url );
}


void Nepomuk2::NepomukProtocol::del(const KUrl& url, bool)
{
    if ( !ensureNepomukRunning() )
        return;

    Resource res = getNepomukUri( url );
    if( !res.exists() ) {
        error( KIO::ERR_DOES_NOT_EXIST, res.uri().toString() );
        return;
    }

    if ( noFollowSet( url ) ) {
        mimeType( "text/html" );
        finished();
        return;
    }

    // If file, then delete the actual file. 
    // The file watch service should take care of the metadata
    if( res.isFile() ) {
        ForwardingSlaveBase::mimetype( url );
        return;
    }
    else {
        res.remove();
        finished();
    }
}


bool Nepomuk2::NepomukProtocol::rewriteUrl( const KUrl& url, KUrl& newURL )
{
    if ( noFollowSet( url ) )
        return false;

    Resource res( url );
    if( res.isFile() ) {
        newURL = res.toFile().url();
    }
    else {
        newURL = redirectionUrl( res );
    }

    kDebug() << "Redirecting " << url << " - > " << newURL;

    return true;
}

void Nepomuk2::NepomukProtocol::prepareUDSEntry(KIO::UDSEntry&, bool) const
{
    // Do nothing
    // We do not want the default implemention which tries to change the UDS_NAME and some
    // other stuff
}


bool Nepomuk2::NepomukProtocol::ensureNepomukRunning()
{
    ResourceManager* rm = ResourceManager::instance();
    if ( !rm->initialized() ) {
        if( rm->init() ) {
            error( KIO::ERR_SLAVE_DEFINED, i18n( "The desktop search service is not activated. "
                                                  "Unable to answer queries without it." ) );
            return false;
        }
        return true;
    }

    return true;
}



extern "C"
{
    int KDE_EXPORT kdemain( int argc, char **argv ) {

        KComponentData componentData("kio_nepomuk");
        QCoreApplication app( argc, argv );

        if (argc != 4)
        {
            fprintf(stderr, "Usage: kio_nepomuk protocol domain-socket1 domain-socket2\n");
            exit(-1);
        }

        Nepomuk2::NepomukProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();

        return 0;
    }
}
