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

#include "NepomukResourceScoreCache.h"

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>

#include <KDebug>

#include "kext.h"
#include "nao.h"
#include "nuao.h"

#include "ActivityManager.h"

using namespace Nepomuk::Vocabulary;

/**
 *
 */
class NepomukResourceScoreCachePrivate {
public:
    Nepomuk::Resource self;

};

NepomukResourceScoreCache::NepomukResourceScoreCache(const QString & activity, const QString & application, const QUrl & resource)
    : d(new NepomukResourceScoreCachePrivate())
{
    const QString query
        = QString::fromLatin1("select ?r where { "
                                  "?r a %1 . "
                                  "?r kext:involvesActivity %2 . "
                                  "?r kext:involvesAgent %3 . "
                                  "?r kext:involvesResource %4 . "
                                  "} LIMIT 1"
            ).arg(
                /* %1 */ Soprano::Node::resourceToN3(KExt::ResourceScoreCache()),
                /* %2 */ Soprano::Node::literalToN3(activity),
                /* %3 */ Soprano::Node::literalToN3(application),
                /* %4 */ Soprano::Node::resourceToN3(Nepomuk::Resource(KUrl(resource)).resourceUri())
            );

    kDebug() << query;

    Soprano::QueryResultIterator it
        = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(query, Soprano::Query::QueryLanguageSparql);

    if (it.next()) {
        Nepomuk::Resource result(it[0].uri());
        it.close();

        d->self = result;

        kDebug() << "Found an old cache" << d->self.resourceUri() << d->self.resourceType();

    } else {
        Nepomuk::Resource result(QUrl(), KExt::ResourceScoreCache());

        result.setProperty(
                KExt::targettedResource(),
                Nepomuk::Resource(resource)
            );
        result.setProperty(
                KExt::initiatingAgent(),
                Nepomuk::Resource(application, NAO::Agent())
            );
        result.setProperty(
                KExt::involvesActivity(),
                Nepomuk::Resource(ActivityManager::self()->CurrentActivity(), KExt::Activity())
            );
        result.setProperty(KExt::cachedScore(), 0);

        d->self = resource;

        kDebug() << "Created a new cache resource" << d->self.resourceUri() << d->self.resourceType();

    }
}

NepomukResourceScoreCache::~NepomukResourceScoreCache()
{
    delete d;
}

void NepomukResourceScoreCache::updateScore()
{
    kDebug() << "Last modified as string" << d->self.property(NAO::lastModified());
    QDateTime lastModified = d->self.property(NAO::lastModified()).toDateTime();

    kDebug() << "Last modified timestamp is" << lastModified << lastModified.isValid();

    const QString query
        = QString::fromLatin1("select ?r where { "
                                  "?r a %1 . "
                                  "?r %2 ?end . "
                                  "FILTER(?end > %3) ."
                                  " } "
            ).arg(
                /* %1 */ Soprano::Node::resourceToN3(NUAO::DesktopEvent()),
                /* %2 */ Soprano::Node::resourceToN3(NUAO::end()),
                /* %3 */ Soprano::Node::literalToN3(lastModified)
            );

    kDebug() << query;

    Soprano::QueryResultIterator it
        = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(query, Soprano::Query::QueryLanguageSparql);

    while (it.next()) {
        Nepomuk::Resource result(it[0].uri());

        kDebug() << result.resourceUri();
    }
}
