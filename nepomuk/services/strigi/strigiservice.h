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
#include <QtCore/QPoint>
#include <QtCore/QTimer>


namespace Strigi {
    class IndexManager;
}
class FileSystemWatcher;

namespace Nepomuk {

    class IndexScheduler;

    /**
     * Service controlling the strigidaemon
     */
    class StrigiService : public Nepomuk::Service
    {
        Q_OBJECT

    public:
        StrigiService( QObject* parent = 0, const QList<QVariant>& args = QList<QVariant>() );
        ~StrigiService();

        IndexScheduler* indexScheduler() const { return m_indexScheduler; }

        bool isSuspended() const;

    Q_SIGNALS:
        void statusStringChanged();

    public Q_SLOTS:
        /**
         * \return A user readable status string
         */
        QString userStatusString() const;
        void setSuspended( bool );

    private Q_SLOTS:
        void delayedInit();
        void updateWatches();
        void slotDirDirty( const QString& );
        void wakeUp();

    private:
        void updateStrigiConfig();

        Strigi::IndexManager* m_indexManager;
        IndexScheduler* m_indexScheduler;

        FileSystemWatcher* m_fsWatcher;

        QTimer* m_wakeUpTimer;
        QPoint m_cursorPos;

        // in case a suspend is requested before the service is initialized
        bool m_suspendRequested;

        // if the service is suspended internally because of a user interaction,
        // the user visible state should be shown as 'idle' and not as 'suspended'
        bool m_internallySuspended;
    };
}

#endif
