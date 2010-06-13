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

#include "resourcestat.h"
#include "nepomuksearchurltools.h"
#include "nie.h"
#include "nfo.h"
#include "pimo.h"

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtCore/QFile>

#include <KUrl>
#include <KMimeType>
#include <KUser>
#include <kio/udsentry.h>
#include <KDebug>

#include <Nepomuk/Thing>
#include <Nepomuk/Variant>
#include <Nepomuk/Types/Class>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/ComparisonTerm>
#include <Nepomuk/Query/ResourceTerm>

#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/QueryResultIterator>
#include <Soprano/NodeIterator>
#include <Soprano/Node>
#include <Soprano/Model>

#include <Solid/Device>
#include <Solid/StorageAccess>


KUrl Nepomuk::stripQuery( const KUrl& url )
{
    KUrl newUrl( url );
    newUrl.setEncodedQuery( QByteArray() );
    return newUrl;
}


Nepomuk::Resource Nepomuk::splitNepomukUrl( const KUrl& url, QString* filename )
{
    //
    // let's try to extract the resource from the url in case we listed a tag or
    // filesystem and need to stat the entries in those virtual folders
    //
    // pre KDE 4.4 resources had just a single section, in KDE 4.4 we have "/res/<UUID>"
    //
    QString urlStr = stripQuery( url ).url();
    int pos = urlStr.indexOf( '/', urlStr.startsWith( QLatin1String( "nepomuk:/res/" ) ) ? 13 : 9 );
    if ( pos > 0 ) {
        KUrl resourceUri = urlStr.left(pos);
        if ( filename )
            *filename = urlStr.mid( pos+1 );
        return resourceUri;
    }
    else {
        return stripQuery( url );
    }
}


bool Nepomuk::isRemovableMediaFile( const Nepomuk::Resource& res )
{
    if ( res.hasProperty( Nepomuk::Vocabulary::NIE::url() ) ) {
        KUrl url = res.property( Nepomuk::Vocabulary::NIE::url() ).toUrl();
        return ( url.protocol() == QLatin1String( "filex" ) );
    }
    else {
        return false;
    }
}


Solid::StorageAccess* Nepomuk::storageFromUUID( const QString& uuid )
{
    QString solidQuery = QString::fromLatin1( "[ StorageVolume.usage=='FileSystem' AND StorageVolume.uuid=='%1' ]" ).arg( uuid.toLower() );
    QList<Solid::Device> devices = Solid::Device::listFromQuery( solidQuery );
    kDebug() << uuid << solidQuery << devices.count();
    if ( !devices.isEmpty() )
        return devices.first().as<Solid::StorageAccess>();
    else
        return 0;
}


bool Nepomuk::mountAndWait( Solid::StorageAccess* storage )
{
    kDebug() << storage;
    QEventLoop loop;
    loop.connect( storage,
                  SIGNAL(accessibilityChanged(bool, QString)),
                  SLOT(quit()) );
    // timeout 20 second
    QTimer::singleShot( 20000, &loop, SLOT(quit()) );

    storage->setup();
    loop.exec();

    kDebug() << storage << storage->isAccessible();

    return storage->isAccessible();
}


KUrl Nepomuk::determineFilesystemPath( const Nepomuk::Resource& fsRes )
{
    QString uuidQuery = QString::fromLatin1( "select ?uuid where { %1 %2 ?uuid . }" )
                        .arg( Soprano::Node::resourceToN3( fsRes.resourceUri() ),
                              Soprano::Node::resourceToN3( Soprano::Vocabulary::NAO::identifier() ) );
    Soprano::QueryResultIterator it = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( uuidQuery, Soprano::Query::QueryLanguageSparql );
    if ( it.next() ) {
        Solid::StorageAccess* storage = storageFromUUID( it["uuid"].toString() );
        it.close();
        if ( storage &&
             ( storage->isAccessible() ||
               mountAndWait( storage ) ) ) {
            return storage->filePath();
        }
    }
    return KUrl();
}


QString Nepomuk::getFileSystemLabelForRemovableMediaFileUrl( const Nepomuk::Resource& res )
{
    QList<Soprano::Node> labelNodes
        = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( QString::fromLatin1( "select ?label where { "
                                                                                                "%1 nie:isPartOf ?fs . "
                                                                                                "?fs a nfo:Filesystem . "
                                                                                                "?fs nao:prefLabel ?label . "
                                                                                                "} LIMIT 1" )
                                                                           .arg( Soprano::Node::resourceToN3( res.resourceUri() ) ),
                                                                           Soprano::Query::QueryLanguageSparql ).iterateBindings( "label" ).allNodes();

    if ( !labelNodes.isEmpty() )
        return labelNodes.first().toString();
    else
        return res.property( Nepomuk::Vocabulary::NIE::url() ).toUrl().host(); // Solid UUID
}


