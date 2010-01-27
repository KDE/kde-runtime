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

#include "kio_nepomuk.h"
#include "nie.h"
#include "nfo.h"
#include "pimo.h"
#include "resourcepagegenerator.h"
#include "nepomuksearchurltools.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>

#include <KComponentData>
#include <KDebug>
#include <KUser>
#include <kdeversion.h>
#include <KMessageBox>
#include <KMimeType>

#include <nepomuk/thing.h>
#include <nepomuk/resourcemanager.h>
#include <nepomuk/variant.h>
#include <nepomuk/class.h>
#include <nepomuk/property.h>
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

    /**
     * Check if the resource represents a file on a removable media using a filex:/
     * URL.
     */
    bool isRemovableMediaFile( const Nepomuk::Resource& res )
    {
        if ( res.hasProperty( Nepomuk::Vocabulary::NIE::url() ) ) {
            KUrl url = res.property( Nepomuk::Vocabulary::NIE::url() ).toUrl();
            return ( url.protocol() == QLatin1String( "filex" ) );
        }
        else {
            return false;
        }
    }

    /**
     * Mount a storage volume via Solid and wait for it to be mounted with
     * a timeout of 20 seconds.
     */
    bool mountAndWait( Solid::StorageAccess* storage )
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

    /**
     * Create a Solid storage access interface from the volume UUID.
     */
    Solid::StorageAccess* storageFromUUID( const QString& uuid )
    {
        QString solidQuery = QString::fromLatin1( "[ StorageVolume.usage=='FileSystem' AND StorageVolume.uuid=='%1' ]" ).arg( uuid.toLower() );
        QList<Solid::Device> devices = Solid::Device::listFromQuery( solidQuery );
        kDebug() << uuid << solidQuery << devices.count();
        if ( !devices.isEmpty() )
            return devices.first().as<Solid::StorageAccess>();
        else
            return 0;
    }

    /**
     * Convert a filex:/ URL into its actual local file URL.
     *
     * \param url The filex:/ URL to convert
     * \param evenMountIfNecessary If true an existing unmouted volume will be mounted to grant access to the local file.
     *
     * \return The converted local URL or an invalid URL if the filesystem the file is stored on could not be/was not mounted.
     */
    KUrl convertRemovableMediaFileUrl( const KUrl& url, bool evenMountIfNecessary = false )
    {
        Solid::StorageAccess* storage = storageFromUUID( url.host() );
        kDebug() << url << storage;
        if ( storage &&
             ( storage->isAccessible() ||
               ( evenMountIfNecessary && mountAndWait( storage ) ) ) ) {
            kDebug() << "converted:" << KUrl( storage->filePath() + QLatin1String( "/" ) + url.path() );
            return storage->filePath() + QLatin1String( "/" ) + url.path();
        }
        else {
            return KUrl();
        }
    }

    /**
     * Get the mount path of a nfo:Filesystem resource as created by the removable storage service.
     */
    KUrl determineFilesystemPath( const Nepomuk::Resource& fsRes )
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

    /**
     * Determine the label for a filesystem \p res is stored on.
     */
    QString getFileSystemLabelForRemovableMediaFileUrl( const Nepomuk::Resource& res )
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

    KUrl stripQuery( const KUrl& url )
    {
        KUrl newUrl( url );
        newUrl.setEncodedQuery( QByteArray() );
        return newUrl;
    }

    /**
     * Split the filename part off a nepomuk:/ URI. This is used in many methods for identifying
     * entries listed from tags and filesystems.
     */
    Nepomuk::Resource splitNepomukUrl( const KUrl& url, QString& filename )
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
            filename = urlStr.mid( pos+1 );
            return resourceUri;
        }
        else {
            return stripQuery( url );
        }
    }
}


Nepomuk::NepomukProtocol::NepomukProtocol( const QByteArray& poolSocket, const QByteArray& appSocket )
    : KIO::ForwardingSlaveBase( "nepomuk", poolSocket, appSocket )
{
    ResourceManager::instance()->init();
}


Nepomuk::NepomukProtocol::~NepomukProtocol()
{
}


