/*
 *   Copyright (C) 2011 Ivan Cukic <ivan.cukic@kde.org>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RANKINGS_H
#define RANKINGS_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

class Rankings: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.ActivityManager.Rankings")

public:
    static void init(QObject * parent = 0);
    static Rankings * self();

    ~Rankings();

public Q_SLOTS:
    /**
     * Registers a new client for the specified activity and application
     * @param client d-bus name
     * @param activity activity to track. If empty, uses the default activity.
     * @param application application to track. If empty, all applications are aggregated
     * @param resource type that the client is interested in
     * @note If the activity is left empty - the results are always related to the current activity,
     *     not the activity that was current when calling this method.
     */
    void registerClient(const QString & client,
            const QString & activity = QString(),
            const QString & application = QString(),
            const QString & resourceType = QString());

    void deregisterClient(const QString & client);

public:
    class QueryParams {
    public:
        QueryParams(
                const QString & _activity = QString(),
                const QString & _application = QString(),
                const QString & _resourceType = QString()
            )
            : activity(_activity), application(_application), resourceType(_resourceType)
        {
        }

        QString activity, application, resourceType;
    };

private Q_SLOTS:
    void setCurrentActivity(const QString & activityId);

private:
    Rankings(QObject * parent = 0);

    static Rankings * s_instance;


    QHash < QString, QueryParams > m_queryParamsForClient;
    QHash < QueryParams, QStringList > m_clientsForQueryParams;
};

#endif
