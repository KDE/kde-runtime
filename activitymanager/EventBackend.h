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

#ifndef EVENT_BACKEND_H_
#define EVENT_BACKEND_H_

#include <EventProcessor.h>
#include "Event.h"

#include <kdemacros.h>
#include <KPluginFactory>
#include <KPluginLoader>

#define KAMD_EXPORT_PLUGIN(ClassName, AboutData)                       \
    K_PLUGIN_FACTORY(ClassName##Factory, registerPlugin<ClassName>();) \
    K_EXPORT_PLUGIN(ClassName##Factory("AboutData"))


/**
 *
 */
class KDE_EXPORT EventBackend: public QObject {
    Q_OBJECT

public:
    EventBackend(QObject * parent);
    virtual ~EventBackend();

    virtual void addEvents(const EventList & events);
    virtual void setResourceMimeType(const QString & uri, const QString & mimetype);

private:
    class Private;
    Private * const d;
};

#endif // EVENT_BACKEND_H_

