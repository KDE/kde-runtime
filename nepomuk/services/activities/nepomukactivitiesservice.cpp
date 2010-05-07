/*
 * Copyright (c) 2010 Ivan Cukic <ivan.cukic(at)kde.org>
 * Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "nepomukactivitiesservice.h"
#include "activitiesserviceadaptor.h"

#include <QUuid>
#include <QDebug>

#include <KPluginFactory>
#include <KDebug>
#include <KConfig>
#include <KStandardDirs>

#include <Soprano/Model>
#include <Soprano/StatementIterator>
#include <Soprano/Statement>
#include <Soprano/Node>
#include <Soprano/NodeIterator>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/RDF>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>
#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/AndTerm>
#include <Nepomuk/Query/OrTerm>
#include <Nepomuk/Query/ComparisonTerm>
#include <Nepomuk/Query/ResourceTypeTerm>
#include <Nepomuk/Query/ResourceTerm>
#include <Nepomuk/Query/LiteralTerm>

#include "nie.h"

#define URL_ACTIVITY_TYPE "http://www.kde.org/ontologies/activities#Activity"

/**
 * this macro creates a service factory which can then be found by the Qt/KDE
 * plugin system in the Nepomuk server.
 */
NEPOMUK_EXPORT_SERVICE( NepomukActivitiesService, "nepomukactivitiesservice" )

NepomukActivitiesService::NepomukActivitiesService(QObject * parent, const QList < QVariant > & params)
    : Nepomuk::Service( parent )
{
    kDebug() << "started";

    Q_UNUSED(parent)
    Q_UNUSED(params)

    Nepomuk::ResourceManager::instance()->init();

    new NepomukActivitiesServiceAdaptor(this);

    m_model = mainModel();

    setServiceInitialized(true);
}

NepomukActivitiesService::~NepomukActivitiesService()
{
}

QStringList NepomukActivitiesService::listAvailable() const
{
    QStringList result;

    foreach (const Nepomuk::Resource & resource,
            Nepomuk::ResourceManager::instance()->allResourcesOfType(QUrl(URL_ACTIVITY_TYPE))) {

        kDebug() << "url for resource" << resource << urlForResource(resource);

        if (resource.identifiers().size()) {
            result << resource.identifiers()
                .first()
                .replace(QLatin1String("activities://"), QString());
        } else {
            kDebug() << "invalid resource:" << resource.resourceUri() << resource.label();
        }
    }

    return result;
}

void NepomukActivitiesService::add(const QString & id, const QString & name)
{
    Nepomuk::Resource activity = activityResource(id);
    activity.setLabel(name);
    activity.addType(QUrl(URL_ACTIVITY_TYPE));

    kDebug() << activity << id << name;
}

void NepomukActivitiesService::remove(const QString & id)
{
    Nepomuk::Resource activity = activityResource(id);

    kDebug() << activity << id;

    activity.remove();
}

QString NepomukActivitiesService::name(const QString & id) const
{
    return activityResource(id).label();
}

void NepomukActivitiesService::setName(const QString & id, const QString & name)
{
    activityResource(id).setLabel(name);
}

QString NepomukActivitiesService::icon(const QString & id) const
{
    QStringList symbols = activityResource(id).symbols();

    if (symbols.isEmpty()) {
        return QString();
    } else {
        return symbols.first();
    }
}

void NepomukActivitiesService::setIcon(const QString & id, const QString & icon)
{
    QStringList symbols;

    symbols << icon;

    activityResource(id).setSymbols(symbols);
}

QString NepomukActivitiesService::resourceUri(const QString & id) const
{
    return KUrl(activityResource(id).resourceUri()).url();
}

QString NepomukActivitiesService::uri(const QString & id) const
{
    return QString("activities://%1").arg(id);
}

void NepomukActivitiesService::associateResource(const QString & activityID,
    const QString & resourceUri, const QString & typeUri)
{
    Nepomuk::Resource activity = activityResource(activityID);
    Nepomuk::Resource resource = Nepomuk::Resource(resourceUri);

    kDebug() << activity << resource;

    activity.addIsRelated(resource);

    if (!typeUri.isEmpty()) {
        resource.addType(QUrl(typeUri));
        kDebug() << "set the resource type to" << typeUri;
    }
}

