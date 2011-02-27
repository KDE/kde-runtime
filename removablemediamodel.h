/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef REMOVABLEMEDIAMODEL_H
#define REMOVABLEMEDIAMODEL_H

#include <Soprano/FilterModel>

#include <QtCore/QMutex>

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
 * Filter model that performs automatic conversion of file URLs
 * from and to the filex:/ protocol.
 *
 * The basic idea is that all resources representing files on removable media
 * do have a nie:url with schema filex:/<UUID>/<relative-path> where <UUID>
 * is the UUID of the removable media and <relative-path> is the path of the
 * file on the medium. This is necessary since different media might be mounted
 * at the same mount point which could lead to clashing URLs.
 *
 * In theory all file URLs could be stored this way. However, to improve performance
 * this conversion is only done for files on removable media.
 *
 * The following conversions are performed to provide a handling
 * of files on removable media that is as transparent as possible:
 *
 * \li file:/ URLs used in all statement commands and queries are converted to
 * the corresponding filex:/ URL if necessary.
 * \li filex:/ URLs referring to existing files (on a medium currently mounted)
 * are converted to file:/ URLs in query and listStatement results.
 * \li filex:/ URLs referring to non-existing files (on a medium currently not
 * mounted) will not be converted (since it is not possible).
 *
 * Thus, whenever clients see a filex:/ URL they can be sure that it refers to
 * a file that is not accessible at that time and act accordingly (show a message
 * box, hide the query results, etc.).
 *
 * \author Sebastian Trueg <trueg@kde.org>
 */
class RemovableMediaModel : public Soprano::FilterModel
{
    Q_OBJECT

public:
    RemovableMediaModel(Soprano::Model *parent = 0);
    ~RemovableMediaModel();

    // overloaded methods that provide file:/filex: transparent conversion
    Soprano::Error::ErrorCode addStatement(const Soprano::Statement &statement);
    Soprano::Error::ErrorCode removeStatement(const Soprano::Statement &statement);
    Soprano::Error::ErrorCode removeAllStatements(const Soprano::Statement &statement);
    bool containsStatement(const Soprano::Statement &statement) const;
    bool containsAnyStatement(const Soprano::Statement &statement) const;
    Soprano::StatementIterator listStatements(const Soprano::Statement &partial) const;
    Soprano::QueryResultIterator executeQuery(const QString &query, Soprano::Query::QueryLanguage language, const QString &userQueryLanguage) const;

private Q_SLOTS:
    void slotSolidDeviceAdded( const QString& udi );
    void slotSolidDeviceRemoved( const QString& udi );

    /**
     * Reacts on Solid::StorageAccess::accessibilityChanged and updates the entry cache.
     */
    void slotAccessibilityChanged( bool accessible, const QString& udi );

private:
    void initCacheEntries();

    /**
     * Converts file:/ URLs into their filex:/ counterpart if necessary.
     * Used in all statement handling methods.
     */
    Soprano::Statement convertFileUrls(const Soprano::Statement& s) const;

    /**
     * Converts file:/ URLs into their filex:/ counterpart if necessary.
     * This includes a simple handling of REGEX filters.
     */
    QString convertFileUrls(const QString& query) const;

    Soprano::Statement convertFilexUrls(const Soprano::Statement& s) const;
    Soprano::Node convertFilexUrl(const Soprano::Node& node) const;

    class Entry {
    public:
        Entry() {}

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
    const Entry* findEntryByFilePath( const QString& path ) const;
    const Entry* findEntryByUuid(const QString& uuid) const;

    mutable QMutex m_entryCacheMutex;

    // cached strings for quicker comparison
    const QString m_filexSchema;
    const QString m_fileSchema;

    class StatementIteratorBackend;
    class QueryResultIteratorBackend;
};
}

#endif
