/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2008-2009 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "kio_nepomuk.h"
#include "nie.h"
#include "nfo.h"
#include "pimo.h"
#include "resourcepagegenerator.h"

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
     * Determine the label for a filesystem \p url is stored on.
     *
     * \param url A filex:/ URL pointing to a file stored on a removable filesystem.
     */
    QString getFileSystemLabelForRemovableMediaFileUrl( const KUrl& url )
    {
        QList<Soprano::Node> labelNodes
            = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( QString::fromLatin1( "select ?label where { "
                                                                                                    "?r nie:url %1 . "
                                                                                                    "?r nie:isPartOf ?fs . "
                                                                                                    "?fs a nfo:Filesystem . "
                                                                                                    "?fs nao:prefLabel ?label . "
                                                                                                    "} LIMIT 1" )
                                                                               .arg( Soprano::Node::resourceToN3( url ) ),
                                                                               Soprano::Query::QueryLanguageSparql ).iterateBindings( "label" ).allNodes();

        if ( !labelNodes.isEmpty() )
            return labelNodes.first().toString();
        else
            return url.host(); // Solid UUID
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
        QString urlStr = url.url();
        int pos = urlStr.indexOf( '/', urlStr.startsWith( QLatin1String( "nepomuk:/res/" ) ) ? 13 : 9 );
        if ( pos > 0 ) {
            KUrl resourceUri = urlStr.left(pos);
            filename = urlStr.mid( pos+1 );
            return resourceUri;
        }
        else {
            return url;
        }
    }

    /**
     * Encode the resource URI into the UDS_NAME to make it unique.
     * It is important that we do not use the % for percent-encoding. Otherwise KUrl::url will
     * re-encode them, thus, destroying our name.
     */
    QString resourceUriToUdsName( const KUrl& url )
    {
        return QString::fromAscii( url.toEncoded().toPercentEncoding( QByteArray(), QByteArray(), '_' ) );
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
    // Some clients (like Gwenview) will directly try to list the parent directory which in our case
    // is nepomuk:/res. Giving an ugly "does not exist" error to the user is no good. Thus, we simply
    // list it as an empty dir
    //
    if ( url == KUrl( QLatin1String( "nepomuk:/res" ) ) ) {
        listEntry( KIO::UDSEntry(), true );
        finished();
    }
    else {
        return ForwardingSlaveBase::listDir( url );
    }
}


void Nepomuk::NepomukProtocol::get( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    kDebug() << url;

    QString filename;
    Nepomuk::Resource res = splitNepomukUrl( url, filename );

    if( !res.exists() ) {
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
    }
    else if ( !filename.isEmpty() ) {
        ForwardingSlaveBase::get( url );
    }
    else if ( res.hasType( Soprano::Vocabulary::NAO::Tag() ) ) {
        error( KIO::ERR_IS_DIRECTORY, url.prettyUrl() );
    }
    else if ( isLocalFile( res ) ) {
        ForwardingSlaveBase::get( url );
    }
    else if ( isRemovableMediaFile( url ) ) {
        KUrl removableMediaUrl = res.property( Nepomuk::Vocabulary::NIE::url() ).toUrl();
        if ( convertRemovableMediaFileUrl( removableMediaUrl, true ).isValid() ) {
            ForwardingSlaveBase::get( url );
        }
        else {
            error( KIO::ERR_SLAVE_DEFINED,
                   i18nc( "@info", "Please insert the removable medium <resource>%1</resource> to access this file.",
                          getFileSystemLabelForRemovableMediaFileUrl( removableMediaUrl ) ) );
        }
    }
    else {
        // TODO: call the share service for remote files (KDE 4.5)

        mimeType( "text/html" );

        ResourcePageGenerator gen( res );

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
            data( gen.generatePage() );
        }
        finished();
    }
}


