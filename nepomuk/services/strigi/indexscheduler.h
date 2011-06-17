/* This file is part of the KDE Project
   Copyright (c) 2008-2010 Sebastian Trueg <trueg@kde.org>

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
#include <QtCore/QQueue>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>

#include <vector>
#include <string>

#include <KUrl>

class KJob;
class QFileInfo;
class QByteArray;

namespace Nepomuk {

    class IndexCleaner;
    /**
     * The IndexScheduler performs the normal indexing,
     * ie. the initial indexing and the timed updates
     * of all files.
     *
     * Events are not handled.
     */
    class IndexScheduler : public QObject
    {
        Q_OBJECT

    public:
        IndexScheduler( QObject* parent=0 );
        ~IndexScheduler();

        bool isSuspended() const;
        bool isIndexing() const;

        /**
         * The folder currently being indexed. Empty if not indexing.
         * If suspended the folder might still be set!
         */
        QString currentFolder() const;

        /**
         * The file currently being indexed. Empty if not indexing.
         * If suspended the file might still be set!
         *
         * This file should always be a child of currentFolder().
         */
        QString currentFile() const;

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

        enum IndexingSpeed {
            /**
             * Index at full speed, i.e. do not use any artificial
             * delays.
             *
             * This is the mode used if the user is "away".
             */
            FullSpeed = 0,

            /**
             * Reduce the indexing speed mildly. This is the normal
             * mode used while the user works. The indexer uses small
             * delay between indexing two files in order to keep the
             * load on CPU and IO down.
             */
            ReducedSpeed,

            /**
             * Like ReducedSpeed delays are used but they are much longer
             * to get even less CPU and IO load. This mode is used for the
             * first 2 minutes after startup to give the KDE session manager
             * time to start up the KDE session rapidly.
             */
            SnailPace
        };

        IndexingSpeed currentSpeed() const { return m_speed; }

    public Q_SLOTS:
        void suspend();
        void resume();

        void setIndexingSpeed( IndexingSpeed speed );

        /**
         * A convenience slot which calls setIndexingSpeed
         * with either FullSpeed or ReducedSpeed, based on the
         * value of \p reduced.
         */
        void setReducedIndexingSpeed( bool reduced = false );

        void setSuspended( bool );

        /**
         * Slot to connect to certain event systems like KDirNotify
         * or KDirWatch
         *
         * Updates a complete folder. Makes sense for
         * signals like KDirWatch::dirty.
         *
         * \param path The folder to update
         * \param flags Additional flags, all except AutoUpdateFolder are supported. This
         * also means that by default \p path is updated non-recursively.
         */
        void updateDir( const QString& path, UpdateDirFlags flags = NoUpdateFlags );

        /**
         * Updates all configured folders.
         */
        void updateAll( bool forceUpdate = false );

        /**
         * Analyze the one file without conditions.
         */
        void analyzeFile( const QString& path );

    Q_SIGNALS:
        void indexingStarted();
        void indexingStopped();

        /// a combination of the two signals above
        void indexingStateChanged( bool indexing );
        void indexingFolder( const QString& );
        void indexingFile( const QString & );
        void indexingSuspended( bool suspended );

    private Q_SLOTS:
        void slotConfigChanged();
        void slotCleaningDone();
        void slotIndexingDone( KJob* job );
        void doIndexing();

    private:
        void run();

        /**
         * It first indexes \p dir. Then it indexes all the files in \p dir, and
         * finally recursivly analyzes all the subfolders in \p dir IF \p flags
         * contains the 'UpdateRecursive' flag. It even sets m_currentFolder
         *
         * Returns true if the folder was analyzed
         */
        bool analyzeDir( const QString& dir, UpdateDirFlags flags );

        void queueAllFoldersForUpdate( bool forceUpdate = false );

        /**
         * Deletes all indexed information about entries and all subfolders and files
         * from the store
         */
        void deleteEntries( const QStringList& entries );

        // emits indexingStarted or indexingStopped based on parameter. Makes sure
        // no signal is emitted twice
        void setIndexingStarted( bool started );

        void callDoIndexing();

        bool m_suspended;
        bool m_indexing;

        QMutex m_resumeStopMutex;

        // A specialized queue that gives priority to dirs that do not use the AutoUpdateFolder flag.
        class UpdateDirQueue : public QQueue<QPair<QString, UpdateDirFlags> >
        {
        public:
            void enqueueDir( const QString& dir, UpdateDirFlags flags );
            void prependDir( const QString& dir, UpdateDirFlags flags );
            void clearByFlags( UpdateDirFlags mask );
        };
        // queue of folders to update (+flags defined in the source file) - changed by updateDir
        UpdateDirQueue m_dirsToUpdate;

        QQueue<QFileInfo> m_filesToUpdate;

        QMutex m_dirsToUpdateMutex;

        QString m_currentFolder;
        KUrl m_currentUrl;

        IndexingSpeed m_speed;
        IndexCleaner* m_cleaner;
    };

    QDebug operator<<( QDebug dbg, IndexScheduler::IndexingSpeed speed );
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Nepomuk::IndexScheduler::UpdateDirFlags)

#endif
