/*
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

#include "kio_tags.h"

#include <Nepomuk2/ResourceManager>
#include <Nepomuk2/Vocabulary/NFO>
#include <Nepomuk2/Vocabulary/NIE>
#include <Nepomuk2/Vocabulary/NUAO>

#include <KUrl>
#include <kio/global.h>
#include <klocale.h>
#include <kio/job.h>
#include <KUser>
#include <KDebug>
#include <KLocale>
#include <kio/netaccess.h>
#include <KComponentData>

#include <Soprano/Vocabulary/NAO>
#include <Soprano/QueryResultIterator>
#include <Soprano/Model>
#include <Soprano/Node>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

#include <sys/types.h>
#include <unistd.h>


Nepomuk2::TagsProtocol::TagsProtocol(const QByteArray& pool_socket, const QByteArray& app_socket)
: KIO::SlaveBase("nepomuktags", pool_socket, app_socket)
{
}

Nepomuk2::TagsProtocol::~TagsProtocol()
{
}

QString Nepomuk2::TagsProtocol::fetchIdentifer(const QUrl& uri)
{
    QHash< QUrl, QString >::const_iterator it = m_tagUriIdentHash.constFind( uri );
    if( it != m_tagUriIdentHash.constEnd() ) {
        return it.value();
    }
    else {
        QString query = QString::fromLatin1("select ?i where { %1 nao:identifier ?i . }")
                        .arg( Soprano::Node::resourceToN3( uri ) );

        Soprano::Model* model = ResourceManager::instance()->mainModel();
        Soprano::QueryResultIterator it = model->executeQuery( query, Soprano::Query::QueryLanguageSparqlNoInference );
        if( it.next() ) {
            QString ident = it[0].literal().toString();

            m_tagIdentUriHash.insert( ident, uri );
            m_tagUriIdentHash.insert( uri, ident );

            return ident;
        }

        return QString();
    }
}

QUrl Nepomuk2::TagsProtocol::fetchUri(const QString& label)
{
    QHash< QString, QUrl >::const_iterator it = m_tagIdentUriHash.constFind( label );
    if( it != m_tagIdentUriHash.constEnd() ) {
        return it.value();
    }
    else {
        QString query = QString::fromLatin1("select ?r where { ?r a nao:Tag; nao:identifier %1 . }")
                        .arg( Soprano::Node::literalToN3( Soprano::LiteralValue(label) ) );

        Soprano::Model* model = ResourceManager::instance()->mainModel();
        Soprano::QueryResultIterator it = model->executeQuery( query, Soprano::Query::QueryLanguageSparqlNoInference );
        if( it.next() ) {
            QUrl uri = it[0].uri();

            m_tagIdentUriHash.insert( label, uri );
            m_tagUriIdentHash.insert( uri, label );

            return uri;
        }

        return QUrl();
    }
}



namespace {
    //FIXME: Find a nice icon for tags?
    //FIXME: Fetch the datetime?
    KIO::UDSEntry createUDSEntryForTag(const QUrl& tagUri, const QString& label) {
        QDateTime dt;

        KIO::UDSEntry uds;
        uds.insert( KIO::UDSEntry::UDS_NAME, label );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, label );
        uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1( "inode/directory" ) );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_TYPE, i18n( "Tag" ) );
        uds.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, dt.toTime_t() );
        uds.insert( KIO::UDSEntry::UDS_CREATION_TIME, dt.toTime_t() );
        uds.insert( KIO::UDSEntry::UDS_ACCESS, 0700 );
        uds.insert( KIO::UDSEntry::UDS_USER, KUser().loginName() );
        uds.insert( KIO::UDSEntry::UDS_NEPOMUK_URI, tagUri.toString() );
        uds.insert( KIO::UDSEntry::UDS_ICON_NAME, QLatin1String("nepomuk") );

        return uds;
    }
}

void Nepomuk2::TagsProtocol::listDir(const KUrl& url)
{
    Soprano::Model* model = ResourceManager::instance()->mainModel();

    QString path = url.path( KUrl::RemoveTrailingSlash );
    if( path.isEmpty() || path == QLatin1String("/") ) {

        QLatin1String query("select distinct ?r ?ident where { ?r a nao:Tag ; nao:identifier ?ident .}");
        Soprano::QueryResultIterator it = model->executeQuery( query, Soprano::Query::QueryLanguageSparqlNoInference );
        while( it.next() ) {
            QString tagLabel = it["ident"].literal().toString();
            QUrl tagUri = it["r"].uri();

            m_tagUriIdentHash.insert( tagUri, tagLabel );
            m_tagIdentUriHash.insert( tagLabel, tagUri );

            listEntry( createUDSEntryForTag( tagUri, tagLabel ), false );
        }

        listEntry( KIO::UDSEntry(), true );
        finished();
        return;
    }

    if( path.startsWith(QChar::fromLatin1('/')) )
        path = path.mid( 1 );

    // Fetch tag uris
    QSet<QUrl> tagUris;
    QStringList tagN3;

    QStringList tags = path.split( QChar::fromLatin1('/') );
    foreach(const QString& tag, tags) {
        QUrl uri = fetchUri(tag);
        if( uri.isEmpty() ) {
            QString text = QString::fromLatin1("Tag %1 does not exist").arg(tag);
            // messageBox( text, Information );
            error( KIO::ERR_CANNOT_ENTER_DIRECTORY, text );
            return;
        }

        tagUris.insert( uri );
        tagN3.append( Soprano::Node::resourceToN3(uri) );
    }

    // Fetch all the files
    QString query = QString::fromLatin1("select ?r ?url where { ?r a nfo:FileDataObject; nie:url ?url; nao:hasTag %1 .}")
                    .arg( tagN3.join(",") );

    Soprano::QueryResultIterator it = model->executeQuery( query, Soprano::Query::QueryLanguageSparqlNoInference );
    QList<QUrl> fileUris;
    while( it.next() ) {
        // Fetch info about each of these files
        QUrl fileUri = it[0].uri();
        QUrl fileUrl = it[1].uri();

        fileUris << fileUri;

        QString localUrl = fileUrl.toLocalFile();

        // Somehow stat the file
        KIO::UDSEntry uds;
        if ( KIO::StatJob* job = KIO::stat( fileUrl, KIO::HideProgressInfo ) ) {
            // we do not want to wait for the event loop to delete the job
            QScopedPointer<KIO::StatJob> sp( job );
            job->setAutoDelete( false );
            if ( KIO::NetAccess::synchronousRun( job, 0 ) ) {
                uds = job->statResult();
            }
        }
        uds.insert( KIO::UDSEntry::UDS_URL, fileUrl.toString() );
        uds.insert( KIO::UDSEntry::UDS_LOCAL_PATH, localUrl );
        uds.insert( KIO::UDSEntry::UDS_NEPOMUK_URI, fileUri.toString() );

        listEntry( uds, false );
    }

    // Get all the tags the files are tagged with
    QSet<QUrl> allTags;
    foreach(const QUrl& fileUri, fileUris) {
        // Fetch all the tags
        QString query = QString::fromLatin1("select distinct ?t where { %1 nao:hasTag ?t . }")
                        .arg( Soprano::Node::resourceToN3( fileUri ) );

        Soprano::QueryResultIterator it = model->executeQuery( query, Soprano::Query::QueryLanguageSparqlNoInference );
        while( it.next() ) {
            allTags.insert( it[0].uri() );
        }
    }

    // Remove already listed tags from tagUris
    QSet<QUrl> tagsToList = allTags.subtract( tagUris );

    // Emit the total number of files
    totalSize( tagsToList.size() + fileUris.size() );

    // List each of the tags
    foreach(const QUrl& tagUri, tagsToList) {
        listEntry( createUDSEntryForTag( tagUri, fetchIdentifer(tagUri) ), false );
    }

    listEntry( KIO::UDSEntry(), true );
    finished();
}

void Nepomuk2::TagsProtocol::stat(const KUrl& url)
{
    QString path = url.path( KUrl::RemoveTrailingSlash );
    if( path.isEmpty() || path == QLatin1String("/") ) {
        KIO::UDSEntry uds;
        uds.insert( KIO::UDSEntry::UDS_ACCESS, 0700 );
        uds.insert( KIO::UDSEntry::UDS_USER, KUser().loginName() );
        uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1( "inode/directory" ) );

        uds.insert( KIO::UDSEntry::UDS_ICON_OVERLAY_NAMES, QLatin1String( "nepomuk" ) );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_TYPE, i18n( "Tag" ) );

        //uds.insert( KIO::UDSEntry::UDS_NAME, QLatin1String(".") );
        uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, i18n("All Tags") );

        statEntry( uds );
        finished();
        return;
    }

    if( path.startsWith(QChar::fromLatin1('/')) )
        path = path.mid( 1 );


    QString displayName;
    QStringList tags = path.split( QChar::fromLatin1('/') );
    foreach(const QString& tag, tags) {
        QUrl uri = fetchUri(tag);
        if( uri.isEmpty() ) {
            QString text = QString::fromLatin1("Tag %1 does not exist").arg(tag);
            error( KIO::ERR_MALFORMED_URL, text );
            return;
        }

        displayName.append( tag + QLatin1Char(' ') );
    }

    KIO::UDSEntry uds;
    uds.insert( KIO::UDSEntry::UDS_ACCESS, 0700 );
    uds.insert( KIO::UDSEntry::UDS_USER, KUser().loginName() );
    uds.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
    uds.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1( "inode/directory" ) );

    uds.insert( KIO::UDSEntry::UDS_ICON_OVERLAY_NAMES, QLatin1String( "nepomuk" ) );
    uds.insert( KIO::UDSEntry::UDS_DISPLAY_TYPE, i18n( "Tag" ) );

    uds.insert( KIO::UDSEntry::UDS_NAME, path );
    uds.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, displayName );

    statEntry( uds );
    finished();
}

void Nepomuk2::TagsProtocol::mkdir(const KUrl& url, int permissions)
{
    Q_UNUSED(permissions);
    error( KIO::ERR_UNSUPPORTED_ACTION, url.prettyUrl() );
}

void Nepomuk2::TagsProtocol::copy(const KUrl& src, const KUrl& dest, int permissions, KIO::JobFlags flags)
{
    Q_UNUSED(src);
    Q_UNUSED(dest);
    Q_UNUSED(permissions);
    Q_UNUSED(flags);

    error( KIO::ERR_UNSUPPORTED_ACTION, src.prettyUrl() );
}


void Nepomuk2::TagsProtocol::get(const KUrl& url)
{
    KIO::SlaveBase::get(url);
}


void Nepomuk2::TagsProtocol::put(const KUrl& url, int permissions, KIO::JobFlags flags)
{
    KIO::SlaveBase::put(url, permissions, flags);
}


void Nepomuk2::TagsProtocol::rename(const KUrl& src, const KUrl& dest, KIO::JobFlags flags)
{
    Q_UNUSED(src);
    Q_UNUSED(dest);
    Q_UNUSED(flags);

    error( KIO::ERR_UNSUPPORTED_ACTION, src.prettyUrl() );
}

void Nepomuk2::TagsProtocol::del(const KUrl& url, bool isfile)
{
    kDebug() << url;
    SlaveBase::del( url, isfile );
}


void Nepomuk2::TagsProtocol::mimetype(const KUrl& url)
{
    kDebug() << url;
    SlaveBase::mimetype( url );
}

extern "C"
{
    KDE_EXPORT int kdemain( int argc, char **argv )
    {
        // necessary to use other kio slaves
        KComponentData( "kio_nepomuktags" );
        QCoreApplication app( argc, argv );

        if (argc != 4) {
            kError() << "Usage: kio_nepomuktags protocol domain-socket1 domain-socket2";
            exit(-1);
        }

        Nepomuk2::TagsProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();

        return 0;
    }
}

