/* This file is part of the KDE Project
   Copyright (c) 2009 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_METADATA_MOVER_H_
#define _NEPOMUK_METADATA_MOVER_H_

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QQueue>
#include <QtCore/QPair>

#include <KUrl>

namespace Soprano {
    class Model;
}

namespace Nepomuk {
    class MetadataMover : public QThread
    {
        Q_OBJECT

    public:
        MetadataMover( Soprano::Model* model, QObject* parent = 0 );
        ~MetadataMover();

    public Q_SLOTS:
        void moveFileMetadata( const KUrl& from, const KUrl& to );
        void removeFileMetadata( const KUrl& file );
        void removeFileMetadata( const KUrl::List& files );

        void stop();

    private:
        void run();

        /**
         * Remove the metadata for file \p url
         */
        void removeMetadata( const KUrl& url );

        /**
         * Recursively update the nie:url and nie:isPartOf properties
         * of the resource describing \p from.
         *
         * If old pre-KDE 4.4 file:/ resource URIs are used these are
         * updated to the new nepomuk:/res/<UUID> scheme
         */
        void updateMetadata( const KUrl& from, const KUrl& to );

        /**
         * Convert old pre-KDE 4.4 style file:/ resource URIs to the
         * new nepomuk:/res/<UUID> scheme.
         *
         * \return The new resource URI. This resource does not have
         * nie:url or nie:isPartOf properties yet.
         */
        QUrl updateLegacyMetadata( const QUrl& oldResourceUri );

        // if the second url is empty, just delete the metadata
        QQueue<QPair<KUrl, KUrl> > m_updateQueue;
        QMutex m_queueMutex;
        QWaitCondition m_queueWaiter;
        bool m_stopped;

        Soprano::Model* m_model;

        QUrl m_strigiParentUrlUri;
    };
}

#endif
