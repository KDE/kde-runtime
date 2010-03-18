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

#ifndef _NEPOMUK_REMOVABLE_STORAGE_SERVICE_H_
#define _NEPOMUK_REMOVABLE_STORAGE_SERVICE_H_

#include <Nepomuk/Service>

#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtCore/QHash>

#include <Solid/Device>

namespace Solid {
    class StorageAccess;
    class StorageVolume;
}

class KUrl;

namespace Nepomuk {
    /**
     * This service caches metadata stored on removable devices such as usb sticks in the local
     * Nepomuk repository. Thus, the metadata can be searched like any other local file metadata.
     *
     * Once a device is mounted, the data is cached. When unmounted the cached data is removed again.
     */
    class RemovableStorageService : public Service
    {
        Q_OBJECT
        Q_CLASSINFO( "D-Bus Interface", "org.kde.nepomuk.RemovableStorage" )

    public:
        RemovableStorageService( QObject* parent, const QVariantList& );
        ~RemovableStorageService();

    public Q_SLOTS:
        /**
         * Determines the resource URI for a local file URL. This also handles
         * local file URLs that are not stored as such in Nepomuk but using a
         * filex:/ scheme URL for a mounted filesystem.
         *
         * This method is called by libnepomuk's Resource class to handle
         * filex:/ URLs transparently.
         *
         * \return The resource URI for \p url or an empty string if it could
         * not be found (this is the case if the filesystem is not mounted or
         * if \p url is simply invalid or not a used URL.)
         */
        Q_SCRIPTABLE QString resourceUriFromLocalFileUrl( const QString& url );

        Q_SCRIPTABLE QStringList currentlyMountedAndIndexed();

    private Q_SLOTS:
        void slotSolidDeviceAdded( const QString& udi );
        void slotSolidDeviceRemoved( const QString& udi );

        /**
         * Reacts on Solid::StorageAccess::accessibilityChanged:
         * \li If \p accessible is true, the metadata from the removable storage
         * is cached in the local Nepomuk store.
         * \li Otherwise the corresponding cache is removed.
         */
        void slotAccessibilityChanged( bool accessible, const QString& udi );

    private:
        void initCacheEntries();

        class Entry {
            RemovableStorageService* q;

        public:
            Entry() {}
            Entry( RemovableStorageService* );

            KUrl constructRelativeUrl( const QString& path ) const;
            QString constructLocalPath( const KUrl& filexUrl ) const;
            bool hasLastMountPath() const;

            Solid::Device m_device;
            QString m_lastMountPath;

            // need to cache the device properties in case
            // an USB device is simply ejected without properly
            // unmounting before
            QString m_description;
            QString m_uuid;
        };
        QHash<QString, Entry> m_metadataCache;

        Entry* createCacheEntry( const Solid::Device& dev );
        Entry* findEntryByFilePath( const QString& path );
    };
}

#endif // _NEPOMUK_REMOVABLE_STORAGE_SERVICE_H_
