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
        switch (event.type) {
            case Event::FocussedIn:
            case Event::Opened:
                kDebug() << "Event::FocussedIn" << focussedWindow << event.wid << event.uri;

                lastFocussedResource[event.wid] = event.uri;

                if (event.wid == focussedWindow) {
                    updateFocus(focussedWindow);
                }

                break;

            case Event::FocussedOut:
            case Event::Closed:
                kDebug() << "Event::FocussedOut" << focussedWindow << event.wid << event.uri;

                if (lastFocussedResource[event.wid] == event.uri) {
                    lastFocussedResource[event.wid] = KUrl();
                }

                if (event.wid == focussedWindow) {
                    updateFocus();
                }

                break;

            default:
                // nothing
        }
    }
}

KUrl SlcEventBackend::_focussedResourceURI()
{
    KUrl kuri;

    if (lastFocussedResource.contains(focussedWindow)) {
        kuri = lastFocussedResource[focussedWindow];
    } else {
        foreach (const KUrl & uri, SharedInfo::self()->windows()[focussedWindow].resources) {
            kuri = uri;
            break;
        }
    }

    return kuri;
}

QString SlcEventBackend::focussedResourceURI()
{
    return _focussedResourceURI().url();
}

QString SlcEventBackend::focussedResourceMimetype()
{
    return SharedInfo::self()->resources().contains(_focussedResourceURI()) ?
        SharedInfo::self()->resources()[_focussedResourceURI()].mimetype : QString();
}

void SlcEventBackend::activeWindowChanged(WId wid)
{
    if (wid == focussedWindow) return;

    focussedWindow = wid;

    updateFocus(wid);
}

void SlcEventBackend::updateFocus(WId wid)
{
    kDebug() << "Updating focus for " << wid;

    if (wid == 0 || !SharedInfo::self()->windows().contains(wid)) {
        kDebug() << "Clearing focus";
        emit focusChanged(QString(), QString());

    } else if (wid == focussedWindow) {
        kDebug() << "It is the currently focussed window";
        emit focusChanged(focussedResourceURI(), SharedInfo::self()->resources()[_focussedResourceURI()].mimetype);

    }
}