void Nepomuk::NepomukProtocol::stat( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    kDebug() << url;

    QString filename;
    Nepomuk::Resource res = splitNepomukUrl( url, filename );

    //
    // let's see if it is a pimo thing which refers to a file
    //
    if ( res.hasType( Nepomuk::Vocabulary::PIMO::Thing() ) ) {
        if ( !res.pimoThing().groundingOccurrences().isEmpty() ) {
            res = res.pimoThing().groundingOccurrences().first();
        }
    }

    if( !res.exists() ) {
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
    }
    else if ( !filename.isEmpty() ) {
        ForwardingSlaveBase::get( url );
    }
    else if ( isLocalFile( res ) ) {
        ForwardingSlaveBase::stat( url );
    }
    else if ( isRemovableMediaFile( res ) &&
              convertRemovableMediaFileUrl( res.property( Nepomuk::Vocabulary::NIE::url() ).toUrl() ).isValid() ) {
        ForwardingSlaveBase::stat( url );
    }
    else {
        //
        // We do not have a local file
        // This is where the magic starts to happen.
        // This is where we only use Nepomuk properties
        //
        KIO::UDSEntry uds;

        // The display name can be anything
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, res.genericLabel() );

        // UDS_NAME needs to be unique but can be ugly
        uds.insert( KIO::UDSEntry::UDS_NAME, resourceUriToUdsName( url ) );

        //
        // There can still be file resources that have a mimetype but are
        // stored remotely, thus they do not have a local nie:url
        //
        QString mimeType = res.property( Vocabulary::NIE::mimeType() ).toString();
        if ( !mimeType.isEmpty() ) {
            uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, mimeType );
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


        // special case: tags and filesystems are handled as folders
        if ( res.hasType( Soprano::Vocabulary::NAO::Tag() ) ||
             res.hasType( Nepomuk::Vocabulary::NFO::Filesystem() ) ) {
            kDebug() << res.resourceUri() << "is tag or filesystem -> mimetype inode/directory";
            uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QLatin1String( "inode/directory" ) );
            uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        }

        statEntry( uds );
        finished();
    }
}

void Nepomuk::NepomukProtocol::mimetype( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    kDebug() << url;

    QString filename;
    Nepomuk::Resource res = splitNepomukUrl( url, filename );
    if ( !filename.isEmpty() ) {
        ForwardingSlaveBase::mimetype( url );
    }
    if ( res.hasType( Soprano::Vocabulary::NAO::Tag() ) ) {
        kDebug() << res.resourceUri() << "is tag -> mimetype inode/directory";
        // in listDir() we list tags as search folders
        mimeType( QLatin1String( "inode/directory" ) );
        finished();
    }
    else if ( isLocalFile( res ) ) {
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


void Nepomuk::NepomukProtocol::del(const KUrl& url, bool isFile)
{
    if ( !ensureNepomukRunning() )
        return;

    QString filename;
    Nepomuk::Resource res = splitNepomukUrl( url, filename );
    if ( !filename.isEmpty() ||
         isLocalFile( res ) ) {
        ForwardingSlaveBase::del( url, isFile );
    }
    else {
        res.remove();
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

    if ( res.hasType( Soprano::Vocabulary::NAO::Tag() ) ) {
        Query::ComparisonTerm term( Soprano::Vocabulary::NAO::hasTag(), Query::ResourceTerm( res ), Query::ComparisonTerm::Equal );
        newURL = Query::Query( term ).toSearchUrl().url();
    }
    else if ( res.hasType( Nepomuk::Vocabulary::NFO::Filesystem() ) ) {
        newURL = determineFilesystemPath( res );
        kDebug() << "Rewriting filesystem URL" << url << "to" << newURL;
    }
    else if ( isLocalFile( res ) ) {
        newURL = res.property( Vocabulary::NIE::url() ).toUrl();
    }
    else if ( isRemovableMediaFile( res ) ) {
        newURL = convertRemovableMediaFileUrl( res.property( Nepomuk::Vocabulary::NIE::url() ).toUrl() );
        kDebug() << "Rewriting removable media URL" << url << "to" << newURL;
    }

    if ( !filename.isEmpty() ) {
        newURL.addPath( filename );
    }

    kDebug() << url << newURL;

    return newURL.isValid();
}


void Nepomuk::NepomukProtocol::prepareUDSEntry( KIO::UDSEntry& uds, bool listing ) const
{
    //
    // We cannot handle listing subdirs, thus, nepomuksearch does create UDS_URL
    // values for us. But since file:/ does not create UDS_MIME_TYPE entries and KFileItem
    // can only handle local files (our URLs are not local) we need to handle the mimetyping
    // here the same way ForwardingSlaveBase does. For !listing we have all the correct values
    // anyway.
    //
    if ( listing ) {
        if ( uds.isDir() ) {
            //
            // nepomuksearch does already create correct UDS_URL values for us. But if we are
            // listing a nfo:Folder resource we do not have the UDS_URL since file:/ does not
            // set it. But since we cannot handle subdirs in there we need to set UDS_URL to
            // the original file URL here.
            //
            KUrl url = processedUrl();
            url.addPath( uds.stringValue( KIO::UDSEntry::UDS_NAME ) );
            uds.insert( KIO::UDSEntry::UDS_URL, url.url() );
        }
        else {
            ForwardingSlaveBase::prepareUDSEntry( uds, listing );
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
