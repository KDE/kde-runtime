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

#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/QueryResultIterator>
#include <Soprano/NodeIterator>
#include <Soprano/Node>
#include <Soprano/Model>

#include <Solid/Device>
#include <Solid/StorageAccess>


namespace {
    bool noFollowSet( const KUrl& url ) {
        return( url.encodedQueryItemValue( "noFollow" ) == "true" );
    }
}

Nepomuk2::NepomukProtocol::NepomukProtocol( const QByteArray& poolSocket, const QByteArray& appSocket )
    : KIO::ForwardingSlaveBase( "nepomuk", poolSocket, appSocket )
{
    ResourceManager::instance()->init();
}


Nepomuk2::NepomukProtocol::~NepomukProtocol()
{
}


void Nepomuk2::NepomukProtocol::listDir( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    //
    // We have two special cases: tags and filesystems
    // which we redirect. Apart from that we do not list
    // anything.
    // See README for details
    //
    Nepomuk2::Resource res = Nepomuk2::splitNepomukUrl( url );
    KUrl reUrl = Nepomuk2::redirectionUrl( res );
    if ( !reUrl.isEmpty() ) {
        redirection( reUrl );
        finished();
    }
    else {
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
    }
}


void Nepomuk2::NepomukProtocol::get( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    kDebug() << url;

    m_currentOperation = Get;
    const bool noFollow = noFollowSet( url );

    Nepomuk2::Resource res = splitNepomukUrl( url );
    if ( !noFollow && Nepomuk2::isRemovableMediaFile( res ) ) {
        error( KIO::ERR_SLAVE_DEFINED,
               i18nc( "@info", "Please insert the removable medium <resource>%1</resource> to access this file.",
                      getFileSystemLabelForRemovableMediaFileUrl( res ) ) );
    }
    else if ( !noFollow && !Nepomuk2::nepomukToFileUrl( url ).isEmpty() ) {
        ForwardingSlaveBase::get( url );
    }
    else {
        // TODO: call the share service for remote files (KDE 4.5)

        mimeType( "text/html" );

        const KUrl newUrl = stripQuery( url );
        if ( !res.exists() ) {
            error( KIO::ERR_DOES_NOT_EXIST, QLatin1String("get: ") + QString::fromLatin1("%1 (%2)").arg(url.url(), url.queryItem(QLatin1String("resource"))) );
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


void Nepomuk2::NepomukProtocol::put( const KUrl& url, int permissions, KIO::JobFlags flags )
{
    if ( !ensureNepomukRunning() )
        return;

    kDebug() << url;

    m_currentOperation = Other;
    ForwardingSlaveBase::put( url, permissions, flags );
}


void Nepomuk2::NepomukProtocol::stat( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    kDebug() << url;

    m_currentOperation = Stat;
    const bool noFollow = noFollowSet( url );
    if ( !noFollow && !Nepomuk2::nepomukToFileUrl( url ).isEmpty() ) {
        ForwardingSlaveBase::stat( url );
    }
    else {
        Nepomuk2::Resource res = splitNepomukUrl( url );

        if ( !res.exists() ) {
            error( KIO::ERR_DOES_NOT_EXIST, QLatin1String("stat: ") + stripQuery(url).prettyUrl() );
        }
        else {
            KIO::UDSEntry uds = Nepomuk2::statNepomukResource( res, noFollow );
            statEntry( uds );
            finished();
        }
    }
}


void Nepomuk2::NepomukProtocol::mimetype( const KUrl& url )
{
    if ( !ensureNepomukRunning() )
        return;

    kDebug() << url;

    m_currentOperation = Other;
    if ( noFollowSet( url ) ) {
        mimeType( "text/html" );
        finished();
        return;
    }

    QString filename;
    Nepomuk2::Resource res = Nepomuk2::splitNepomukUrl( url, &filename );
    if ( filename.isEmpty() &&
         Nepomuk2::willBeRedirected( res ) ) {
        kDebug() << res.uri() << "is tag or file system -> mimetype inode/directory";
        mimeType( QLatin1String( "inode/directory" ) );
        finished();
    }
    else {
        if ( !Nepomuk2::nepomukToFileUrl( url ).isEmpty() ) {
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


void Nepomuk2::NepomukProtocol::del(const KUrl& url, bool isFile)
{
    if ( !ensureNepomukRunning() )
        return;

    m_currentOperation = Other;

    KUrl newUrl;
    if ( rewriteUrl( url, newUrl ) ) {
        ForwardingSlaveBase::del( url, isFile );
    }
    else {
        Nepomuk2::Resource res( url );
        if ( !res.exists() ) {
            error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
        }
        else {
            res.remove();
            finished();
        }
    }
}


bool Nepomuk2::NepomukProtocol::rewriteUrl( const KUrl& url, KUrl& newURL )
{
    if ( noFollowSet( url ) )
        return false;

    newURL = Nepomuk2::nepomukToFileUrl( url, m_currentOperation == Get );
    return newURL.isValid();
}


bool Nepomuk2::NepomukProtocol::ensureNepomukRunning()
{
    if ( Nepomuk2::ResourceManager::instance()->init() ) {
        error( KIO::ERR_SLAVE_DEFINED, i18n( "The desktop search service is not activated. Unable to answer queries without it." ) );
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

        Nepomuk2::NepomukProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();

        return 0;
    }
}