KUrl Nepomuk::convertRemovableMediaFileUrl( const KUrl& url, bool evenMountIfNecessary )
{
    Solid::StorageAccess* storage = Nepomuk::storageFromUUID( url.host() );
    kDebug() << url << storage;
    if ( storage &&
         ( storage->isAccessible() ||
           ( evenMountIfNecessary && Nepomuk::mountAndWait( storage ) ) ) ) {
        kDebug() << "converted:" << KUrl( storage->filePath() + QLatin1String( "/" ) + url.path() );
        return storage->filePath() + QLatin1String( "/" ) + url.path();
    }
    else {
        return KUrl();
    }
}


KIO::UDSEntry Nepomuk::statNepomukResource( const Nepomuk::Resource& res )
{
    //
    // We do not have a local file
    // This is where the magic starts to happen.
    // This is where we only use Nepomuk properties
    //
    KIO::UDSEntry uds;

    // we handle files on removable media which are not mounted
    // as a special case
    bool isFileOnRemovableMedium = isRemovableMediaFile( res );

    // The display name can be anything
    QString displayName;
    if ( isFileOnRemovableMedium ) {
        displayName = i18nc( "%1 is a filename of a file on a removable device, "
                             "%2 is the name of the removable medium which often is something like "
                             "'X GiB Removable Media.",
                             "%1 (on unmounted medium <resource>%2</resource>)",
                             res.genericLabel(),
                             getFileSystemLabelForRemovableMediaFileUrl( res ) );
    }
    else {
        displayName = res.genericLabel();
    }
    uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, displayName );

    // UDS_NAME needs to be unique but can be ugly
    uds.insert( KIO::UDSEntry::UDS_NAME, resourceUriToUdsName( res.resourceUri() ) );

    //
    // There can still be file resources that have a mimetype but are
    // stored remotely, thus they do not have a local nie:url
    //
    // Sadly Strigi's mimetype is not very useful (yet)
    /* QStringList mimeTypes = res.property( Vocabulary::NIE::mimeType() ).toStringList();
    if ( !mimeTypes.isEmpty() ) {
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, mimeTypes.first() );
    }
    else */
    if ( isFileOnRemovableMedium ) {
        KMimeType::Ptr mt = KMimeType::findByUrl( res.property( Vocabulary::NIE::url() ).toUrl(),
                                                  0,
                                                  false, /* no local file as it is not accessible at the moment */
                                                  true   /* fast mode */ );
        if ( mt ) {
            uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, mt->name() );
        }
    }
    else {
        // Use nice display types like "Person", "Project" and so on
        Nepomuk::Types::Class type( res.resourceType() );
        if (!type.label().isEmpty())
            uds.insert( KIO::UDSEntry::UDS_DISPLAY_TYPE, type.label() );

        QString icon = res.genericIcon();
        if ( !icon.isEmpty() ) {
            uds.insert( KIO::UDSEntry::UDS_ICON_NAME, icon );
        }
        else {
            // a fallback icon for nepomuk resources
            uds.insert( KIO::UDSEntry::UDS_ICON_NAME, QLatin1String( "nepomuk" ) );
        }

        if ( uds.stringValue( KIO::UDSEntry::UDS_ICON_NAME ) != QLatin1String( "nepomuk" ) )
            uds.insert( KIO::UDSEntry::UDS_ICON_OVERLAY_NAMES, QLatin1String( "nepomuk" ) );
    }

    //
    // Add some random values
    //
    uds.insert( KIO::UDSEntry::UDS_ACCESS, 0700 );
    uds.insert( KIO::UDSEntry::UDS_USER, KUser().loginName() );
    if ( res.hasProperty( Vocabulary::NIE::lastModified() ) ) {
        // remotely stored files
        uds.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, res.property( Vocabulary::NIE::lastModified() ).toDateTime().toTime_t() );
    }
    else {
        // all nepomuk resources
        uds.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, res.property( Soprano::Vocabulary::NAO::lastModified() ).toDateTime().toTime_t() );
        uds.insert( KIO::UDSEntry::UDS_CREATION_TIME, res.property( Soprano::Vocabulary::NAO::created() ).toDateTime().toTime_t() );
    }

    if ( res.hasProperty( Vocabulary::NIE::contentSize() ) ) {
        // remotely stored files
        uds.insert( KIO::UDSEntry::UDS_SIZE, res.property( Vocabulary::NIE::contentSize() ).toInt() );
    }


    //
    // Starting with KDE 4.4 we have the pretty UDS_NEPOMUK_URI which makes
    // everything much cleaner since kio slaves can decide if the resources can be
    // annotated or not.
    //
    uds.insert( KIO::UDSEntry::UDS_NEPOMUK_URI, KUrl( res.resourceUri() ).url() );

    KUrl reUrl = Nepomuk::redirectionUrl( res );
    if ( !reUrl.isEmpty() ) {
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QLatin1String( "inode/directory" ) );
        uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );

        // FIXME: not sure this is necessary if we already do the redirect in listDir()
        uds.insert( KIO::UDSEntry::UDS_URL, reUrl.url() );
    }

    return uds;
}


