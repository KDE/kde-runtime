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

#ifndef _NEPOMUK_STRIGI_INDEX_SCHEDULER_H_
#define _NEPOMUK_STRIGI_INDEX_SCHEDULER_H_

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QSet>
#include <QtCore/QDateTime>

#include <vector>
#include <string>

namespace Strigi {
    class StreamAnalyzer;
    class IndexManager;
}

class StoppableConfiguration;
class QFileInfo;
class QUrl;
class KUrl;
class QByteArray;


namespace Nepomuk {
    /**
     * The IndexScheduler performs the normal indexing,
     * ie. the initial indexing and the timed updates
     * of all files.
     *
     * Events are not handled.
     */
    class IndexScheduler : public QThread
    {
        Q_OBJECT

    public:
        IndexScheduler( Strigi::IndexManager* manager, QObject* parent );
        ~IndexScheduler();

        bool isSuspended() const;
        bool isIndexing() const;

        /**
         * The folder currently being indexed. Empty if not indexing.
         * If suspended the folder might still be set!
         */
        QString currentFolder() const;

        enum UpdateDirFlag {
            /**
             * No flags, only used to make code more readable
             */
            NoUpdateFlags = 0x0,

            /**
             * The folder should be updated recursive
             */
            UpdateRecursive = 0x1,

            /**
             * The folder has been scheduled to update by the
             * update system, not by a call to updateDir
             */
            AutoUpdateFolder = 0x2,

            /**
             * The files in the folder should be updated regardless
             * of their state.
             */
            ForceUpdate = 0x4
        };
        Q_DECLARE_FLAGS( UpdateDirFlags, UpdateDirFlag )

    public Q_SLOTS:
        void suspend();
        void resume();
        void stop();
        void restart();

        void setSuspended( bool );

        /**
         * Slot to connect to certain event systems like KDirNotify
         * or KDirWatch
         *
         * Updates a complete folder (non-recursively). Makes sense for
         * signals like KDirWatch::dirty.
         */
        void updateDir( const QString& path, bool forceUpdate = false );

        /**
         * Updates all configured folders.
         */
        void updateAll( bool forceUpdate = false );

        /**
         * Analyze a resource that is not read from the local harddisk.
         *
         * \param uri The resource URI to identify the resource.
         * \param modificationTime The modification date of the resource. Used to determine if
         *        an actual update is necessary.
         * \data The data to analyze, ie. the contents of the resource.
         */
        void analyzeResource( const QUrl& uri, const QDateTime& modificationTime, QDataStream& data );

    Q_SIGNALS:
        void indexingStarted();
        void indexingStopped();
        void indexingFolder( const QString& );

    private Q_SLOTS:
        void readConfig();
        void slotConfigChanged();

    private:
        void run();

        bool waitForContinue();
        bool updateDir( const QString& dir, Strigi::StreamAnalyzer* analyzer, UpdateDirFlags flags );
        void analyzeFile( const QFileInfo& file, Strigi::StreamAnalyzer* analyzer );

        /**
         * Deletes all indexed information about entries and all subfolders and files
         * from the store
         */
        void deleteEntries( const std::vector<std::string>& entries );

        // emits indexingStarted or indexingStopped based on parameter. Makes sure
        // no signal is emitted twice
        void setIndexingStarted( bool started );

        /**
         * Removes all previously indexed entries that are not in the list of folders
         * to index anymore.
         */
        void removeOldAndUnwantedEntries();

        bool m_suspended;
        bool m_stopped;
        bool m_indexing;

        QMutex m_resumeStopMutex;
        QWaitCondition m_resumeStopWc;

        StoppableConfiguration* m_analyzerConfig;
        Strigi::IndexManager* m_indexManager;

        // set of folders to update (+flags defined in the source file) - changed by updateDir
        QSet<QPair<QString, UpdateDirFlags> > m_dirsToUpdate;

        QMutex m_dirsToUpdateMutex;
        QWaitCondition m_dirsToUpdateWc;

        QString m_currentFolder;
    };
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Nepomuk::IndexScheduler::UpdateDirFlags)

#endif
