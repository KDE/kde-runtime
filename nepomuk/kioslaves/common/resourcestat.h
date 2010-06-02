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

#ifndef _NEPOMUK_RESOURCE_STAT_H_
#define _NEPOMUK_RESOURCE_STAT_H_

class QString;
class KUrl;
namespace KIO {
    class UDSEntry;
}
namespace Solid {
    class StorageAccess;
}

namespace Nepomuk {

    class Resource;

    /**
     * Remove all query parts from a KUrl.
     */
    KUrl stripQuery( const KUrl& url );

    /**
     * Split the filename part off a nepomuk:/ URI. This is used in many methods for identifying
     * entries listed from tags and filesystems.
     */
    Nepomuk::Resource splitNepomukUrl( const KUrl& url, QString* filename = 0 );

    /**
     * Check if the resource represents a file on a removable media using a filex:/
     * URL.
     */
    bool isRemovableMediaFile( const Nepomuk::Resource& res );

    /**
     * Create a Solid storage access interface from the volume UUID.
     */
    Solid::StorageAccess* storageFromUUID( const QString& uuid );

    /**
     * Mount a storage volume via Solid and wait for it to be mounted with
     * a timeout of 20 seconds.
     */
    bool mountAndWait( Solid::StorageAccess* storage );

    /**
     * Get the mount path of a nfo:Filesystem resource as created by the removable storage service.
     */
    KUrl determineFilesystemPath( const Nepomuk::Resource& fsRes );

    /**
     * Determine the label for a filesystem \p res is stored on.
     */
    QString getFileSystemLabelForRemovableMediaFileUrl( const Nepomuk::Resource& res );

    /**
     * Convert a filex:/ URL into its actual local file URL.
     *
     * \param url The filex:/ URL to convert
     * \param evenMountIfNecessary If true an existing unmouted volume will be mounted to grant access to the local file.
     *
     * \return The converted local URL or an invalid URL if the filesystem the file is stored on could not be/was not mounted.
     */
    KUrl convertRemovableMediaFileUrl( const KUrl& url, bool evenMountIfNecessary = false );

    /**
     * Stat a Nepomuk resource. Might start a local event loop
     */
    KIO::UDSEntry statNepomukResource( const Nepomuk::Resource& res );

    /**
     * \return \p true for all resources that will get a valid redirection url in
     * redirectionUrl().
     */
    bool willBeRedirected( const Nepomuk::Resource& res );

    /**
     * Create a redirection query URL for resources such as tags or filesystems.
     * For other resources an empty KUrl is returned.
     */
    KUrl redirectionUrl( const Nepomuk::Resource& res );

    /**
     * Convert a nepomuk:/ URL to a file:/ URL if possible.
     * Otherwise return an empty KUrl.
     */
    KUrl nepomukToFileUrl( const KUrl& url, bool evenMountIfNecessary = false );
}

#endif
