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

    public:
        StrigiService( QObject* parent = 0, const QList<QVariant>& args = QList<QVariant>() );
        ~StrigiService();

        IndexScheduler* indexScheduler() const { return m_indexScheduler; }

    Q_SIGNALS:
        void statusStringChanged();

    public Q_SLOTS:
        /**
         * \return A user readable status string
         */
        QString userStatusString() const;
        bool isIdle() const;
        void setSuspended( bool );
        bool isSuspended() const;

    private Q_SLOTS:
        void finishInitialization();
        void updateWatches();

    private:
        void updateStrigiConfig();

        Strigi::IndexManager* m_indexManager;
        IndexScheduler* m_indexScheduler;
    };
}

#endif
