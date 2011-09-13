/* This file is part of the KDE Project
   Copyright (c) 2008-2010 Sebastian Trueg <trueg@kde.org>
   Copyright (c) 2010 Vishesh Handa <handa.vish@gmail.com>

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

#ifndef _NEPOMUK_STRIGI_SERVICE_H_
#define _NEPOMUK_STRIGI_SERVICE_H_

#include <Nepomuk/Service>
#include <QtCore/QTimer>
#include <QtCore/QThread>

namespace Strigi {
    class IndexManager;
}

namespace Nepomuk {

    class IndexScheduler;

    /**
     * Service controlling the strigidaemon
     */
    class FileIndexer : public Nepomuk::Service
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "org.kde.nepomuk.Strigi")

    public:
        FileIndexer( QObject* parent = 0, const QList<QVariant>& args = QList<QVariant>() );
        ~FileIndexer();

    Q_SIGNALS:
        void statusStringChanged();
        void statusChanged(); //vHanda: Can't we just use statusStringChanged? or should that be renamed
        void indexingFolder( const QString& path );
        void indexingStarted();
        void indexingStopped();

    public Q_SLOTS:
        /**
         * \return A user readable status string. Includes the currently indexed folder.
         */
        QString userStatusString() const;

        /**
         * Simplified status string without details.
         */
        QString simpleUserStatusString() const;

        bool isSuspended() const;
        bool isIndexing() const;

        void resume() const;
        void suspend() const;
        void setSuspended( bool );

        QString currentFolder() const;
        QString currentFile() const;

        /**
         * Update folder \a path if it is configured to be indexed.
         */
        void updateFolder( const QString& path, bool recursive, bool forced );

        /**
         * Update all folders configured to be indexed.
         */
        void updateAllFolders( bool forced );

        /**
         * Index a folder independent of its configuration status.
         */
        void indexFolder( const QString& path, bool recursive, bool forced );

        /**
         * Index a specific file
         */
        void indexFile( const QString& path );

    private Q_SLOTS:
        void finishInitialization();
        void updateWatches();
        void slotIdleTimeoutReached();
        void slotIdleTimerResume();

    private:
        void updateStrigiConfig();
        QString userStatusString( bool simple ) const;

        IndexScheduler* m_indexScheduler;
        QThread* m_schedulingThread;
    };
}

#endif