void Nepomuk::NepomukProtocol::listDir( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    //
    // We have two special cases: tags and filesystems
    // which we redirect. Apart from that we do not list
    // anything.
    // See README for details
    //
    QString filename;
    Nepomuk::Resource res = splitNepomukUrl( url, filename );

    if ( res.hasType( Soprano::Vocabulary::NAO::Tag() ) ) {
        Query::ComparisonTerm term( Soprano::Vocabulary::NAO::hasTag(), Query::ResourceTerm( res ), Query::ComparisonTerm::Equal );
        redirection( Query::Query( term ).toSearchUrl().url() );
        finished();
    }
    else if ( res.hasType( Nepomuk::Vocabulary::NFO::Filesystem() ) ) {
        KUrl fsUrl = determineFilesystemPath( res );
        if ( fsUrl.isValid() ) {
            redirection( fsUrl );
            finished();
        }
        else {
            error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
        }
    }
    else {
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
    }
}


void Nepomuk::NepomukProtocol::get( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    kDebug() << url;

    //
    // First we check if it is a generic resource which cannot be forwarded
    // If we could rewrite the URL than we can continue as Get operation
    // which will also try to mount removable devices.
    // If not we generate a HTML page.
    //
    m_currentOperation = GetPrepare;
    KUrl newUrl;
    if ( rewriteUrl( url, newUrl ) ) {
        m_currentOperation = Get;
        ForwardingSlaveBase::get( url );
    }
    else {
        // TODO: call the share service for remote files (KDE 4.5)

        mimeType( "text/html" );

        KUrl newUrl = stripQuery( url );
        Nepomuk::Resource res( newUrl );
        if ( !res.exists() ) {
            error( KIO::ERR_DOES_NOT_EXIST, newUrl.prettyUrl() );
            return;
        }

        if ( url.hasQueryItem( QLatin1String( "action") ) &&
             url.queryItem( QLatin1String( "action" ) ) == QLatin1String( "delete" ) &&
             messageBox( i18n( "Do you really want to delete the resource and all relations "
                               "to and from it?" ),
                         KIO::SlaveBase::QuestionYesNo,
                         i18n( "Delete Resource" ) ) == KMessageBox::Yes ) {
            res.remove();
            data( "<html><body><p>Resource has been deleted from the Nepomuk storage.</p></body></html>" );
        }
        else {
            ResourcePageGenerator gen( res );
            data( gen.generatePage() );
        }
        finished();
    }
}


void Nepomuk::NepomukProtocol::put( const KUrl& url, int permissions, KIO::JobFlags flags )
{
    if ( !ensureNepomukRunning() )
        return;

    kDebug() << url;

    m_currentOperation = Other;
    ForwardingSlaveBase::put( url, permissions, flags );
}


void Nepomuk::NepomukProtocol::stat( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    kDebug() << url;

    m_currentOperation = Stat;
    KUrl newUrl;
    if ( rewriteUrl( url, newUrl ) ) {
        ForwardingSlaveBase::stat( url );
    }
    else {
        newUrl = stripQuery( url );
        Nepomuk::Resource res( newUrl );

        if ( !res.exists() ) {
            error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
        }
        else {
            KIO::UDSEntry uds = statNepomukResource( res );

            // special case: tags and filesystems are handled as folders
            if ( res.hasType( Soprano::Vocabulary::NAO::Tag() ) ||
                 res.hasType( Nepomuk::Vocabulary::NFO::Filesystem() ) ) {
                kDebug() << res.resourceUri() << "is tag or filesystem -> mimetype inode/directory";
                uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QLatin1String( "inode/directory" ) );
                uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
            }

            // special case: tags cannot be redirected in stat, thus we need to use UDS_URL here
            if ( res.hasType( Soprano::Vocabulary::NAO::Tag() ) ) {
                Query::ComparisonTerm term( Soprano::Vocabulary::NAO::hasTag(), Query::ResourceTerm( res ), Query::ComparisonTerm::Equal );
                uds.insert( KIO::UDSEntry::UDS_URL, Query::Query( term ).toSearchUrl().url() );
            }

            statEntry( uds );
            finished();
        }
    }
}


KIO::UDSEntry Nepomuk::NepomukProtocol::statNepomukResource( const Nepomuk::Resource& res )
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
            uds.insert( KIO::UDSEntry::UDS_ICON_NAME, "nepomuk" );
        }
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

    return uds;
}


