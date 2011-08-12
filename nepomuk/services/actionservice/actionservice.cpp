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

#include "actionservice.h"
#include "resourceactionplugin.h"

#include <KPluginFactory>
#include <kservice.h>
#include <kservicetypetrader.h>
#include <kurl.h>
#include <kdebug.h>

#include <QtCore/QStringList>
#include <QtCore/QSet>

// TODO: make all async

NEPOMUK_EXPORT_SERVICE( Nepomuk::ActionService, "nepomukactionservice")

Nepomuk::ActionService::ActionService(QObject *parent, const QVariantList& )
    : Nepomuk::Service(parent)
{
}

Nepomuk::ActionService::~ActionService()
{
}

QStringList Nepomuk::ActionService::actionsForResources(const QStringList &subjectResources, const QStringList &objectResources)
{
    QSet<QString> actionIds;
    QStringList desktopFilesNames;
    foreach(KService::Ptr service, actionServicesForResources(subjectResources, objectResources)) {
        if(!service->library().isEmpty()) {
            //
            // Check if the plugin can handle the resources
            //
            ResourceActionPlugin* plugin = pluginFromCache(service);
            if(!plugin ||
               actionIds.contains(service->name()) ||
               !plugin->canExecuteActionFor(service->name(), KUrl::List(subjectResources), KUrl::List(objectResources))) {
                continue;
            }
        }

        if(!actionIds.contains(service->name())) {
            actionIds << service->name();
            desktopFilesNames << service->desktopEntryName();
        }
    }
    return desktopFilesNames;
}

QStringList Nepomuk::ActionService::actionsForTypes(const QString &subjectType, const QString &objectType, int subjectCount, int objectCount)
{
    QSet<QString> actionIds;
    QStringList desktopFilesNames;
    foreach(const KService::Ptr& service, actionServicesForTypes(subjectType, objectType, subjectCount, objectCount)) {
        // FIXME: determine the preferred service for the action
        if(!actionIds.contains(service->name())) {
            actionIds << service->name();
            desktopFilesNames << service->desktopEntryName();
        }
    }
    return desktopFilesNames;
}

bool Nepomuk::ActionService::executeAction(const QString &actionId, const QStringList &subjectResources, const QStringList &objectResources)
{
    KService::Ptr service = KService::serviceByDesktopName(actionId);
    if(!service->library().isEmpty()) {
        ResourceActionPlugin* plugin = pluginFromCache(service);
        if(plugin) {
            return plugin->executeAction(actionId, KUrl::List(subjectResources), KUrl::List(objectResources));
        }
        else {
            kDebug() << "Failed to load plugin" << service->library();
            return false;
        }
    }
    else if(!service->exec().isEmpty()) {
        // TODO: extend KRun? use our own code?
        return false;
    }
    else if(!service->property(QLatin1String("X-Nepomuk-DBusCommand"), QVariant::String).toString().isEmpty()) {
        // TODO
        return false;
    }
    else {
        kDebug() << "Invalid action service description" << service->desktopEntryName();
        return false;
    }
}

KService::List Nepomuk::ActionService::actionServicesForTypes(const QString &subjectType, const QString &objectType, int subjectCount, int objectCount)
{
    // FIXME: handle super-types, prefer actions on more specific types
    QStringList constraints;
    if(!subjectType.isEmpty()) {
        constraints << QString::fromLatin1("( '*' in [X-Nepomuk-SubjectTypes] or '%1' in [X-Nepomuk-SubjectTypes] )").arg(subjectType);
        if(subjectCount != 1) {
            if(subjectCount != 0) {
                constraints << QString::fromLatin1("( [X-Nepomuk-SubjectCardinality] == %1 or [X-Nepomuk-SubjectCardinality] == 0 )").arg(subjectCount);
            }
            else {
                constraints << QString::fromLatin1("( [X-Nepomuk-SubjectCardinality] == 0 )");
            }
        }
    }
    if(!objectType.isEmpty()) {
        constraints << QString::fromLatin1("( '*' in [X-Nepomuk-ObjectTypes] or '%1' in [X-Nepomuk-ObjectTypes] )").arg(objectType);
        if(objectCount != 1) {
            if(objectCount != 0) {
                constraints << QString::fromLatin1("( [X-Nepomuk-ObjectCardinality] == %1 or [X-Nepomuk-ObjectCardinality] == 0 )").arg(objectCount);
            }
            else {
                constraints << QString::fromLatin1("( [X-Nepomuk-ObjectCardinality] == 0 )");
            }
        }
    }

    return KServiceTypeTrader::self()->query(QLatin1String("Nepomuk/ActionPlugin"), constraints.join(QLatin1String(" and ")));
}

KService::List Nepomuk::ActionService::actionServicesForResources(const QStringList &subjects, const QStringList &objects)
{
    // TODO
    return KService::List();
}

Nepomuk::ResourceActionPlugin * Nepomuk::ActionService::pluginFromCache(KService::Ptr service)
{
    if(m_pluginCache.contains(service->desktopEntryName())) {
        return m_pluginCache[service->desktopEntryName()];
    }
    else {
        ResourceActionPlugin* plugin = service->createInstance<Nepomuk::ResourceActionPlugin>();
        if(plugin) {
            m_pluginCache.insert(service->desktopEntryName(), plugin);
        }
        return plugin;
    }
}

#include "actionservice.moc"
