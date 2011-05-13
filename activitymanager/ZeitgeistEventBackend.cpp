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

#include "config-features.h"

#ifndef HAVE_QZEITGEIST
    #warning "No QtZeitgeist, disabling desktop events processing"

#else // HAVE_QZEITGEIST

#include "ZeitgeistEventBackend.h"

#include <QtZeitgeist/QtZeitgeist>
#include <QtZeitgeist/Interpretation>
#include <QtZeitgeist/Manifestation>

static QString eventInterpretation(Event::Type type)
{
    switch (type) {
        case Event::Accessed:
            return QtZeitgeist::Interpretation::Event::ZGAccessEvent;

        case Event::Opened:
            return QtZeitgeist::Interpretation::Event::ZGAccessEvent;

        case Event::Modified:
            return QtZeitgeist::Interpretation::Event::ZGModifyEvent;

        case Event::Closed:
            return QtZeitgeist::Interpretation::Event::ZGLeaveEvent;

    }

    // shut up GCC
    return QString();
}

static QString eventManifestation(Event::Reason reason)
{
    switch (reason) {
        case Event::User:
            return QtZeitgeist::Manifestation::Event::ZGUserActivity;

        case Event::Heuristic:
            return QtZeitgeist::Manifestation::Event::ZGHeuristicActivity;

        case Event::Scheduled:
            return QtZeitgeist::Manifestation::Event::ZGScheduledActivity;

        case Event::System:
            return QtZeitgeist::Manifestation::Event::ZGSystemNotification;

        case Event::World:
            return QtZeitgeist::Manifestation::Event::ZGWorldActivity;
    }

    // shut up GCC
    return QtZeitgeist::Manifestation::Event::ZGUserActivity;
}

static QString applicationUri(const QString & application)
{
    // TODO: Make this correct
    return "app://" + application + ".desktop";
}

ZeitgeistEventBackend::ZeitgeistEventBackend()
{
    QtZeitgeist::init();
}

void ZeitgeistEventBackend::addEvents(const EventList & events)
{

}

#endif // HAVE_QZEITGEIST
