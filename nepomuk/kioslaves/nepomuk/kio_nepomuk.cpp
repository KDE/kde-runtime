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
//#include "nmm.h"
#include "pimo.h"
#include "resourcepagegenerator.h"
#include "nepomuksearchurltools.h"
#include "resourcestat.h"

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
    Nepomuk::Resource res = Nepomuk::splitNepomukUrl( url );
    KUrl reUrl = Nepomuk::redirectionUrl( res );
    if ( !reUrl.isEmpty() ) {
        redirection( reUrl );
        finished();
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

    m_currentOperation = Get;

    Nepomuk::Resource res = splitNepomukUrl( url );
    if ( Nepomuk::isRemovableMediaFile( res ) ) {
        error( KIO::ERR_SLAVE_DEFINED,
               i18nc( "@info", "Please insert the removable medium <resource>%1</resource> to access this file.",
                      getFileSystemLabelForRemovableMediaFileUrl( res ) ) );
    }
    else if ( !Nepomuk::nepomukToFileUrl( url ).isEmpty() ) {
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
            gen.setFlagsFromUrl( url );
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
    if ( !Nepomuk::nepomukToFileUrl( url ).isEmpty() ) {
        ForwardingSlaveBase::stat( url );
    }
    else {
        KUrl strippedUrl = stripQuery( url );
        Nepomuk::Resource res( strippedUrl );

        if ( !res.exists() ) {
            error( KIO::ERR_DOES_NOT_EXIST, strippedUrl.prettyUrl() );
        }
        else {
            KIO::UDSEntry uds = Nepomuk::statNepomukResource( res );
            statEntry( uds );
            finished();
        }
    }
}


void Nepomuk::NepomukProtocol::mimetype( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    kDebug() << url;

    m_currentOperation = Other;

    QString filename;
    Nepomuk::Resource res = Nepomuk::splitNepomukUrl( url, &filename );
    if ( filename.isEmpty() &&
         Nepomuk::willBeRedirected( res ) ) {
        kDebug() << res.resourceUri() << "is tag or file system -> mimetype inode/directory";
        mimeType( QLatin1String( "inode/directory" ) );
        finished();
    }
    else {
        if ( !Nepomuk::nepomukToFileUrl( url ).isEmpty() ) {
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
    if ( url.queryItemValue( QLatin1String( "noFollow" ) ) == QLatin1String( "true" ) )
        return false;

    newURL = Nepomuk::nepomukToFileUrl( url, m_currentOperation == Get );
    return newURL.isValid();
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
