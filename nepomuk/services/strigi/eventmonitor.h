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

#ifndef _NEPOMUK_STRIGI_EVENT_MONITOR_H_
#define _NEPOMUK_STRIGI_EVENT_MONITOR_H_

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>

class KDiskFreeSpace;

namespace Nepomuk {

    class IndexScheduler;

    class EventMonitor : public QObject
    {
        Q_OBJECT

    public:
        EventMonitor( IndexScheduler* scheduler, QObject* parent );
        ~EventMonitor();

    private Q_SLOTS:
        void slotPowerManagementStatusChanged( bool conserveResources );
        void slotCheckAvailableSpace();
        void slotIndexingStopped();
        void pauseIndexing(int pauseState);
        void resumeIndexing();
        void slotIndexingSuspended( bool suspended );
        void slotIndexingStateChanged( bool indexing );

    private:
        enum {
            NotPaused,
            PausedDueToPowerManagement,
            PausedDueToAvailSpace,
            PausedCustom
        };

        IndexScheduler* m_indexScheduler;
        int m_pauseState;

        // timer used to periodically check for available space
        QTimer m_availSpaceTimer;

        QDateTime m_indexingStartTime;
        int m_totalIndexingSeconds;
    };
}

#endif
