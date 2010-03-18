/* This file is part of the KDE Project
   Copyright (c) 2008-2009 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_STRIGI_SERVICE_CONFIG_H_
#define _NEPOMUK_STRIGI_SERVICE_CONFIG_H_

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QRegExp>

#include <kconfig.h>
#include <kio/global.h>


namespace Nepomuk {
    /**
     * Active config class which emits signals if the config
     * was changed, for example if the KCM saved the config file.
     */
    class StrigiServiceConfig : public QObject
    {
        Q_OBJECT

    public:
        ~StrigiServiceConfig();
        static StrigiServiceConfig* self();

        /**
         * A cleaned up list of all include and exclude folders
         * with their respective include/exclude flag sorted by
         * path. None of the paths have trailing slashes.
         */
        QList<QPair<QString, bool> > folders() const;

        /**
         * The folders to search for files to analyze. Cached and cleaned up.
         */
        QStringList includeFolders() const;

        /**
         * The folders that should be excluded. Cached and cleaned up.
         * It is perfectly possible to include subfolders again.
         */
        QStringList excludeFolders() const;

        QStringList excludeFilters() const;

        QList<QRegExp> excludeFilterRegExps() const { return m_excludeFilterRegExpCache; }

        bool indexHiddenFolders() const;

        /**
         * The minimal available disk space. If it drops below
         * indexing will be suspended.
         */
        KIO::filesize_t minDiskSpace() const;

        /**
         * true the first time the service is run (or after manually
         * tampering with the config.
         */
        bool isInitialRun() const;

        /**
         * Check if the folder should be indexed based on
         * folders() and excludeFolders()
         */
        bool shouldFolderBeIndexed( const QString& ) const;

        /**
         * Check \p fileName for all exclude filters. This does
         * not take file paths into account.
         */
        bool shouldFileBeIndexed( const QString& fileName ) const;

    Q_SIGNALS:
        void configChanged();

    private Q_SLOTS:
        void slotConfigDirty();

    private:
        StrigiServiceConfig();

        /**
         * Check if \p path is in the list of folders to be indexed taking
         * include and exclude folders into account.
         * \p exact is set to true if \p path is a top level folder from
         * the include or exclude list.
         */
        bool folderInFolderList( const QString& path, bool& exact ) const;
        void buildFolderCache();
        void buildExcludeFilterRegExpCache();

        KConfig m_config;

        /// Caching cleaned up list (no duplicates, no useless entries, etc.)
        QList<QPair<QString, bool> > m_folderCache;

        /// cache of regexp objects for all exclude filters
        /// to prevent regexp parsing over and over
        QList<QRegExp> m_excludeFilterRegExpCache;
    };
}

#endif
