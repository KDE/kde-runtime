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

#include "NepomukResourceScoreMaintainer.h"

#include <QList>
#include <QMutex>
#include <QThread>

#include <KDebug>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>

#include <Soprano/QueryResultIterator>
#include <Soprano/Node>
#include <Soprano/Model>

#include <time.h>
#include "kext.h"

#include "ActivityManager.h"

class NepomukResourceScoreMaintainerPrivate: public QThread {
public:
    typedef QString ApplicationName;
    typedef QString ActivityName;
    typedef QList < QUrl > ResourceList;

    typedef QMap < ApplicationName, ResourceList > Applications;

    QMap < ActivityName, Applications > openResources;

    QList < Nepomuk::Resource > resources;
    QMutex resources_mutex;

    void run();

    static NepomukResourceScoreMaintainer * s_instance;

    Nepomuk::Resource stats(const Nepomuk::Resource & resource) const;

};

NepomukResourceScoreMaintainer * NepomukResourceScoreMaintainerPrivate::s_instance = NULL;

Nepomuk::Resource NepomukResourceScoreMaintainerPrivate::stats(const Nepomuk::Resource & resource) const
{
    const QString query
        = QString::fromLatin1("select ?r where { "
                                  "?r a %1 . "
                                  "?r nuao:involvedResource %2 . "
                                  "LIMIT 1")
            .arg(
                Soprano::Node::resourceToN3(Nepomuk::Vocabulary::KExt::ResourceScoreCache()),
                Soprano::Node::resourceToN3(resource.resourceUri())
            );

    Soprano::QueryResultIterator it
        = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(query, Soprano::Query::QueryLanguageSparql);

    if (it.next()) {
        Nepomuk::Resource result(it[0].uri());
        it.close();

        return result;
    }

    Nepomuk::Resource result(QUrl(), Nepomuk::Vocabulary::KExt::ResourceScoreCache());

    result.setProperty(Nepomuk::Vocabulary::KExt::involvedResource(), resource);
    result.setProperty(Nepomuk::Vocabulary::KExt::cachedScore(), 0);

    return resource;
}

void NepomukResourceScoreMaintainerPrivate::run()
{
    forever {
        // initial delay before processing the resources
        sleep(5);

        NepomukResourceScoreMaintainerPrivate::resources_mutex.lock();

        if (resources.count() == 0) {
            kDebug() << "No more resources to process, exiting.";

            NepomukResourceScoreMaintainerPrivate::resources_mutex.unlock();
            return;
        }

        QList < Nepomuk::Resource > currentResources = resources;
        resources.clear();

        NepomukResourceScoreMaintainerPrivate::resources_mutex.unlock();
    }
}

NepomukResourceScoreMaintainer * NepomukResourceScoreMaintainer::self()
{
    if (!NepomukResourceScoreMaintainerPrivate::s_instance) {
        NepomukResourceScoreMaintainerPrivate::s_instance = new NepomukResourceScoreMaintainer();
    }

    return NepomukResourceScoreMaintainerPrivate::s_instance;
}

NepomukResourceScoreMaintainer::NepomukResourceScoreMaintainer()
    : d(new NepomukResourceScoreMaintainerPrivate())
{
}

NepomukResourceScoreMaintainer::~NepomukResourceScoreMaintainer()
{
    delete d;
}

void NepomukResourceScoreMaintainer::processResource(const KUrl & resource, const QString & application)
{
    d->resources_mutex.lock();

    // Checking whether the item is already scheduled for
    // processing

    const QString & activity = ActivityManager::self()->CurrentActivity();

    if (d->openResources.contains(activity) &&
            d->openResources[activity].contains(application) &&
            d->openResources[activity][application].contains(resource)) {
        return;
    }

    d->openResources[activity][application] << resource;

    d->resources_mutex.unlock();

    d->start();
}