void Nepomuk::NepomukProtocol::mimetype( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    kDebug() << url;

    m_currentOperation = Other;

    QString filename;
    Nepomuk::Resource res = splitNepomukUrl( url, filename );
    if ( filename.isEmpty() &&
         ( res.hasType( Soprano::Vocabulary::NAO::Tag() ) ||
           res.hasType( Nepomuk::Vocabulary::NFO::Filesystem() ) ) ) {
        kDebug() << res.resourceUri() << "is tag or file system -> mimetype inode/directory";
        // in listDir() we list tags as search folders
        mimeType( QLatin1String( "inode/directory" ) );
        finished();
    }
    else {
        KUrl newUrl;
        if ( rewriteUrl( url, newUrl ) ) {
            ForwardingSlaveBase::mimetype( url );
        }
        else {
            //
            // There can still be file resources that have a mimetype but are
            // stored remotely, thus they do not have a local nie:url
            //
            QString m = res.property( Vocabulary::NIE::mimeType() ).toString();
            if ( !m.isEmpty() ) {
                mimeType( m );
                finished();
            }
            else {
                // everything else we list as html file
                mimeType( "text/html" );
                finished();
            }
        }
    }
}


void Nepomuk::NepomukProtocol::del(const KUrl& url, bool isFile)
{
    if ( !ensureNepomukRunning() )
        return;

    m_currentOperation = Other;

    KUrl newUrl;
    if ( rewriteUrl( url, newUrl ) ) {
        ForwardingSlaveBase::del( url, isFile );
    }
    else {
        Nepomuk::Resource res( url );
        if ( !res.exists() ) {
            error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
        }
        else {
            res.remove();
            finished();
        }
    }
}


bool Nepomuk::NepomukProtocol::rewriteUrl( const KUrl& url, KUrl& newURL )
{
    QString filename;
    Nepomuk::Resource res = splitNepomukUrl( url, filename );

    if ( !res.exists() )
        return false;

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
        newURL = convertRemovableMediaFileUrl( removableMediaUrl, m_currentOperation == Get || m_currentOperation == GetPrepare );
        if ( !newURL.isValid() && m_currentOperation == Get ) {
            error( KIO::ERR_SLAVE_DEFINED,
                   i18nc( "@info", "Please insert the removable medium <resource>%1</resource> to access this file.",
                          getFileSystemLabelForRemovableMediaFileUrl( res ) ) );
            return true;
        }
        else if ( m_currentOperation == GetPrepare ) {
            return true;
        }
        kDebug() << "Rewriting removable media URL" << url << "to" << newURL;
    }

    if ( newURL.isValid() && !filename.isEmpty() ) {
        newURL.addPath( filename );
    }

    kDebug() << url << newURL;

    return newURL.isValid();
}


void Nepomuk::NepomukProtocol::prepareUDSEntry( KIO::UDSEntry& uds,
                                                bool listing ) const
{
    // this will set mimetype and UDS_LOCAL_PATH for local files
    ForwardingSlaveBase::prepareUDSEntry( uds, listing );

    // make sure we have unique names for everything
    uds.insert( KIO::UDSEntry::UDS_NAME, resourceUriToUdsName( requestedUrl() ) );

    // make sure we do not use these ugly names for display
    if ( !uds.contains( KIO::UDSEntry::UDS_DISPLAY_NAME ) ) {
        Nepomuk::Resource res( requestedUrl() );
        if ( res.hasType( Nepomuk::Vocabulary::PIMO::Thing() ) ) {
            if ( !res.pimoThing().groundingOccurrences().isEmpty() ) {
                res = res.pimoThing().groundingOccurrences().first();
            }
        }

        if ( res.hasProperty( Nepomuk::Vocabulary::NIE::url() ) ) {
            KUrl fileUrl( res.property( Nepomuk::Vocabulary::NIE::url() ).toUrl() );
            uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, fileUrl.fileName() );
        }
        else {
            uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, res.genericLabel() );
        }
    }
}


bool Nepomuk::NepomukProtocol::ensureNepomukRunning()
{
    if ( Nepomuk::ResourceManager::instance()->init() ) {
        error( KIO::ERR_SLAVE_DEFINED, i18n( "The Nepomuk system is not activated. Unable to answer queries without it." ) );
        return false;
    }
    else {
        return true;
    }
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

        Nepomuk::NepomukProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();

        return 0;
    }
}
