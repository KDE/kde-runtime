/*
 *   Copyright (C) 2011 Ivan Cukic <ivan.cukic(at)kde.org>
 *   Copyright (c) 2011 Sebastian Trueg <trueg@kde.org>
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

#ifndef HAVE_NEPOMUK
    #warning "No Nepomuk, disabling desktop events processing"

#else // HAVE_NEPOMUK

#include "NepomukEventBackend.h"
#include "NepomukResourceScoreMaintainer.h"

#include "Event.h"
#include "ActivityManager.h"
#include "kext.h"

#include <nepomuk/resource.h>
#include <nepomuk/nuao.h>
#include <nepomuk/resourcemanager.h>
#include <nepomuk/variant.h>

#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/ResourceTerm>
#include <Nepomuk/Query/ResourceTypeTerm>
#include <Nepomuk/Query/ComparisonTerm>
#include <Nepomuk/Query/LiteralTerm>
#include <Nepomuk/Query/NegationTerm>

#include <Soprano/Vocabulary/NAO>
#include <Soprano/QueryResultIterator>
#include <Soprano/Node>
#include <Soprano/Model>

#include <kdebug.h>

#include "NepomukCommon.h"

using namespace Soprano::Vocabulary;
using namespace Nepomuk::Vocabulary;
using namespace Nepomuk::Query;

NepomukEventBackend::NepomukEventBackend()
{
}

void NepomukEventBackend::addEvents(const EventList & events)
{
    foreach (const Event& event, events) {
        if (event.type == Event::Accessed) {
            // one-shot event

            Nepomuk::Resource eventRes = createDesktopEvent(event.uri, event.timestamp, event.application);
            eventRes.addType(NUAO::UsageEvent());
            eventRes.addProperty(NUAO::end(), event.timestamp);

            kDebug() << "one-shot Accessed event" << eventRes;

        } else if (event.type == Event::Opened) {
            // create a new event
            createDesktopEvent(event.uri, event.timestamp, event.application);

        } else {
            // find the corresponding event
            // FIXME: enable this once the range of nao:identifier has been fixed and is no longer assumed to be rdfs:Resource
            // resulting in a wrong query.
//            Query query(ResourceTypeTerm(NUAO::DesktopEvent())
//                        && ComparisonTerm(NUAO::involves(),
//                                          ResourceTerm(Nepomuk::Resource(KUrl(event.uri))), ComparisonTerm::Equal)
//                        && ComparisonTerm(NUAO::involves(),
//                                          ResourceTypeTerm(NAO::Agent())
//                                          && ComparisonTerm(NAO::identifier(), LiteralTerm(event.application), ComparisonTerm::Equal))
//                        && !ComparisonTerm(NUAO::end(), Term()));
//            query.setLimit(1);
//            query.setQueryFlags(Query::NoResultRestrictions);
//            const QString query = query.toSparqlQuery();

            // TODO: Something strange is going on here - this should check for
            // the activity as well
            const QString query
                    = QString::fromLatin1("select ?r where { "
                                          "?r a nuao:DesktopEvent . "
                                          "?r %1 %2 . "
                                          "?r %3 %4 . "
                                          "OPTIONAL { ?r nuao:end ?d . } . "
                                          "FILTER(!BOUND(?d)) . } "
                                          "LIMIT 1")
                    .arg(
                        /* %1 */ resN3(NUAO_targettedResource),
                        /* %2 */ resN3(anyResource(KUrl(event.uri))),
                        /* %3 */ resN3(NUAO_initiatingAgent),
                        /* %4 */ resN3(agentResource(event.application))
                    );

            kDebug() << query;

            Soprano::QueryResultIterator it
                    = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(query, Soprano::Query::QueryLanguageSparql);

            if (it.next()) {
                Nepomuk::Resource eventRes(it[0].uri());
                it.close();

                eventRes.addProperty(NUAO::end(), event.timestamp);
                if (event.type == Event::Modified) {
                    eventRes.addType(NUAO::ModificationEvent());
                } else {
                    eventRes.addType(NUAO::UsageEvent());
                }

                // In case of a modification event we create a new event which will
                // be completed by the final Closed event since this one resource
                // modification is done now. It ended with saving the resource.
                if (event.type == Event::Modified) {
                    // create a new event
                    createDesktopEvent(event.uri, event.timestamp, event.application);
                }

            } else {
                kDebug() << "Failed to find matching Open event for resource" << event.uri << "and application" << event.application;
            }

            if (event.type == Event::Closed) {
                NepomukResourceScoreMaintainer::self()->processResource(event.uri, event.application);
            }
        }
    }
}

Nepomuk::Resource NepomukEventBackend::createDesktopEvent(const KUrl& uri, const QDateTime& startTime, const QString& app)
{
    // one-shot event
    Nepomuk::Resource eventRes(QUrl(), NUAO::DesktopEvent());
    eventRes.addProperty(NUAO_targettedResource, Nepomuk::Resource(uri));
    eventRes.addProperty(NUAO::start(), startTime);

    // the app
    Nepomuk::Resource appRes(app, NAO::Agent());
    eventRes.addProperty(NUAO_initiatingAgent, appRes);

    // the activity
    if (!m_currentActivity.isValid()
            || m_currentActivity.identifiers().isEmpty()
            || m_currentActivity.identifiers().first() != ActivityManager::self()->CurrentActivity()) {
        // update the current activity resource
        Soprano::QueryResultIterator it
                = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(QString::fromLatin1("select ?r where { ?r a %1 . ?r %2 %3 . } LIMIT 1")
                                                                                  .arg(Soprano::Node::resourceToN3(KExt::Activity()),
                                                                                       Soprano::Node::resourceToN3(KExt::activityIdentifier()),
                                                                                       Soprano::Node::literalToN3(ActivityManager::self()->CurrentActivity())),
                                                                                  Soprano::Query::QueryLanguageSparql);
        if (it.next()) {
            m_currentActivity = it[0].uri();
        } else {
            m_currentActivity = Nepomuk::Resource(ActivityManager::self()->CurrentActivity(), KExt::Activity());
            m_currentActivity.setProperty(KExt::activityIdentifier(), ActivityManager::self()->CurrentActivity());
        }
    }

    eventRes.setProperty(KExt::usedActivity(), m_currentActivity);

    return eventRes;
}

#endif // HAVE_NEPOMUK
