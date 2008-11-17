/* This file is part of the KDE Project
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_STRIGI_CONFIG_H_
#define _NEPOMUK_STRIGI_CONFIG_H_

#include <QtCore/QObject>

#include <kconfig.h>
#include <kio/global.h>


namespace Nepomuk {
    /**
     * Active config class which emits signals if the config
     * was changed, for example if the KCM saved the config file.
     */
    class Config : public QObject
    {
        Q_OBJECT

    public:
        ~Config();
        static Config* self();

        /**
         * The folders to search for files to analyze
         */
        QStringList folders() const;

        /**
         * The folders that should be excluded.
         * It is perfectly possible to include subfolders again.
         */
        QStringList excludeFolders() const;

        QStringList excludeFilters() const;
        QStringList includeFilters() const;

        bool showGui() const;
        void setShowGui( bool showGui );

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
        bool shouldFolderBeIndex( const QString& );

    Q_SIGNALS:
        void configChanged();

    private Q_SLOTS:
        void slotConfigDirty();

    private:
        Config();

        KConfig m_config;
    };
}

#endif
