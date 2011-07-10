/*
 *   Copyright (C) 2011 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "EventBackendLoader.h"
#include "EventBackend.h"

#include <KServiceTypeTrader>
#include <KDebug>

EventBackendLoader::EventBackendLoader(QObject * parent)
    : QObject(parent)
{
}

EventBackendLoader::~EventBackendLoader()
{
}

void EventBackendLoader::loadPlugins()
{
    kDebug() << "Loading plugins...";

    KService::List offers = KServiceTypeTrader::self()->query("ActivityManager/Plugin");

    foreach(const KService::Ptr & service, offers) {
        KPluginFactory * factory = KPluginLoader(service->library()).factory();

        if (!factory) {
            kDebug() << "Failed to load plugin:" << service->name();
            continue;
        }

        EventBackend * plugin = factory->create < EventBackend > (this);

        if (plugin) {
            // emit pluginLoaded(plugin);
        } else {
            kDebug() << "Failed to load plugin:" << service->name();
        }

    }
}
