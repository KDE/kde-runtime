/*
 *   Copyright (C) 2010 Ivan Cukic <ivan.cukic(at)kde.org>
 *   Copyright (c) 2010 Sebastian Trueg <trueg@kde.org>
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

#include "EventProcessor.h"
#include "config-features.h"

#ifndef HAVE_NEPOMUK
    #warning "No Nepomuk, disabling desktop events processing"
#endif

#include <QList>
#include <QMutex>

#include <KDebug>

#include <Nepomuk/ResourceManager>

#include <time.h>

class Event {
public:
    Event(const QString & application, const QString & uri, EventProcessor::EventType type, time_t timestamp)
        : _application(application), _uri(uri), _type(type), _timestamp(timestamp)
    {
    }

    bool operator == (const Event & event)
    {
        // we don't want the timestamp here
        return
            (_type == event._type) &&
            (_application == event._application) &&
            (_uri == event._uri);
    }

    QString _application;
    QString _uri;
    EventProcessor::EventType _type;
    time_t _timestamp;
};

class EventProcessorPrivate: public QThread {
public:
    QList < Event > events;
    QMutex events_mutex;

    void run();

    static EventProcessor * s_instance;

};

EventProcessor * EventProcessorPrivate::s_instance = NULL;

void EventProcessorPrivate::run()
{
    kDebug() << "starting event processing...";

    forever {
        // initial delay before processing the events
        // sleep(5); // do we need it?

        EventProcessorPrivate::events_mutex.lock();

        if (events.count() == 0) {
            kDebug() << "No more events to process, exiting.";

            EventProcessorPrivate::events_mutex.unlock();
            return;
        }

        QList < Event > currentEvents = events;
        events.clear();

        EventProcessorPrivate::events_mutex.unlock();

        foreach (const Event & event, currentEvents) {
            kDebug() << "processing"
                     << event._application
                     << event._uri;

            sleep(2);
        }
    }
}

EventProcessor * EventProcessor::self()
{
#ifdef HAVE_NEPOMUK

    if (!Nepomuk::ResourceManager::instance()->initialized()) {
        return NULL;
    }

    if (!EventProcessorPrivate::s_instance) {
        EventProcessorPrivate::s_instance = new EventProcessor();
    }

    return EventProcessorPrivate::s_instance;

#else // not HAVE_NEPOMUK

    return NULL;

#endif // HAVE_NEPOMUK
}

EventProcessor::EventProcessor()
    : d(new EventProcessorPrivate())
{
}

EventProcessor::~EventProcessor()
{
    delete d;
}

void EventProcessor::_event(const QString & application, const QString & uri, EventType type)
{
    Event newEvent(application, uri, type, time(0));

    d->events_mutex.lock();

    foreach (const Event & event, d->events) {
        if (event._type == Accessed && event._uri == uri
                && event._application == application) {
            // Accessed events are of a lower priority
            // then the other ones
            if (type == Accessed) {
                d->events.removeAll(newEvent);
            }
        }
    }

    d->events.append(newEvent);

    d->events_mutex.unlock();

    d->start();
}

void EventProcessor::addEvent(const QString & application, const QString & uri, EventType type)
{
    // self() can return NULL if nepomuk is not available
    EventProcessor * ep = self();
    if (ep) {
        ep->_event(application, uri, type);
    }
}

