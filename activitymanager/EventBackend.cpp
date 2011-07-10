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

#include "EventBackend.h"

class EventBackend::Private {
};

EventBackend::EventBackend(QObject * parent)
    : QObject(parent), d(new Private())
{
}

EventBackend::~EventBackend()
{
    delete d;
}

void EventBackend::addEvents(const EventList & events)
{
    Q_UNUSED(events)
}

void EventBackend::setResourceMimeType(const QString & uri, const QString & mimetype)
{
    Q_UNUSED(uri)
    Q_UNUSED(mimetype)
}

