/* This file is part of the KDE Project
   Copyright (c) 2009 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "removablestorageservice.h"
#include "nfo.h"
#include "nie.h"
#include "strigiserviceinterface.h"

#include <QtDBus/QDBusConnection>
#include <QtCore/QUuid>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <KDebug>
#include <KUrl>
#include <KPluginFactory>
#include <KStandardDirs>
#include <KConfig>
#include <KConfigGroup>
#include <kdirnotify.h>

#include <Solid/DeviceNotifier>
#include <Solid/DeviceInterface>
#include <Solid/Block>
#include <Solid/Device>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>
#include <Solid/StorageAccess>
#include <Solid/Predicate>

#include <Soprano/StatementIterator>
#include <Soprano/Statement>
#include <Soprano/NodeIterator>
#include <Soprano/Node>
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Serializer>
#include <Soprano/Parser>
#include <Soprano/PluginManager>
#include <Soprano/Util/SimpleStatementIterator>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/Xesam>

#include <Nepomuk/Resource>
#include <Nepomuk/Variant>

//
// A few notes on filex:
// Relative URLs:
//   without device: filex:///relative/path
//   with device:    filex://<UID>/relative/path
// [Absolute URLs:
//   without device: filex:///absolute/path
//   with device:    filex:/<UID>//absolute/path]
//

using namespace Soprano;

namespace {
    bool isUsableVolume( const Solid::Device& dev ) {
        if ( dev.is<Solid::StorageVolume>() &&
             dev.is<Solid::StorageAccess>() &&
             dev.parent().is<Solid::StorageDrive>() &&
             dev.parent().as<Solid::StorageDrive>()->isRemovable() ) {
            const Solid::StorageVolume* volume = dev.as<Solid::StorageVolume>();
            if ( !volume->isIgnored() && volume->usage() == Solid::StorageVolume::FileSystem )
                return true;
        }

        return false;
    }

    bool isUsableVolume( const QString& udi ) {
        Solid::Device dev( udi );
        return isUsableVolume( dev );
    }
}


Nepomuk::RemovableStorageService::RemovableStorageService( QObject* parent, const QList<QVariant>& )
    : Service( parent )
{
    kDebug();

    initCacheEntries();

    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceAdded( const QString& ) ),
             this, SLOT( slotSolidDeviceAdded( const QString& ) ) );
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceRemoved( const QString& ) ),
             this, SLOT( slotSolidDeviceRemoved( const QString& ) ) );
}


Nepomuk::RemovableStorageService::~RemovableStorageService()
{
}


QString Nepomuk::RemovableStorageService::resourceUriFromLocalFileUrl( const QString& urlString )
{
    KUrl url( urlString );
    KUrl fileXUrl;
    QString path = url.path();

    for ( QHash<QString, Entry>::ConstIterator it = m_metadataCache.constBegin();
          it != m_metadataCache.constEnd(); ++it ) {
        const Entry& entry = it.value();
        if ( !entry.m_lastMountPath.isEmpty() && path.startsWith( entry.m_lastMountPath ) ) {
            // construct the filex:/ URL and use it below
            fileXUrl = entry.constructRelativeUrl( path );
            break;
        }
    }


    //
    // This is a query similar to the one Resource in libnepomuk uses. Once we found the filex:/ URL above we
    // can simply continue with that. If the entry does not exist yet libnepomuk will create a new
    // random nepomuk:/ URL.
    //
    QString query;
    if ( fileXUrl.isEmpty() )
        query = QString::fromLatin1("select distinct ?r ?o where { "
                                    "{ ?r %1 %2 . } "
                                    "UNION "
                                    "{ %2 ?p ?o . } "
                                    "} LIMIT 1")
                .arg( Soprano::Node::resourceToN3(Nepomuk::Vocabulary::NIE::url()),
                      Soprano::Node::resourceToN3(url) );
    else
        query = QString::fromLatin1("select distinct ?r ?o where { "
                                    "{ ?r %1 %2 . } "
                                    "UNION "
                                    "{ ?r %1 %3 . } "
                                    "UNION "
                                    "{ %2 ?p ?o . } "
                                    "} LIMIT 1")
                .arg( Soprano::Node::resourceToN3(Nepomuk::Vocabulary::NIE::url()),
                      Soprano::Node::resourceToN3(url),
                      Soprano::Node::resourceToN3(fileXUrl) );

    Soprano::QueryResultIterator it = mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    if( it.next() ) {
        KUrl resourceUri = it["r"].uri();
        if( resourceUri.isEmpty() )
            return url.url();
        else
            return resourceUri.url();
    }
    else {
        return QString();
    }
}


void Nepomuk::RemovableStorageService::initCacheEntries()
{
    QList<Solid::Device> devices
        = Solid::Device::listFromQuery( QLatin1String( "StorageVolume.usage=='FileSystem'" ) );
    foreach( const Solid::Device& dev, devices ) {
        if ( isUsableVolume( dev ) ) {
            Entry* entry = createCacheEntry( dev );
            const Solid::StorageAccess* storage = entry->m_device.as<Solid::StorageAccess>();
            if ( storage && storage->isAccessible() )
                slotAccessibilityChanged( true, dev.udi() );
        }
    }
}


Nepomuk::RemovableStorageService::Entry* Nepomuk::RemovableStorageService::createCacheEntry( const Solid::Device& dev )
{
    Entry entry( this );
    entry.m_device = dev;
    connect( dev.as<Solid::StorageAccess>(), SIGNAL(accessibilityChanged(bool, QString)),
             this, SLOT(slotAccessibilityChanged(bool, QString)) );

    m_metadataCache.insert( dev.udi(), entry );

    kDebug() << "Found removable storage volume for Nepomuk docking:" << dev.udi() << dev.description();

    return &m_metadataCache[dev.udi()];
}


Nepomuk::RemovableStorageService::Entry* Nepomuk::RemovableStorageService::findEntryByFilePath( const QString& path )
{
    for( QHash<QString, Entry>::iterator it = m_metadataCache.begin();
         it != m_metadataCache.end(); ++it ) {
        Entry& entry = *it;
        if ( entry.m_device.as<Solid::StorageAccess>()->isAccessible() &&
             path.startsWith( entry.m_device.as<Solid::StorageAccess>()->filePath() ) )
            return &entry;
    }
    return 0;
}


void Nepomuk::RemovableStorageService::slotSolidDeviceAdded( const QString& udi )
{
    kDebug() << udi;

    if ( isUsableVolume( udi ) ) {
        createCacheEntry( Solid::Device( udi ) );
    }
}


void Nepomuk::RemovableStorageService::slotSolidDeviceRemoved( const QString& udi )
{
    kDebug() << udi;
    if ( m_metadataCache.contains( udi ) ) {
        kDebug() << "Found removable storage volume for Nepomuk undocking:" << udi;
        m_metadataCache.remove( udi );
    }
}


void Nepomuk::RemovableStorageService::slotAccessibilityChanged( bool accessible, const QString& udi )
{
    kDebug() << accessible << udi;

    Entry& entry = m_metadataCache[udi];
    if ( accessible ) {
        //
        // cache new mount path
        //
        entry.m_lastMountPath = entry.m_device.as<Solid::StorageAccess>()->filePath();

        //
        // tell Strigi to update the newly mounted device
        //
        if( KConfig( "nepomukstrigirc" ).group( "General" ).readEntry( "index newly mounted", false ) ) {
            org::kde::nepomuk::Strigi interface( "org.kde.nepomuk.services.nepomukstrigiservice",
                                                 "/nepomukstrigiservice",
                                                 QDBusConnection::sessionBus() );
            interface.indexFolder( entry.m_lastMountPath, false );
        }

        // TODO: remove all metadata for files that have been removed from the media whilst it was not mounted with us.
    }
    else {
        //
        // The first thing we need to do is to inform nepomuk:/ kio slave instances that something has changed
        // so any caches will be cleared. Otherwise KDirModel and friends might try to access the old media URLs
        // which nepomuk:/ KIO has rewritten without asking again.
        //
        // FIXME: cannot use "org::kde::KDirNotify::emitFilesRemoved( QStringList() << entry.m_lastMountPath );"
        //        as that will make the FileWatchService delete all metadata
        //

        //
        // First we create the filesystem resource. We mostly need this for the user readable label.
        //
        const Solid::StorageVolume* volume = entry.m_device.as<Solid::StorageVolume>();
        Nepomuk::Resource fsRes( volume->uuid(), Nepomuk::Vocabulary::NFO::Filesystem() );
        fsRes.setLabel( entry.m_device.description() );

        //
        // We change all absolute file:/ URLs to relative filex:/ URLs which include the solid id
        //
        // FIXME: do this asyncroneously
        // FIXME: We need to handle the mountpoint folder resource separately. It can no longer be the parent resource for the top-level files on the device.
        QString query = QString::fromLatin1( "select ?r ?url ?g where { " // FIXME: can Virtuoso directly select a substring of the url? Can we maybe even do this in an update query?
                                             "graph ?g { ?r %1 ?url . } . "
                                             "FILTER(REGEX(STR(?url),'^file://%2/')) . }" )
                        .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ) )
                        .arg( entry.m_lastMountPath );
        kDebug() << query;
        QList<Soprano::BindingSet> bindings = mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql ).allBindings();

        foreach( const Soprano::BindingSet& b, bindings ) {
            const QUrl resource = b["r"].uri();
            const QString path = b["url"].uri().path();
            const QUrl graph = b["g"].uri();

            // construct the new filex:/ URL from the solid device UID and the relative path on the device
            const QUrl filexUrl = entry.constructRelativeUrl( path );

            kDebug() << "Converting URL" << b["url"] << "to" << filexUrl;

            // for performance reasons we do not use Nepomuk::Resource but do it the old fashioned way
            mainModel()->removeAllStatements( resource, Nepomuk::Vocabulary::NIE::url(), Node() );
            mainModel()->addStatement( resource, Nepomuk::Vocabulary::NIE::url(), filexUrl, graph );

            // IDEA: What about nie:hasPart nfo:FileSystem for all top-level items instead of the nfo:Folder that is the mount point.
            // But then we run into the recursion problem
            Resource fileRes( resource );
            fileRes.addProperty( Nepomuk::Vocabulary::NIE::isPartOf(), fsRes );
        }
    }
    kDebug() << "done";
}


Nepomuk::RemovableStorageService::Entry::Entry( RemovableStorageService* parent )
    : q( parent )
{
}


KUrl Nepomuk::RemovableStorageService::Entry::constructRelativeUrl( const QString& path ) const
{
    const Solid::StorageVolume* volume = m_device.as<Solid::StorageVolume>();
    if ( volume ) {
        const QString relativePath = path.mid( m_lastMountPath.count() );
        return KUrl( QLatin1String("filex://") + volume->uuid() + relativePath );
    }
    else {
        return KUrl();
    }
}

NEPOMUK_EXPORT_SERVICE( Nepomuk::RemovableStorageService, "nepomukremovablestorageservice")

#include "removablestorageservice.moc"
