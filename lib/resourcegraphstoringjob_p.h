/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

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

#ifndef RESOURCEGRAPHSTORINGJOB_P_H
#define RESOURCEGRAPHSTORINGJOB_P_H

#include <KJob>

#include <QtCore/QHash>
#include <QtCore/QUrl>


class QDBusPendingCallWatcher;
class KComponentData;

namespace Nepomuk {
class SimpleResourceGraph;

/**
 * A simple wrapper job around an async DBus call to storeResources().
 */
class ResourceGraphStoringJob : public KJob
{
    Q_OBJECT

public:
    /**
     * Will directly start the job.
     */
    ResourceGraphStoringJob(const Nepomuk::SimpleResourceGraph& graph, const QHash<QUrl, QVariant>& additionalMetadata, const KComponentData& cd);
    ~ResourceGraphStoringJob();

    void start();

private Q_SLOTS:
    void slotDBusCallFinished(QDBusPendingCallWatcher*);
};
}

#endif
