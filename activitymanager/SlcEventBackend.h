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

#ifndef SLC_EVENT_BACKEND_H_
#define SLC_EVENT_BACKEND_H_

#include "EventBackend.h"

#include <QObject>
#include <KUrl>

/**
 *
 */
class SlcEventBackend: public QObject, public EventBackend {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.ActivityManager.SLC")

public:
    SlcEventBackend();

    virtual void addEvents(const EventList & events);

private Q_SLOTS:
    void activeWindowChanged(WId windowId);

public Q_SLOTS:
    QString focussedResourceURI();
    QString focussedResourceMimetype();

Q_SIGNALS:
    void focusChanged(const QString & uri, const QString & mimetype);

private:
    void updateFocus(WId wid);

    WId focussedWindow;
    QHash < WId, KUrl > lastFocussedResource;
};

#endif // SLC_EVENT_BACKEND_H_

