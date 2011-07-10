/*
 *   Copyright (C) 2011 Ivan Cukic ivan.cukic(at)kde.org
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "slc.h"
#include "slcadaptor.h"

#include <QDBusConnection>
#include <KDebug>
#include <KWindowSystem>
#include <KUrl>

#include "../../SharedInfo.h"

SlcPlugin::SlcPlugin(QObject * parent, const QVariantList & args)
    : EventBackend(parent), focussedWindow(0)
{
    Q_UNUSED(args)
    kDebug() << "We are in the SlcPlugin";

    QDBusConnection dbus = QDBusConnection::sessionBus();
    new SLCAdaptor(this);
    dbus.registerObject("/SLC", this);

    connect(KWindowSystem::self(), SIGNAL(activeWindowChanged(WId)),
            this, SLOT(activeWindowChanged(WId)));
}

SlcPlugin::~SlcPlugin()
{
}

void SlcPlugin::addEvents(const EventList & events)
{
    foreach (const Event & event, events) {
        switch (event.type) {
            case Event::FocussedIn:
            case Event::Opened:
                // kDebug() << "Event::FocussedIn" << focussedWindow << event.wid << event.uri;

                lastFocussedResource[event.wid] = event.uri;

                if (event.wid == focussedWindow) {
                    updateFocus(focussedWindow);
                }

                break;

            case Event::FocussedOut:
            case Event::Closed:
                // kDebug() << "Event::FocussedOut" << focussedWindow << event.wid << event.uri;

                if (lastFocussedResource[event.wid] == event.uri) {
                    lastFocussedResource[event.wid] = KUrl();
                }

                if (event.wid == focussedWindow) {
                    updateFocus();
                }

                break;

            default:
                // nothing
                break;
        }
    }
}

KUrl SlcPlugin::_focussedResourceURI()
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

QString SlcPlugin::focussedResourceURI()
{
    return _focussedResourceURI().url();
}

QString SlcPlugin::focussedResourceMimetype()
{
    return SharedInfo::self()->resources().contains(_focussedResourceURI()) ?
        SharedInfo::self()->resources()[_focussedResourceURI()].mimetype : QString();
}

void SlcPlugin::activeWindowChanged(WId wid)
{
    if (wid == focussedWindow) return;

    focussedWindow = wid;

    updateFocus(wid);
}

void SlcPlugin::updateFocus(WId wid)
{
    // kDebug() << "Updating focus for " << wid;

    if (wid == 0 || !SharedInfo::self()->windows().contains(wid)) {
        // kDebug() << "Clearing focus";
        emit focusChanged(QString(), QString());

    } else if (wid == focussedWindow) {
        // kDebug() << "It is the currently focussed window";
        emit focusChanged(focussedResourceURI(), SharedInfo::self()->resources()[_focussedResourceURI()].mimetype);

    }
}

KAMD_EXPORT_PLUGIN(SlcPlugin, "activitymanger_slc")
