/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MERGETHREAD_H
#define MERGETHREAD_H

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QUrl>
#include <QtCore/QQueue>

#include "changelogmerger.h"

namespace Soprano {
    class Model;
}

namespace Nepomuk {

    class ResourceManager;
    class ChangeLog;

    class Merger : public QThread
    {
        Q_OBJECT

        Merger( QObject* parent = 0 );
        virtual ~Merger();

        void stop();

    public :
        static Merger* instance();

    public Q_SLOTS:
        int process( const Nepomuk::ChangeLog & logFile );

    Q_SIGNALS:
        void completed( int percent );
        void multipleMerge( const QString & uri, const QString & prop );

    protected:
        virtual void run();

    private:
        QQueue<ChangeLogMerger*> m_queue;
        QMutex m_queueMutex;
        QWaitCondition m_queueWaiter;

        bool m_stopped;

        QHash<int, ChangeLogMerger*> m_processes;
        QMutex m_processMutex;
    };

}
#endif // MERGETHREAD_H
