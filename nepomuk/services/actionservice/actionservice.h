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

#ifndef ACTIONSERVICE_H
#define ACTIONSERVICE_H

#include <Nepomuk/Service>

#include <QtCore/QVariant>

#include <kservice.h>


namespace Nepomuk {
class ResourceActionPlugin;

class ActionService : public Nepomuk::Service
{
    Q_OBJECT
    Q_CLASSINFO( "D-Bus Interface", "org.kde.nepomuk.ResourceActions" )

public:
    ActionService(QObject *parent, const QVariantList& );
    ~ActionService();

public slots:
    /**
     * \return A list of desktop file names which identify the actions
     * that can be executed on the provided resources.
     */
    Q_SCRIPTABLE QStringList actionsForResources(const QStringList& subjectResources, const QStringList& objectResources);

    /**
     * \return A list of desktop file names which identify the actions
     * that can be executed on resources of type \p subjectType and \p objectType.
     *
     * \param subjectCount The number of subject resources of type \p subjectType.
     * A value of 0 refers to "many".
     * \param objectCount The number of object resources of type \p objectType.
     * A value of 0 refers to "many".
     */
    Q_SCRIPTABLE QStringList actionsForTypes(const QString& subjectType, const QString& objectType, int subjectCount, int objectCount);

    /**
     * Execute the action referred to by \p desktop file name on the provided resources.
     */
    Q_SCRIPTABLE bool executeAction(const QString& desktopFileName, const QStringList& subjectResources, const QStringList& objectResources);

private:
    KService::List actionServicesForTypes(const QString& subjectType, const QString& objectType, int subjectCount, int objectCount);
    KService::List actionServicesForResources(const QStringList& subjects, const QStringList& objects);
    ResourceActionPlugin* pluginFromCache(KService::Ptr service);

    QHash<QString, ResourceActionPlugin*> m_pluginCache;
};
}

#endif // ACTIONSERVICE_H
