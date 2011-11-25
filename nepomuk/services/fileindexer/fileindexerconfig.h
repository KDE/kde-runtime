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
#include <QtCore/QMutex>

#include <kconfig.h>
#include <kio/global.h>

#include "regexpcache.h"

namespace Nepomuk {
    /**
     * Active config class which emits signals if the config
     * was changed, for example if the KCM saved the config file.
     */
    class FileIndexerConfig : public QObject
    {
        Q_OBJECT

    public:
        FileIndexerConfig(QObject* parent = 0);
        ~FileIndexerConfig();
        static FileIndexerConfig* self();

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

        bool indexHiddenFilesAndFolders() const;

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
         * A "hidden" config option which allows to disable the initial
         * update of all indexed folders.
         *
         * This should be used in combination with isInitialRun() to make
         * sure all folders are at least indexed once.
         */
        bool initialUpdateDisabled() const;

        /**
         * A "hidden" config option which allows to disable the feature
         * where the file indexing is suspended when in powersave mode.
         * This is especially useful for mobile devices which always run
         * on battery.
         *
         * At some point this should be a finer grained configuration.
         */
        bool suspendOnPowerSaveDisabled() const;

        /**
         * Check if \p path should be indexed taking into account
         * the includeFolders(), the excludeFolders(), and the
         * excludeFilters().
         *
         * Be aware that this method does not check if parent dirs
         * match any of the exclude filters. Only the name of the
         * dir itself it checked.
         *
         * \return \p true if the file or folder at \p path should
         * be indexed according to the configuration.
         */
        bool shouldBeIndexed( const QString& path ) const;

        /**
         * Check if the folder at \p path should be indexed.
         *
         * Be aware that this method does not check if parent dirs
         * match any of the exclude filters. Only the name of the
         * dir itself it checked.
         *
         * \return \p true if the folder at \p path should
         * be indexed according to the configuration.
         */
        bool shouldFolderBeIndexed( const QString& path ) const;

        /**
         * Check \p fileName for all exclude filters. This does
         * not take file paths into account.
         *
         * \return \p true if a file with name \p filename should
         * be indexed according to the configuration.
         */
        bool shouldFileBeIndexed( const QString& fileName ) const;

    Q_SIGNALS:
        void configChanged();

    public Q_SLOTS:
        /**
         * Should be called once the initial indexing is done, ie. all folders
         * have been indexed.
         */
        void setInitialRun(bool isInitialRun);

    public Q_SLOTS:
        /**
         * Reread the config from disk and update the configuration cache.
         * This is only required for testing as normally the config updates
         * itself whenever the config file on disk changes.
         */
        void forceConfigUpdate();

    private Q_SLOTS:
        void slotConfigDirty();

    private:
        /**
         * Check if \p path is in the list of folders to be indexed taking
         * include and exclude folders into account.
         * \p folder is set to the folder which was the reason for the descision.
         */
        bool folderInFolderList( const QString& path, QString& folder ) const;
        void buildFolderCache();
        void buildExcludeFilterRegExpCache();

        mutable KConfig m_config;

        /// Caching cleaned up list (no duplicates, no useless entries, etc.)
        QList<QPair<QString, bool> > m_folderCache;

        /// cache of regexp objects for all exclude filters
        /// to prevent regexp parsing over and over
        RegExpCache m_excludeFilterRegExpCache;

        mutable QMutex m_folderCacheMutex;

        static FileIndexerConfig* s_self;
    };
}

#endif
