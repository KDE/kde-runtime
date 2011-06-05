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

#include "SlcEventBackend.h"
#include "slcadaptor.h"

#include <QDBusConnection>
#include <KDebug>
#include <KWindowSystem>
#include <KUrl>

#include "SharedInfo.h"

SlcEventBackend::SlcEventBackend()
    : focussedWindow(0)
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    new SLCAdaptor(this);
    dbus.registerObject("/SLC", this);

    connect(KWindowSystem::self(), SIGNAL(activeWindowChanged(WId)),
            this, SLOT(activeWindowChanged(WId)));
}

void SlcEventBackend::addEvents(const EventList & events)
{
    foreach (const Event & event, events) {
        if (event.type == Event::FocussedIn) {
            lastFocussedResource[event.wid] = event.uri;

        } else if (event.type == Event::FocussedOut) {
            lastFocussedResource[event.wid] = KUrl();

        }
    }
}

QString SlcEventBackend::focussedResourceURI()
{
    return "42";
}

QString SlcEventBackend::focussedResourceMimetype()
{
    return "text/answer";
}

void SlcEventBackend::activeWindowChanged(WId wid)
{
    updateFocus(wid);
}

void SlcEventBackend::updateFocus(WId wid)
{
    if (wid == focussedWindow) {
        KUrl kuri = lastFocussedResource[wid];

        if (kuri == KUrl()) {
            foreach (const KUrl & uri, SharedInfo::self()->windows()[wid].resources) {
                kuri = uri;
                break;
            }
        }

        emit focusChanged(kuri.url(), SharedInfo::self()->resources()[kuri].mimetype);
    }
}
