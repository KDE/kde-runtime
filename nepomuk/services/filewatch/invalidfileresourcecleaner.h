/*
   Copyright (C) 2010 Vishesh Handa <handa.vish@gmail.com>
   Copyright (C) 2010 by Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_INVALID_FILE_RESOURCE_CLEANER_H_
#define _NEPOMUK_INVALID_FILE_RESOURCE_CLEANER_H_

#include <QtCore/QThread>

namespace Nepomuk {
    /**
     * Once started cleans up file resource entries for
     * files that do not exist anymore.
     * This is something that might happen when the file indexer is
     * disabled and files have been removed while Nepomuk
     * was not running.
     *
     * The class deletes itself once finished.
     */
    class InvalidFileResourceCleaner : public QThread
    {
        Q_OBJECT

    public:
        /**
         * Creates a new instance but does not start the thread yet.
         * Use start() to actually start the cleanup.
         */
        InvalidFileResourceCleaner( QObject* parent = 0 );

        /**
         * Deleting the class is normally not necessary
         * since it will delete itself once finished. But
         * it can also be deleted manually beforehand
         * which will cancel the cleanup process gracefully.
         */
        ~InvalidFileResourceCleaner();

        /**
         * Start the cleaner thread with a base path, ie. only
         * check files under that optional path.
         */
        void start(const QString& basePath);

        using QThread::start;

    private:
        void run();

        bool m_stopped;
        QString m_basePath;
    };
}

#endif