void NepomukActivitiesService::disassociateResource(const QString & activityID,
    const QString & resourceUri)
{
    Nepomuk::Resource activity = activityResource(activityID);
    Nepomuk::Resource resource = Nepomuk::Resource(resourceUri);

    kDebug() << activity << resource;

    QList< Nepomuk::Resource > v = activity.isRelateds();
    v.removeAll(resource);
    activity.setIsRelateds(v);
}

QStringList NepomukActivitiesService::associatedResources(
    const QString & id, const QString & resourceType) const
{

    Nepomuk::Resource activity = activityResource(id);

    Nepomuk::Query::Term term = Nepomuk::Query::ComparisonTerm(
            Soprano::Vocabulary::NAO::isRelated(),
            Nepomuk::Query::ResourceTerm(activity)
        ).inverted();

    if (!resourceType.isEmpty()) {
        // OrTerm so that we can support programs that use
        // types not available in ontologies
        KUrl resourceTypeUri(resourceType);

        term = Nepomuk::Query::AndTerm(
            term,
            Nepomuk::Query::OrTerm(
                Nepomuk::Query::ComparisonTerm(
                    Soprano::Vocabulary::RDF::type(),
                    Nepomuk::Query::ResourceTerm(Nepomuk::Resource(resourceTypeUri))
                ),
                Nepomuk::Query::ResourceTypeTerm(resourceTypeUri)
            )
        );
    }

    Nepomuk::Query::Query query(term);

    Soprano::QueryResultIterator it = m_model->executeQuery(query.toSparqlQuery(),
        Soprano::Query::QueryLanguageSparql );

    QStringList result;

    while ( it.next() ) {
        QUrl uri = it["r"].uri();

        Nepomuk::Resource resource(uri);

        result << urlForResource(resource);
    }

    return result;
}

QStringList NepomukActivitiesService::forResource(const QString & uri) const
{
    kDebug() << "uri" << uri;

    Nepomuk::Resource resource = Nepomuk::Resource(KUrl(uri));
    Nepomuk::Resource activityType(QUrl(URL_ACTIVITY_TYPE));

    Nepomuk::Query::Query query(
        Nepomuk::Query::AndTerm(
            Nepomuk::Query::ComparisonTerm(
                Soprano::Vocabulary::RDF::type(),
                Nepomuk::Query::ResourceTerm(activityType)
            ),
            Nepomuk::Query::ComparisonTerm(
                Soprano::Vocabulary::NAO::isRelated(),
                Nepomuk::Query::ResourceTerm(resource)
            )
        )
    );

    Soprano::QueryResultIterator it = m_model->executeQuery(query.toSparqlQuery(),
        Soprano::Query::QueryLanguageSparql);

    QStringList result;

    while ( it.next() ) {
        QUrl uri = it["r"].uri();

        Nepomuk::Resource resource(uri);

        kDebug() << urlForResource(resource);

        result << resource.identifiers()
                          .first()
                          .replace(QLatin1String("activities://"), QString());
    }

    return result;

}

void NepomukActivitiesService::_deleteAll()
{
    foreach (Nepomuk::Resource resource,
            Nepomuk::ResourceManager::instance()->allResourcesOfType(QUrl(URL_ACTIVITY_TYPE))) {
        kDebug() << "NepomukActivitiesService::_deleteAllActivities: resource: " << resource <<
                resource.exists();
        resource.remove();
    }
}

QString NepomukActivitiesService::_serviceIteration() const
{
    return "0.3";
}

// Private

Nepomuk::Resource NepomukActivitiesService::activityResource(const QString & id) const
{
    return Nepomuk::Resource(KUrl("activities://" + id));
}

QString NepomukActivitiesService::urlForResource(const Nepomuk::Resource & resource) const
{
    if (resource.identifiers().size()) {
        return resource.identifiers()[0];

    } else if (resource.hasProperty(Ontologies::nie::url())) {
        return KUrl(resource.property(Ontologies::nie::url()).toUrl()).url();

    } else {
        kDebug() << resource.properties().keys();

        return KUrl(resource.resourceUri()).url();

    }
}

#include "nepomukactivitiesservice.moc"
