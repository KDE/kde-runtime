/* This file is part of the KDE Project
   Copyright (c) 2007-2010 Sebastian Trueg <trueg@kde.org>

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

#include "nepomukfilewatch.h"
#include "metadatamover.h"
#include "strigiserviceinterface.h"
#include "fileexcludefilters.h"
#include "invalidfileresourcecleaner.h"
#include "removabledeviceindexnotification.h"
#include "../strigi/strigiserviceconfig.h"

#ifdef BUILD_KINOTIFY
#include "kinotify.h"
#endif

#include <QtCore/QDir>
#include <QtCore/QThread>
#include <QtDBus/QDBusConnection>

#include <KDebug>
#include <KUrl>
#include <KPluginFactory>
#include <KConfigGroup>

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Vocabulary/NIE>

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Node>

#include <Solid/DeviceNotifier>
#include <Solid/DeviceInterface>
#include <Solid/Device>
#include <Solid/StorageDrive>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>
#include <Solid/NetworkShare>
#include <Solid/Predicate>


NEPOMUK_EXPORT_SERVICE( Nepomuk::FileWatch, "nepomukfilewatch")


#ifdef BUILD_KINOTIFY
namespace {
    /**
     * A variant of KInotify which ignores all paths in the Nepomuk
     * ignore list.
     */
    class IgnoringKInotify : public KInotify
    {
    public:
        IgnoringKInotify( RegExpCache* rec, QObject* parent );
        ~IgnoringKInotify();

        bool addWatch( const QString& path, WatchEvents modes, WatchFlags flags = WatchFlags() );
    protected:
        bool filterWatch( const QString & path, WatchEvents & modes, WatchFlags & flags );

    private:
        RegExpCache* m_pathExcludeRegExpCache;
    };

    IgnoringKInotify::IgnoringKInotify( RegExpCache* rec, QObject* parent )
        : KInotify( parent ),
          m_pathExcludeRegExpCache( rec )
    {
    }

    IgnoringKInotify::~IgnoringKInotify()
    {
    }

    bool IgnoringKInotify::addWatch( const QString& path, WatchEvents modes, WatchFlags flags )
    {
        if( !m_pathExcludeRegExpCache->filenameMatch( path ) ) {
            return KInotify::addWatch( path, modes, flags );
        }
        else {
            kDebug() << "Ignoring watch patch" << path;
            return false;
        }
    }

    bool IgnoringKInotify::filterWatch( const QString & path, WatchEvents & modes, WatchFlags & flags )
    {
        Q_UNUSED( flags );

        //Only watch the strigi index folders for file creation and change.
        if( Nepomuk::StrigiServiceConfig::self()->shouldFolderBeIndexed( path ) ) {
            modes |= KInotify::EventCreate;
            modes |= KInotify::EventModify;
        }
        else {
            modes &= (~KInotify::EventCreate);
            modes &= (~KInotify::EventModify);
        }

        return true;
    }
}
#endif // BUILD_KINOTIFY


Nepomuk::FileWatch::FileWatch( QObject* parent, const QList<QVariant>& )
    : Service( parent )
{
    // the list of default exclude filters we use here differs from those
    // that can be configured for the strigi service
    // the default list should only contain files and folders that users are
    // very unlikely to ever annotate but that change very often. This way
    // we avoid a lot of work while hopefully not breaking the workflow of
    // too many users.
    m_pathExcludeRegExpCache = new RegExpCache();
    m_pathExcludeRegExpCache->rebuildCacheFromFilterList( defaultExcludeFilterList() );

    // start the mover thread
    m_metadataMover = new MetadataMover( mainModel(), this );
    connect( m_metadataMover, SIGNAL(movedWithoutData(QString)),
             this, SLOT(slotMovedWithoutData(QString)),
             Qt::QueuedConnection );
    m_metadataMover->start();

#ifdef BUILD_KINOTIFY
    // monitor the file system for changes (restricted by the inotify limit)
    m_dirWatch = new IgnoringKInotify( m_pathExcludeRegExpCache, this );

    connect( m_dirWatch, SIGNAL( moved( QString, QString ) ),
             this, SLOT( slotFileMoved( QString, QString ) ) );
    connect( m_dirWatch, SIGNAL( deleted( QString, bool ) ),
             this, SLOT( slotFileDeleted( QString, bool ) ) );
    connect( m_dirWatch, SIGNAL( created( QString ) ),
             this, SLOT( slotFileCreated( QString ) ) );
    connect( m_dirWatch, SIGNAL( modified( QString ) ),
             this, SLOT( slotFileModified( QString ) ) );
    connect( m_dirWatch, SIGNAL( watchUserLimitReached() ),
             this, SLOT( slotInotifyWatchUserLimitReached() ) );

    // recursively watch the whole home dir

    // FIXME: we cannot simply watch the folders that contain annotated files since moving
    // one of these files out of the watched "area" would mean we "lose" it, i.e. we have no
    // information about where it is moved.
    // On the other hand only using the homedir means a lot of restrictions.
    // One dummy solution would be a hybrid: watch the whole home dir plus all folders that
    // contain annotated files outside of the home dir and hope for the best

    watchFolder( QDir::homePath() );
#else
    connectToKDirWatch();
#endif

    // we automatically watch newly mounted media - it is very unlikely that anything non-interesting is mounted
    addWatchesForMountedRemovableMedia();
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceAdded( const QString& ) ),
             this, SLOT( slotSolidDeviceAdded( const QString& ) ) );


    (new InvalidFileResourceCleaner(this))->start();
    
    connect( StrigiServiceConfig::self(), SIGNAL( configChanged() ),
             this, SLOT( updateIndexedFoldersWatches() ) );
}