bool Nepomuk::willBeRedirected( const Nepomuk::Resource& res )
{
    return( res.hasType( Nepomuk::Vocabulary::NFO::Folder() ) ||
            res.hasType( Soprano::Vocabulary::NAO::Tag() ) ||
            res.hasType( Nepomuk::Vocabulary::NFO::Filesystem() ) );
}


KUrl Nepomuk::redirectionUrl( const Nepomuk::Resource& res )
{
    if ( res.hasType( Nepomuk::Vocabulary::NFO::Folder() ) ) {
        return res.property( Nepomuk::Vocabulary::NIE::url() ).toUrl();
    }
    else if ( res.hasType( Nepomuk::Vocabulary::NFO::Filesystem() ) ) {
        KUrl fsUrl = determineFilesystemPath( res );
        if ( fsUrl.isValid() ) {
            return fsUrl;
        }
    }
    else if ( res.hasType( Soprano::Vocabulary::NAO::Tag() ) ) {
        Query::ComparisonTerm term( Soprano::Vocabulary::NAO::hasTag(), Query::ResourceTerm( res ), Query::ComparisonTerm::Equal );
        KUrl queryUrl( Query::Query( term ).toSearchUrl() );
        queryUrl.addQueryItem( QLatin1String( "title" ), i18n( "Things tagged '%1'", res.genericLabel() ) );
        return queryUrl.url();
    }

#if 0 // disabled as long as the strigi service does create a dedicated album resource for each track
    else if ( res.hasType( Nepomuk::Vocabulary::NMM::MusicAlbum() ) ) {
        Query::ComparisonTerm term( Nepomuk::Vocabulary::NMM::musicAlbum(), Query::ResourceTerm( res ) );
        KUrl queryUrl( Query::Query( term ).toSearchUrl() );
        queryUrl.addQueryItem( QLatin1String( "title" ), res.genericLabel() );
        return queryUrl.url();
    }
#endif

    return KUrl();
}


namespace {
    /**
     * Check if the resource represents a local file with an existing nie:url property.
     */
    bool isLocalFile( const Nepomuk::Resource& res )
    {
        if ( res.hasProperty( Nepomuk::Vocabulary::NIE::url() ) ) {
            KUrl url = res.property( Nepomuk::Vocabulary::NIE::url() ).toUrl();
            return ( !url.isEmpty() &&
                     QFile::exists( url.toLocalFile() ) );
        }
        else {
            return false;
        }
    }
}

KUrl Nepomuk::nepomukToFileUrl( const KUrl& url, bool evenMountIfNecessary )
{
    QString filename;
    Nepomuk::Resource res = splitNepomukUrl( url, &filename );

    if ( !res.exists() )
        return KUrl();

    KUrl newURL;

    //
    // let's see if it is a pimo thing which refers to a file
    //
    if ( res.hasType( Nepomuk::Vocabulary::PIMO::Thing() ) ) {
        if ( !res.pimoThing().groundingOccurrences().isEmpty() ) {
            res = res.pimoThing().groundingOccurrences().first();
        }
    }

    if ( isLocalFile( res ) ) {
        newURL = res.property( Vocabulary::NIE::url() ).toUrl();
    }
    else if ( isRemovableMediaFile( res ) ) {
        const KUrl removableMediaUrl = res.property( Nepomuk::Vocabulary::NIE::url() ).toUrl();
        newURL = convertRemovableMediaFileUrl( removableMediaUrl, evenMountIfNecessary );
    }

    if ( newURL.isValid() && !filename.isEmpty() ) {
        newURL.addPath( filename );
    }

    kDebug() << url << newURL;

    return newURL;
}
