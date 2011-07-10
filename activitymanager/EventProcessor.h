/*
 *   Copyright (C) 2010 Ivan Cukic <ivan.cukic(at)kde.org>
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

#ifndef EVENT_PROCESSOR_H
#define EVENT_PROCESSOR_H

#include <QObject>
#include <QThread>

#include "Event.h"

class EventBackend;
class EventProcessorPrivate;

/**
 * Thread to process desktop/usage events
 */
class EventProcessor: public QObject {
public:
    static EventProcessor * self();

    virtual ~EventProcessor();

    void addEvent(const QString & application, WId wid, const QString & uri,
            Event::Type type = Event::Accessed, Event::Reason reason = Event::User);

private:
    EventProcessor();

    class EventProcessorPrivate * const d;
};

#endif // EVENT_PROCESSOR_H
