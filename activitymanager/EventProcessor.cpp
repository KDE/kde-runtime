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

#ifndef HAVE_QZEITGEIST
    #warning "No QtZeitgeist, disabling desktop events processing"
#endif

#include <QDateTime>
#include <QList>
#include <QMutex>

#include <KDebug>

#ifdef HAVE_QZEITGEIST
    #include <QtZeitgeist/QtZeitgeist>
    #include <QtZeitgeist/DataModel/Event>
    #include <QtZeitgeist/Manifestation>
    #include <QtZeitgeist/Interpretation>
#endif

#include <time.h>

class Event {
public:
    static QString eventInterpretation(EventProcessor::EventType type)
    {
        switch (type) {
            case EventProcessor::Accessed:
                return QtZeitgeist::Interpretation::Event::ZGAccessEvent;

            case EventProcessor::Opened:
                return QtZeitgeist::Interpretation::Event::ZGAccessEvent;

            case EventProcessor::Modified:
                return QtZeitgeist::Interpretation::Event::ZGModifyEvent;

            case EventProcessor::Closed:
                return QtZeitgeist::Interpretation::Event::ZGLeaveEvent;

        }
    }

    static QString eventManifestation(EventProcessor::EventReason reason)
    {
        switch (reason) {
            case EventProcessor::User:
                return QtZeitgeist::Manifestation::Event::ZGUserActivity;

            case EventProcessor::Heuristic:
                return QtZeitgeist::Manifestation::Event::ZGHeuristicActivity;

            case EventProcessor::Scheduled:
                return QtZeitgeist::Manifestation::Event::ZGScheduledActivity;

            case EventProcessor::System:
                return QtZeitgeist::Manifestation::Event::ZGSystemNotification;

            case EventProcessor::World:
                return QtZeitgeist::Manifestation::Event::ZGWorldActivity;
        }
    }

    static QString applicationUri(const QString & application)
    {
        return application;
    }

    Event(const QString & application, const QString & uri,
          EventProcessor::EventType type = EventProcessor::Accessed,
          EventProcessor::EventReason reason = EventProcessor::User
    )
       : _application(application), _uri(uri), _type(type), _reason(reason),
         _timestamp(QDateTime::currentDateTime())
    {
        Q_UNUSED(uri);

    }

    bool operator == (const Event & other)
    {
        return
            _application == other._application &&
            _uri == other._uri &&
            _type == other._type &&
            _reason == other._reason;
    }

    QString _application;
    QString _uri;

    EventProcessor::EventType _type;
    EventProcessor::EventReason _reason;
    QDateTime _timestamp;

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
            Q_UNUSED(event)
            // TODO: Do something ;)
            sleep(2);
        }
    }
}

EventProcessor * EventProcessor::self()
{
#ifdef HAVE_QZEITGEIST

    if (!EventProcessorPrivate::s_instance) {
        QtZeitgeist::init();

        EventProcessorPrivate::s_instance = new EventProcessor();
    }

    return EventProcessorPrivate::s_instance;

#else // not HAVE_QZEITGEIST

    return NULL;

#endif // HAVE_QZEITGEIST
}

EventProcessor::EventProcessor()
    : d(new EventProcessorPrivate())
{
}

EventProcessor::~EventProcessor()
{
    delete d;
}

void EventProcessor::_event(const QString & application, const QString & uri,
        EventType type, EventReason reason)
{
    Event newEvent(application, uri, type, reason);

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

void EventProcessor::addEvent(const QString & application, const QString & uri,
        EventType type, EventReason reason)
{
    // self() can return NULL if nepomuk is not available
    EventProcessor * ep = self();
    if (ep) {
        ep->_event(application, uri, type, reason);
    }
}

