/*
 *   Copyright (C) 2011 Trever Fischer <tdfischer@fedoraproject.org>
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

#include "ZeitgeistPlugin.h"

#include <QtZeitgeist/QtZeitgeist>
#include <QtZeitgeist/Log>
#include <QtZeitgeist/DataModel/Event>
#include <QtZeitgeist/Interpretation>
#include <QtZeitgeist/Manifestation>

#include <KDE/KMimeType>
#include <KDE/KDebug>
#include <KDE/KUrl>

ZeitgeistPlugin *ZeitgeistPlugin::s_instance = NULL;

ZeitgeistPlugin::ZeitgeistPlugin(QObject *parent, const QVariantList &args)
    : Plugin(parent)
{
    Q_UNUSED(args);
    QtZeitgeist::init();
    s_instance = this;
    m_log = new QtZeitgeist::Log(this);
    kDebug() << "Loaded zeitgeist plugin";
}

ZeitgeistPlugin::~ZeitgeistPlugin()
{
}

ZeitgeistPlugin *ZeitgeistPlugin::self()
{
    return s_instance;
}

void ZeitgeistPlugin::addEvents(const EventList &events)
{
    QtZeitgeist::DataModel::EventList zgEvents;
    foreach (const Event &event, events) {
        QtZeitgeist::DataModel::Event zgEvent;
        switch (event.type) {
            case Event::Accessed:
            case Event::Opened:
            case Event::FocussedIn:
                zgEvent.setInterpretation(QtZeitgeist::Interpretation::Event::ZGAccessEvent);
                break;
            case Event::Modified:
                zgEvent.setInterpretation(QtZeitgeist::Interpretation::Event::ZGModifyEvent);
                break;
            case Event::Closed:
            case Event::FocussedOut:
                zgEvent.setInterpretation(QtZeitgeist::Interpretation::Event::ZGLeaveEvent);
                break;
        }
        switch (event.reason) {
            case Event::User:
                zgEvent.setManifestation(QtZeitgeist::Manifestation::Event::ZGUserActivity);
                break;
            case Event::Scheduled:
                zgEvent.setManifestation(QtZeitgeist::Manifestation::Event::ZGScheduledActivity);
                break;
            case Event::Heuristic:
                zgEvent.setManifestation(QtZeitgeist::Manifestation::Event::ZGHeuristicActivity);
                break;
            case Event::System:
                zgEvent.setManifestation(QtZeitgeist::Manifestation::Event::ZGSystemNotification);
                break;
            case Event::World:
                zgEvent.setManifestation(QtZeitgeist::Manifestation::Event::ZGWorldActivity);
                break;
        }
        zgEvent.setActor("app://"+event.application);
        QtZeitgeist::DataModel::Subject subject;
        subject.setUri(event.uri);

        KUrl url(event.uri);
        KMimeType::Ptr type = KMimeType::findByUrl(url);
        subject.setMimeType(type->name());

        if (url.protocol() == "file") {
            subject.setManifestation(QtZeitgeist::Manifestation::Subject::NFOFileDataObject);
        } else {
            subject.setManifestation(QtZeitgeist::Manifestation::Subject::NFORemoteDataObject);
        }

        QString interpretation;
        if (type->name().startsWith("audio/")) {
            interpretation = QtZeitgeist::Interpretation::Subject::NFOAudio;
        } else if (type->name().startsWith("video/")) {
            interpretation = QtZeitgeist::Interpretation::Subject::NFOVideo;
        } else if (type->name().startsWith("image/")) {
            interpretation = QtZeitgeist::Interpretation::Subject::NFOImage;
        } else if (type->name() == "text/plain") {
            interpretation = QtZeitgeist::Interpretation::Subject::NFOPlainTextDocument;
        } else {
            kDebug() << "Don't know how to interpret" << type->name();
        }
        subject.setInterpretation(interpretation);

        zgEvent.addSubject(subject);
        zgEvents << zgEvent;
    }
    m_log->insertEvents(zgEvents);
}

KAMD_EXPORT_PLUGIN(ZeitgeistPlugin, "activitymanager_plugin_zeitgeist")
