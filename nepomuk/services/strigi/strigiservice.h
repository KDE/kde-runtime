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

#ifndef _NEPOMUK_STRIGI_SERVICE_H_
#define _NEPOMUK_STRIGI_SERVICE_H_

#include <Nepomuk/Service>
#include <QtCore/QTimer>


namespace Strigi {
    class IndexManager;
}

namespace Nepomuk {

    class IndexScheduler;

    /**
     * Service controlling the strigidaemon
     */
    class StrigiService : public Nepomuk::Service
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "org.kde.nepomuk.Strigi")

    public:
        StrigiService( QObject* parent = 0, const QList<QVariant>& args = QList<QVariant>() );
        ~StrigiService();

        //vHanda: Is this really required? I've removed all the code that uses it.
        IndexScheduler* indexScheduler() const { return m_indexScheduler; }

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
        void updateFolder( const QString& path, bool forced );

        /**
         * Update all folders configured to be indexed.
         */
        void updateAllFolders( bool forced );

        /**
         * Index a folder independant of its configuration status.
         */
        void indexFolder( const QString& path, bool forced );

        /**
         * Index a specific file
         */
        void indexFile( const QString& path );

        void analyzeResource( const QString& uri, uint mTime, const QByteArray& data );
        void analyzeResourceFromTempFileAndDeleteTempFile( const QString& uri, uint mTime, const QString& tmpFile );

    private Q_SLOTS:
        void finishInitialization();
        void updateWatches();
        void slotIdleTimerPause();
        void slotIdleTimerResume();

    private:
        void updateStrigiConfig();
        QString userStatusString( bool simple ) const;

        Strigi::IndexManager* m_indexManager;
        IndexScheduler* m_indexScheduler;
    };
}

#endif