Nepomuk::FileWatch::~FileWatch()
{
    kDebug();
    m_metadataMover->stop();
    m_metadataMover->wait();
}


void Nepomuk::FileWatch::watchFolder( const QString& path )
{
    kDebug() << path;
#ifdef BUILD_KINOTIFY
    if ( m_dirWatch && !m_dirWatch->watchingPath( path ) )
        m_dirWatch->addWatch( path,
                              KInotify::WatchEvents( KInotify::EventMove|KInotify::EventDelete|KInotify::EventDeleteSelf|KInotify::EventCreate|KInotify::EventModify ),
                              KInotify::WatchFlags() );
#endif
}


void Nepomuk::FileWatch::slotFileMoved( const QString& urlFrom, const QString& urlTo )
{
    if( !ignorePath( urlFrom ) ) {
        KUrl from( urlFrom );
        KUrl to( urlTo );

        kDebug() << from << to;

        m_metadataMover->moveFileMetadata( from, to );
    }
}


void Nepomuk::FileWatch::slotFilesDeleted( const QStringList& paths )
{
    KUrl::List urls;
    foreach( const QString& path, paths ) {
        if( !ignorePath( path ) ) {
            urls << KUrl(path);
        }
    }

    if( !urls.isEmpty() ) {
        kDebug() << urls;
        m_metadataMover->removeFileMetadata( urls );
    }
}


void Nepomuk::FileWatch::slotFileDeleted( const QString& urlString, bool isDir )
{
    // Directories must always end with a trailing slash '/'
    QString url = urlString;
    if( isDir && url[ url.length() - 1 ] != '/') {
        url.append('/');
    }
    slotFilesDeleted( QStringList( url ) );
}


void Nepomuk::FileWatch::slotFileCreated( const QString& path )
{
    kDebug() << path;
    updateFolderViaStrigi( path );
}


void Nepomuk::FileWatch::slotFileModified( const QString& path )
{
    updateFolderViaStrigi( path );
}


void Nepomuk::FileWatch::slotMovedWithoutData( const QString& path )
{
    updateFolderViaStrigi( path );
}


// static
void Nepomuk::FileWatch::updateFolderViaStrigi( const QString& path )
{
    if( StrigiServiceConfig::self()->shouldBeIndexed(path) ) {
        //
        // Tell Strigi service (if running) to update the newly created
        // folder or the folder containing the newly created file
        //
        org::kde::nepomuk::Strigi strigi( "org.kde.nepomuk.services.nepomukstrigiservice", "/nepomukstrigiservice", QDBusConnection::sessionBus() );
        if ( strigi.isValid() ) {
            strigi.updateFolder( path, false /* non-recursive */, false /* no forced update */ );
        }
    }
}


void Nepomuk::FileWatch::connectToKDirWatch()
{
    // monitor KIO for changes
    QDBusConnection::sessionBus().connect( QString(), QString(), "org.kde.KDirNotify", "FileMoved",
                                           this, SIGNAL( slotFileMoved( const QString&, const QString& ) ) );
    QDBusConnection::sessionBus().connect( QString(), QString(), "org.kde.KDirNotify", "FilesRemoved",
                                           this, SIGNAL( slotFilesDeleted( const QStringList& ) ) );
}


#ifdef BUILD_KINOTIFY
void Nepomuk::FileWatch::slotInotifyWatchUserLimitReached()
{
    // we do it the brutal way for now hoping with new kernels and defaults this will never happen
    delete m_dirWatch;
    m_dirWatch = 0;
    connectToKDirWatch();
}
#endif


bool Nepomuk::FileWatch::ignorePath( const QString& path )
{
    // when using KInotify there is no need to check the folder since
    // we only watch interesting folders to begin with.
    return m_pathExcludeRegExpCache->filenameMatch( path );
}


