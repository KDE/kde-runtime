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

#ifndef EVENT_H_
#define EVENT_H_

/**
 *
 */
class Event {
public:
    enum Type {
        Accessed,   ///< resource was accessed, but we don't know for how long it will be open/used
        Opened,     ///< resource was opened
        Modified,   ///< previously opened resource was modified
        Closed      ///< previously opened resource was closed
    };

    enum Reason {
        User,
        Scheduled,
        Heuristic,
        System,
        World
    };

    Event(const QString & application, const QString & uri, Type type = Accessed, Reason reason = User);

public:
    QString application;
    QString uri;
    Type    type;
    Reason  reason;

};

typedef QList<Event> EventList;

#endif // EVENT_H_

