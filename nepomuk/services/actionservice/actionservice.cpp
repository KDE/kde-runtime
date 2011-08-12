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
#include <QtCore/QProcess>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>

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
        QString command = service->exec();
        command.replace(QLatin1String("%s"), subjectResources.join(QLatin1String(" ")));
        command.replace(QLatin1String("%o"), objectResources.join(QLatin1String(" ")));
        return QProcess::startDetached(command);
    }
    else if(!service->property(QLatin1String("X-Nepomuk-DBusCommand"), QVariant::String).toString().isEmpty()) {
        const QStringList commandTokens = service->property(QLatin1String("X-Nepomuk-DBusCommand"), QVariant::String).toString().split(QLatin1String(" "));
        if(commandTokens.count() < 4) {
            kDebug() << "Invalid DBus command:" << commandTokens;
            return false;
        }
        const QString dbusService = commandTokens[0];
        const QString dbusObject = commandTokens[1];
        const QString dbusInterface = commandTokens[2];
        const QString dbusMethod = commandTokens[3];

        QVariantList arguments;
        for(int i = 4; i < commandTokens.count(); ++i) {
            QString token = commandTokens[i].simplified();
            if(token == QLatin1String("%s")) {
                arguments << subjectResources;
            }
            else if(token == QLatin1String("%s")) {
                arguments << objectResources;
            }
            else {
                // FIXME: do we need to cast to the appropriate type?
                arguments << token;
            }
        }

        return QDBusInterface(dbusService,
                              dbusObject,
                              dbusInterface,
                              QDBusConnection::sessionBus())
                .callWithArgumentList(QDBus::Block,
                                      dbusMethod,
                                      arguments)
                .errorName().isEmpty();
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
        ResourceActionPlugin* plugin = service->createInstance<Nepomuk::ResourceActionPlugin>(this);
        if(plugin) {
            m_pluginCache.insert(service->desktopEntryName(), plugin);
        }
        return plugin;
    }
}

#include "actionservice.moc"