void Nepomuk::FileWatch::updateIndexedFoldersWatches()
{
#ifdef BUILD_KINOTIFY
    if( m_dirWatch ) {
        QStringList folders = StrigiServiceConfig::self()->includeFolders();
        foreach( const QString & folder, folders ) {
            m_dirWatch->removeWatch( folder );
            watchFolder( folder );
        }
    }
#endif
}

namespace {
    bool isUsableDevice( const Solid::Device& dev ) {
        if ( dev.is<Solid::StorageAccess>() ) {
            if( dev.is<Solid::StorageVolume>() &&
                    dev.parent().is<Solid::StorageDrive>() &&
                    ( dev.parent().as<Solid::StorageDrive>()->isRemovable() ||
                      dev.parent().as<Solid::StorageDrive>()->isHotpluggable() ) ) {
                const Solid::StorageVolume* volume = dev.as<Solid::StorageVolume>();
                if ( !volume->isIgnored() && volume->usage() == Solid::StorageVolume::FileSystem )
                    return true;
            }
            else if(dev.is<Solid::NetworkShare>()) {
                return !dev.as<Solid::NetworkShare>()->url().isEmpty();
            }
        }

        // fallback
        return false;
    }

    bool isUsableDevice( const QString& udi ) {
        Solid::Device dev( udi );
        return isUsableDevice( dev );
    }
}

void Nepomuk::FileWatch::addWatchesForMountedRemovableMedia()
{
    QList<Solid::Device> devices
            = Solid::Device::listFromQuery(QLatin1String("StorageVolume.usage=='FileSystem'"))
            + Solid::Device::listFromType(Solid::DeviceInterface::NetworkShare);
    foreach( const Solid::Device& dev, devices ) {
        slotSolidDeviceAdded(dev.udi());
    }
}

void Nepomuk::FileWatch::slotSolidDeviceAdded(const QString &udi)
{
    //
    // We are interested in every mount there is.
    //
    Solid::Device dev(udi);
    if ( isUsableDevice(dev) ) {
        const Solid::StorageAccess* storage = dev.as<Solid::StorageAccess>();
        if ( storage && !storage->isIgnored() ) {
            connect(storage, SIGNAL(accessibilityChanged(bool,QString)), SLOT(slotDeviceAccessibilityChanged(bool,QString)));
            slotDeviceAccessibilityChanged(storage->isAccessible(), dev.udi());
        }
    }
}

void Nepomuk::FileWatch::slotDeviceAccessibilityChanged(bool accessible, const QString &udi)
{
    if(accessible) {
        Solid::Device dev(udi);
        if(Solid::StorageAccess* sa = dev.as<Solid::StorageAccess>()) {
            kDebug() << "Installing watch for removable storage at mount point" << sa->filePath();
            watchFolder(sa->filePath());

            //
            // now that the device is mounted we can clean up our db - in case we have any
            // data for file that have been deleted from the device in the meantime.
            //
            InvalidFileResourceCleaner* cleaner = new InvalidFileResourceCleaner(this);
            cleaner->start(sa->filePath());

            //
            // tell Strigi to update the newly mounted device
            //
            KConfig strigiConfig( "nepomukstrigirc" );
            int index = 0;
            // FIXME: use the medium URI as generated by the removable media model instead of the non-unique UDI
//            if(strigiConfig.group("Devices").hasKey(udi)) {
//                index = strigiConfig.group("Devices").readEntry(udi, false) ? 1 : -1;
//            }

            const bool indexNewlyMounted = strigiConfig.group( "RemovableMedia" ).readEntry( "index newly mounted", false );
            const bool askIndividually = strigiConfig.group( "RemovableMedia" ).readEntry( "ask user", false );

            if( index == 0 && indexNewlyMounted && !askIndividually ) {
                index = 1;
            }

            // index automatically
            if( index == 1 ) {
                kDebug() << "Device configured for automatic indexing. Calling Strigi service.";
                org::kde::nepomuk::Strigi strigi( "org.kde.nepomuk.services.nepomukstrigiservice", "/nepomukstrigiservice", QDBusConnection::sessionBus() );
                if ( strigi.isValid() ) {
                    strigi.indexFolder( sa->filePath(), true /* recursive */, false /* no forced update */ );
                }
            }

            // ask the user if we should index
            else if( index == 0 && indexNewlyMounted && askIndividually ) {
                kDebug() << "Device unknown. Asking user for action.";
                (new RemovableDeviceIndexNotification(dev, this))->sendEvent();
            }

            else {
                // TODO: remove all the indexed info
                kDebug() << "Device configured to not be indexed.";
            }
        }
    }
}


#include "nepomukfilewatch.moc"
