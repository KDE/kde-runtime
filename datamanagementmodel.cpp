/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-2011 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "datamanagementmodel.h"
#include "classandpropertytree.h"
#include "resourcemerger.h"

#include <nepomuk/simpleresource.h>
#include <nepomuk/resourceidentifier.h>
#include <nepomuk/resourcemerger.h>

#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>

#include <Soprano/Graph>
#include <Soprano/QueryResultIterator>
#include <Soprano/StatementIterator>
#include <Soprano/NodeIterator>
#include <Soprano/Error/ErrorCode>

#include <QtCore/QHash>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>
#include <QtCore/QUuid>
#include <QtCore/QSet>
#include <QtCore/QPair>

#include <Nepomuk/Vocabulary/NIE>

#include <KDebug>
#include <KService>
#include <KServiceTypeTrader>


//// TODO: do not allow to create properties or classes through the "normal" methods. Instead provide methods for it.

namespace {
    /// used to handle sets and lists of QUrls
    template<typename T> QStringList resourcesToN3(const T& urls) {
        QStringList n3;
        Q_FOREACH(const QUrl& url, urls) {
            n3 << Soprano::Node::resourceToN3(url);
        }
        return n3;
    }

    // convert a hash of URL->URI mappings to N3, omitting empty URIs.
    // This is a helper for the return type of DataManagementModel::resolveUrls
    QStringList resourcesToN3(const QHash<QUrl, QUrl>& urls) {
        QStringList n3;
        QHash<QUrl, QUrl>::const_iterator end = urls.constEnd();
        for(QHash<QUrl, QUrl>::const_iterator it = urls.constBegin(); it != end; ++it) {
            if(!it.value().isEmpty())
                n3 << Soprano::Node::resourceToN3(it.value());
        }
        return n3;
    }

    QStringList nodesToN3(const QSet<Soprano::Node>& nodes) {
        QStringList n3;
        Q_FOREACH(const Soprano::Node& node, nodes) {
            n3 << node.toN3();
        }
        return n3;
    }

    QString createResourceMetadataPropertyFilter(const QString& propVar) {
        return QString::fromLatin1("%1!=%2 && %1!=%3 && %1!=%4 && %1!=%5 && %1!=%6")
                .arg(propVar,
                     Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::created()),
                     Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::lastModified()),
                     Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::creator()),
                     Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::userVisible()),
                     Soprano::Node::resourceToN3(Nepomuk::Vocabulary::NIE::url()));
    }

    template<typename T> QString createResourceExcludeFilter(const T& resources, const QString& var) {
        QStringList terms = resourcesToN3(resources);
        for(int i = 0; i < terms.count(); ++i)
            terms[i].prepend(QString::fromLatin1("%1!=").arg(var));
        return terms.join(QLatin1String(" && "));
    }
}

class Nepomuk::DataManagementModel::Private
{
public:
    ClassAndPropertyTree m_classAndPropertyTree;
};

Nepomuk::DataManagementModel::DataManagementModel(Soprano::Model* model, QObject *parent)
    : Soprano::FilterModel(model),
      d(new Private())
{
    setParent(parent);
    updateTypeCachesAndSoOn();
}

Nepomuk::DataManagementModel::~DataManagementModel()
{
    delete d;
}


Soprano::Error::ErrorCode Nepomuk::DataManagementModel::updateModificationDate(const QUrl& resource, const QUrl & graph, const QDateTime& date)
{
    Q_ASSERT(!graph.isEmpty());

    QSet<QUrl> mtimeGraphs;
    Soprano::QueryResultIterator it = executeQuery(QString::fromLatin1("select ?g where { graph ?g { %1 %2 ?d . } . }")
                                                   .arg(Soprano::Node::resourceToN3(resource),
                                                        Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::lastModified())),
                                                   Soprano::Query::QueryLanguageSparql);
    while(it.next()) {
        mtimeGraphs << it[0].uri();
    }

    Soprano::Error::ErrorCode c = removeAllStatements(resource, Soprano::Vocabulary::NAO::lastModified(), Soprano::Node());
    if (c != Soprano::Error::ErrorNone)
        return c;

    removeTrailingGraphs(mtimeGraphs);

    return addStatement(resource, Soprano::Vocabulary::NAO::lastModified(), Soprano::LiteralValue( date ), graph);
}

void Nepomuk::DataManagementModel::addProperty(const QList<QUrl> &resources, const QUrl &property, const QVariantList &values, const QString &app)
{
    // 1. check cardinality (via property cache/tree)
    // 2. if cardinality 1 (we only have 1 and none):
    //    2.1. make sure values.count() == 1 - otherwise fail
    //    2.2. make sure no value exists - otherwise fail
    // 3. create a new graph with all the necessary data and the app
    // 4. check if the app exists, if not create it in the new graph
    // 5. add the new triples to the new graph
    // 6. update resources' mtime

    //
    // Check parameters
    //
    if(app.isEmpty()) {
        setError(QLatin1String("addProperty: Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    if(resources.isEmpty()) {
        setError(QLatin1String("addProperty: No resource specified."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    foreach( const QUrl & res, resources ) {
        if(res.isEmpty()) {
            setError(QLatin1String("addProperty: Encountered empty resource URI."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }
    if(property.isEmpty()) {
        setError(QLatin1String("addProperty: Property needs to be specified."), Soprano::Error::ErrorInvalidArgument);
        return;
    }


    //
    // Check the integrity of the values
    //
    const QSet<Soprano::Node> nodes = d->m_classAndPropertyTree.variantListToNodeSet(values, property);
    if(nodes.isEmpty()) {
        setError(QString::fromLatin1("addProperty: At least one value could not be converted into an RDF node."), Soprano::Error::ErrorInvalidArgument);
        return;
    }


    //
    // Resolve local file URLs (we need to hash the values since we do not want to write anything yet)
    //
    QHash<QUrl, QUrl> uriHash = resolveUrls(resources);
    QHash<Soprano::Node, Soprano::Node> resolvedNodes = resolveNodes(nodes);


    //
    // Check cardinality conditions
    //
    const int maxCardinality = d->m_classAndPropertyTree.maxCardinality(property);
    if( maxCardinality == 1 ) {
        // check if any of the resources already has a value set which differs from the one we want to add

        // an empty hashed value means that the resource for a file URL does not exist yet. Thus, there is
        // no need to filter it out. We basically only need to check if any value exists.
        QString valueFilter;
        if(resolvedNodes.constBegin().value().isValid()) {
            valueFilter = QString::fromLatin1("FILTER(?v!=%3) . ")
                    .arg(resolvedNodes.constBegin().value().toN3());
        }

        QStringList terms;
        Q_FOREACH(const QUrl& res, resources) {
            if(!uriHash[res].isEmpty()) {
                terms << QString::fromLatin1("%1 %2 ?v . %3")
                         .arg(Soprano::Node::resourceToN3(uriHash[res]),
                              Soprano::Node::resourceToN3(property),
                              valueFilter);
            }
        }

        const QString cardinalityQuery = QString::fromLatin1("ask where { { %1 } }")
                .arg(terms.join(QLatin1String("} UNION {")));

        if(executeQuery(cardinalityQuery, Soprano::Query::QueryLanguageSparql).boolValue()) {
            setError(QString::fromLatin1("%1 has cardinality of 1. At least one of the resources already has a value set that differs from the new one.")
                     .arg(Soprano::Node::resourceToN3(property)), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }


    //
    // Do the actual work
    //
    addProperty(uriHash, property, resolvedNodes, app);
}


// setting a property can be implemented by way of addProperty. All we have to do before calling addProperty is to remove all
// the values that are not in the setProperty call
void Nepomuk::DataManagementModel::setProperty(const QList<QUrl> &resources, const QUrl &property, const QVariantList &values, const QString &app)
{
    //
    // Check parameters
    //
    if(app.isEmpty()) {
        setError(QLatin1String("setProperty: Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    if(resources.isEmpty()) {
        setError(QLatin1String("setProperty: No resource specified."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    foreach( const QUrl & res, resources ) {
        if(res.isEmpty()) {
            setError(QLatin1String("setProperty: Encountered empty resource URI."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }
    if(property.isEmpty()) {
        setError(QLatin1String("setProperty: Property needs to be specified."), Soprano::Error::ErrorInvalidArgument);
        return;
    }

    const QSet<Soprano::Node> nodes = d->m_classAndPropertyTree.variantListToNodeSet(values, property);
    if(!values.isEmpty() && nodes.isEmpty()) {
        setError(QString::fromLatin1("setProperty: At least one value could not be converted into an RDF node."), Soprano::Error::ErrorInvalidArgument);
        return;
    }


    //
    // Resolve local file URLs
    //
    QHash<QUrl, QUrl> uriHash = resolveUrls(resources);
    QHash<Soprano::Node, Soprano::Node> resolvedNodes = resolveNodes(nodes);


    //
    // Remove values that are not wanted anymore
    //
    const QSet<Soprano::Node> existingValues = QSet<Soprano::Node>::fromList(resolvedNodes.values());
    QList<Soprano::BindingSet> existing
            = executeQuery(QString::fromLatin1("select ?r ?v where { ?r %1 ?v . FILTER(?r in (%2)) . }")
                           .arg(Soprano::Node::resourceToN3(property),
                                resourcesToN3(uriHash).join(QLatin1String(","))),
                           Soprano::Query::QueryLanguageSparql).allBindings();
    Q_FOREACH(const Soprano::BindingSet& binding, existing) {
        if(!existingValues.contains(binding["v"])) {
            removeAllStatements(binding["r"], property, binding["v"]);
        }
    }

    //
    // And finally add the rest of the statements (only if there is anything to add)
    //
    if(!nodes.isEmpty()) {
        addProperty(uriHash, property, resolvedNodes, app);
    }
}

void Nepomuk::DataManagementModel::removeProperty(const QList<QUrl> &resources, const QUrl &property, const QVariantList &values, const QString &app)
{
    // 1. remove the triples
    // 2. remove trailing graphs
    // 3. update resource mtime only if we actually change anything on the resource

    //
    // Check arguments
    //
    if(app.isEmpty()) {
        setError(QLatin1String("removeProperty: Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    if(resources.isEmpty()) {
        setError(QLatin1String("removeProperty: No resource specified."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    foreach( const QUrl & res, resources ) {
        if(res.isEmpty()) {
            setError(QLatin1String("removeProperty: Encountered empty resource URI."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }
    if(property.isEmpty()) {
        setError(QLatin1String("removeProperty: Property needs to be specified."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    const QSet<Soprano::Node> valueNodes = d->m_classAndPropertyTree.variantListToNodeSet(values, property);
    if(valueNodes.isEmpty()) {
        setError(QString::fromLatin1("removeProperty: At least one value could not be converted into an RDF node."), Soprano::Error::ErrorInvalidArgument);
        return;
    }


    clearError();


    //
    // Resolve file URLs, we can simply ignore the non-existing file resources which are reflected by empty resolved URIs
    //
    QSet<QUrl> resolvedResources = QSet<QUrl>::fromList(resolveUrls(resources).values());
    QSet<Soprano::Node> resolvedNodes = QSet<Soprano::Node>::fromList(resolveNodes(valueNodes).values());
    resolvedResources.remove(QUrl());
    resolvedNodes.remove(Soprano::Node());


    //
    // Actually change data
    //
    QUrl mtimeGraph;
    QSet<QUrl> graphs;
    const QString propertyN3 = Soprano::Node::resourceToN3(property);
    foreach( const QUrl & res, resolvedResources ) {
        const QList<Soprano::BindingSet> valueGraphs
                = executeQuery(QString::fromLatin1("select ?g ?v where { graph ?g { %1 %2 ?v . } . FILTER(?v in (%3)) . }")
                               .arg(Soprano::Node::resourceToN3(res),
                                    propertyN3,
                                    nodesToN3(resolvedNodes).join(QLatin1String(","))),
                               Soprano::Query::QueryLanguageSparql).allBindings();

        foreach(const Soprano::BindingSet& binding, valueGraphs) {
            graphs.insert( binding["g"].uri() );
            removeAllStatements( res, property, binding["v"] );
        }

        // we only update the mtime in case we actually remove anything
        if(!valueGraphs.isEmpty()) {
            // If the resource is now empty we remove it completely
            if(!doesResourceExist(res)) {
                removeResources(QList<QUrl>() << res, app, false);
            }
            else {
                if(mtimeGraph.isEmpty()) {
                    mtimeGraph = createGraph(app);
                }
                updateModificationDate(res, mtimeGraph);
            }
        }
    }

    removeTrailingGraphs( graphs );
}

void Nepomuk::DataManagementModel::removeProperties(const QList<QUrl> &resources, const QList<QUrl> &properties, const QString &app)
{
    // 1. remove the triples
    // 2. remove trailing graphs
    // 3. update resource mtime

    //
    // Check arguments
    //
    if(app.isEmpty()) {
        setError(QLatin1String("removeProperties: Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    if(resources.isEmpty()) {
        setError(QLatin1String("removeProperties: No resource specified."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    foreach( const QUrl & res, resources ) {
        if(res.isEmpty()) {
            setError(QLatin1String("removeProperties: Encountered empty resource URI."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }
    if(properties.isEmpty()) {
        setError(QLatin1String("removeProperties: No properties specified."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    foreach(const QUrl& property, properties) {
        if(property.isEmpty()) {
            setError(QLatin1String("removeProperties: Encountered empty property URI."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }


    clearError();


    //
    // Resolve file URLs, we can simply ignore the non-existing file resources which are reflected by empty resolved URIs
    //
    QSet<QUrl> resolvedResources = QSet<QUrl>::fromList(resolveUrls(resources).values());
    resolvedResources.remove(QUrl());


    //
    // Actually change data
    //
    QUrl mtimeGraph;
    QSet<QUrl> graphs;
    foreach( const QUrl & res, resolvedResources ) {
        const QList<Soprano::BindingSet> valueGraphs
                = executeQuery(QString::fromLatin1("select ?g ?p where { graph ?g { %1 ?p ?v . } . FILTER(?p in (%2)) . }")
                               .arg(Soprano::Node::resourceToN3(res),
                                    resourcesToN3(properties).join(QLatin1String(","))),
                               Soprano::Query::QueryLanguageSparql).allBindings();

        foreach(const Soprano::BindingSet& binding, valueGraphs) {
            graphs.insert( binding["g"].uri() );
            removeAllStatements( res, binding["p"], Soprano::Node() );
        }

        // we only update the mtime in case we actually remove anything
        if(!valueGraphs.isEmpty()) {
            // If the resource is now empty we remove it completely
            if(!doesResourceExist(res)) {
                removeResources(QList<QUrl>() << res, app, false);
            }
            else {
                if(mtimeGraph.isEmpty()) {
                    mtimeGraph = createGraph(app);
                }
                updateModificationDate(res, mtimeGraph);
            }
        }
    }

    removeTrailingGraphs( graphs );
}


QUrl Nepomuk::DataManagementModel::createResource(const QList<QUrl> &types, const QString &label, const QString &description, const QString &app)
{
    // 1. create an new graph
    // 2. check if the app exists, if not create it in the new graph
    // 3. create the new resource in the new graph
    // 4. return the resource's URI

    //
    // Check parameters
    //
    if(app.isEmpty()) {
        setError(QLatin1String("createResource: Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return QUrl();
    }
    foreach(const QUrl& type, types) {
        if(type.isEmpty()) {
            setError(QLatin1String("createResource: Encountered empty type URI."), Soprano::Error::ErrorInvalidArgument);
            return QUrl();
        }
    }

    clearError();

    const QUrl graph = createGraph(app);
    const QUrl resUri = createUri(ResourceUri);

    foreach(const QUrl& type, types) {
        addStatement(resUri, Soprano::Vocabulary::RDF::type(), type, graph);
    }
    if(!label.isEmpty()) {
        addStatement(resUri, Soprano::Vocabulary::NAO::prefLabel(), Soprano::LiteralValue(label), graph);
    }
    if(!description.isEmpty()) {
        addStatement(resUri, Soprano::Vocabulary::NAO::description(), Soprano::LiteralValue(description), graph);
    }
    return resUri;
}

void Nepomuk::DataManagementModel::removeResources(const QList<QUrl> &resources, const QString &app, RemovalFlags flags)
{
    kDebug() << resources << app << flags;

    Q_UNUSED(app);
    // 1. get all sub-resources and check if they are used by some other resource (not in the list of resources to remove)
    //    for the latter one can use a bif:exists and a !filter(?s in <s>, <s>, ...) - based on the value of force
    // 2. remove the resources and the sub-resources
    // 3. drop trailing graphs (could be optimized by enumerating the graphs up front)

    //
    // Check parameters
    //
    if(app.isEmpty()) {
        setError(QLatin1String("removeResources: Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    if(resources.isEmpty()) {
        setError(QLatin1String("removeResources: No resource specified."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    foreach( const QUrl & res, resources ) {
        if(res.isEmpty()) {
            setError(QLatin1String("removeResources: Encountered empty resource URI."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }


    clearError();


    //
    // Resolve file URLs, we can simply ignore the non-existing file resources which are reflected by empty resolved URIs
    //
    QSet<QUrl> resolvedResources = QSet<QUrl>::fromList(resolveUrls(resources).values());
    resolvedResources.remove(QUrl());
    if(resolvedResources.isEmpty()) {
        return;
    }

    //
    // Handle the sub-resources:
    // this has to be done before deleting the resouces in resolvedResources. Otherwise the nao:hasSubResource relationships are already gone!
    //
    // Explanation of the query:
    // The query selects all subresources of the resources in resolvedResources.
    // It then filters out the sub-resources that are related from other resources that are not the ones being deleted.
    //
    if(flags & RemoveSubResoures) {
        QList<QUrl> subResources;
        Soprano::QueryResultIterator it
                = executeQuery(QString::fromLatin1("select ?r where { ?r ?p ?o . "
                                                   "?parent %1 ?r . "
                                                   "FILTER(?parent in (%2)) . "
                                                   "FILTER(!bif:exists((select (1) where { ?r2 ?p3 ?r . FILTER(%3) . FILTER(!bif:exists((select (1) where { ?x %1 ?r2 . FILTER(?x in (%2)) . }))) . }))) . "
                                                   "}")
                               .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::hasSubResource()),
                                    resourcesToN3(resolvedResources).join(QLatin1String(",")),
                                    createResourceExcludeFilter(resolvedResources, QLatin1String("?r2"))),
                               Soprano::Query::QueryLanguageSparql);
        while(it.next()) {
            subResources << it[0].uri();
        }
        if(!subResources.isEmpty()) {
            removeResources(subResources, app, flags);
        }
    }


    // get the graphs we need to check with removeTrailingGraphs later on
    QSet<QUrl> graphs;
    Soprano::QueryResultIterator it
            = executeQuery(QString::fromLatin1("select distinct ?g where { graph ?g { ?r ?p ?o . } . FILTER(?r in (%1)) . }")
                           .arg(resourcesToN3(resolvedResources).join(QLatin1String(","))),
                           Soprano::Query::QueryLanguageSparql);
    while(it.next()) {
        graphs << it[0].uri();
    }

    // remove the resources
    foreach(const QUrl & res, resolvedResources) {
        removeAllStatements(res, Soprano::Node(), Soprano::Node());
        removeAllStatements(Soprano::Node(), Soprano::Node(), res);
    }

    removeTrailingGraphs(graphs);
}

void Nepomuk::DataManagementModel::removeDataByApplication(const QList<QUrl> &resources, const QString &app, RemovalFlags flags)
{
    //
    // Check parameters
    //
    if(app.isEmpty()) {
        setError(QLatin1String("removeDataByApplication: Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    foreach( const QUrl & res, resources ) {
        if(res.isEmpty()) {
            setError(QLatin1String("removeDataByApplication: Encountered empty resource URI."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }


    clearError();


    const QUrl appRes = findApplicationResource(app, false);
    if(appRes.isEmpty()) {
        return;
    }


    //
    // Resolve file URLs, we can simply ignore the non-existing file resources which are reflected by empty resolved URIs
    //
    QSet<QUrl> resolvedResources = QSet<QUrl>::fromList(resolveUrls(resources).values());
    resolvedResources.remove(QUrl());
    if(resolvedResources.isEmpty()) {
        return;
    }


    //
    // Handle the sub-resources: we can delete all sub-resources of the deleted ones that are entirely defined by our app
    // and are not related by other resources.
    // this has to be done before deleting the resouces in resolvedResources. Otherwise the nao:hasSubResource relationships are already gone!
    //
    // Explanation of the query:
    // The query selects all subresources of the resources in resolvedResources.
    // It then filters out those resources that are maintained by another app.
    // It then filters out the sub-resources that have properties defined by other apps which are not metadata.
    // It then filters out the sub-resources that are related from other resources that are not the ones being deleted.
    //
    if(flags & RemoveSubResoures) {
        QList<QUrl> subResources;
        Soprano::QueryResultIterator it
                = executeQuery(QString::fromLatin1("select ?r where { graph ?g { ?r ?p ?o . } . "
                                                   "?parent %1 ?r . "
                                                   "FILTER(?parent in (%2)) . "
                                                   "?g %3 %4 . "
                                                   "FILTER(!bif:exists((select (1) where { ?g %3 ?a . FILTER(?a!=%4) . }))) . "
                                                   "FILTER(!bif:exists((select (1) where { graph ?g2 { ?r ?p2 ?o2 . } . ?g2 %3 ?a2 . FILTER(?a2!=%4) . FILTER(%6) . }))) . "
                                                   "FILTER(!bif:exists((select (1) where { graph ?g2 { ?r2 ?p3 ?r . } . FILTER(%5) . FILTER(!bif:exists((select (1) where { ?x %1 ?r2 . FILTER(?x in (%2)) . }))) . }))) . "
                                                   "}")
                               .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::hasSubResource()),
                                    resourcesToN3(resolvedResources).join(QLatin1String(",")),
                                    Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::maintainedBy()),
                                    Soprano::Node::resourceToN3(appRes),
                                    createResourceExcludeFilter(resolvedResources, QLatin1String("?r2")),
                                    createResourceMetadataPropertyFilter(QLatin1String("?p2"))),
                               Soprano::Query::QueryLanguageSparql);
        while(it.next()) {
            subResources << it[0].uri();
        }
        if(!subResources.isEmpty()) {
            removeDataByApplication(subResources, app, flags);
        }
    }


    // Get the graphs we need to check with removeTrailingGraphs later on.
    QHash<QUrl, int> graphs;
    Soprano::QueryResultIterator it
            = executeQuery(QString::fromLatin1("select distinct ?g (select count(distinct ?app) where { ?g %1 ?app . }) as ?c where { "
                                               "graph ?g { ?r ?p ?o . } . "
                                               "?g %1 %2 . "
                                               "FILTER(?r in (%3)) . }")
                           .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::maintainedBy()),
                                Soprano::Node::resourceToN3(appRes),
                                resourcesToN3(resolvedResources).join(QLatin1String(","))),
                           Soprano::Query::QueryLanguageSparql);
    while(it.next()) {
        graphs.insert(it["g"].uri(), it["c"].literal().toInt());
    }


    // remove the resources
    // Other apps might be maintainer, too. In that case only remove the app as a maintainer but keep the data
    const QDateTime now = QDateTime::currentDateTime();
    QUrl mtimeGraph;
    for(QHash<QUrl, int>::const_iterator it = graphs.constBegin(); it != graphs.constEnd(); ++it) {
        const QUrl& g = it.key();
        const int appCnt = it.value();
        if(appCnt == 1) {
            foreach(const QUrl& res, resolvedResources) {
                if(doesResourceExist(res, g)) {
                    // we cannot remove the nie:url since that would remove the tie to the desktop resource
                    // thus, we remember it and re-add it later on
                    QUrl nieUrl;
                    Soprano::QueryResultIterator nieUrlIt
                            = executeQuery(QString::fromLatin1("select ?u where { graph %1 { %2 %3 ?u . } . } limit 1")
                                           .arg(Soprano::Node::resourceToN3(g),
                                                Soprano::Node::resourceToN3(res),
                                                Soprano::Node::resourceToN3(Nepomuk::Vocabulary::NIE::url())),
                                           Soprano::Query::QueryLanguageSparql);
                    if(nieUrlIt.next()) {
                        nieUrl = nieUrlIt[0].uri();
                    }

                    removeAllStatements(res, Soprano::Node(), Soprano::Node(), g);
                    removeAllStatements(Soprano::Node(), Soprano::Node(), res, g);

                    if(!nieUrl.isEmpty()) {
                        addStatement(res, Nepomuk::Vocabulary::NIE::url(), nieUrl, g);
                    }

                    // update mtime
                    if(mtimeGraph.isEmpty()) {
                        mtimeGraph = createGraph(app);
                        if(lastError()) {
                            return;
                        }
                    }
                    updateModificationDate(res, mtimeGraph, now);
                }
            }
        }
        else {
            removeAllStatements(g, Soprano::Vocabulary::NAO::maintainedBy(), appRes);
        }
    }

    // make sure we do not leave anything empty trailing around and propery update the mtime
    QList<QUrl> resourcesToRemoveCompletely;
    foreach(const QUrl& res, resolvedResources) {
        if(!doesResourceExist(res)) {
            resourcesToRemoveCompletely << res;
        }
    }
    if(!resourcesToRemoveCompletely.isEmpty()){
        removeResources(resourcesToRemoveCompletely, app, flags);
    }


    removeTrailingGraphs(QSet<QUrl>::fromList(graphs.keys()));
}

void Nepomuk::DataManagementModel::removeDataByApplication(const QString &app, RemovalFlags flags)
{
    setError("Not implemented yet");
    return;

    //
    // Check parameters
    //
    if(app.isEmpty()) {
        setError(QLatin1String("removeDataByApplication: Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }

    clearError();

    const QUrl appRes = findApplicationResource(app, false);
    if(appRes.isEmpty()) {
        return;
    }

    // TODO: remove all graphs maintained by app alone
    //       without deleting any resource metadata for resources that still exist in other graphs

    // We use two steps:
    // 1. remove all graphs that do not contain any data that is vital to any other graph we cannot delete
    // 2. remove the parts of the graphs that we can delete

    QSet<QUrl> graphsToRemove;
    QSet<QUrl> resourcesToCheckForRemoval;
    Soprano::QueryResultIterator it
            = executeQuery(QString::fromLatin1("select ?g ?r where { graph ?g { ?r ?p ?o . } . "
                                               "?g %1 %2 . FILTER(!bif:exists((select (1) where { ?g %1 ?a . FILTER(?a!=%2) . }))) . "
                                               "FILTER(!bif:exists((select (1) where { graph ?g2 { ?r ?p2 ?o2 . } . ?g2 %1 ?a2 . FILTER(?a2!=%2) . FILTER(%3)})) || %4) . " // %4 is incorrect. What we actually want is to check that there is no metadata ?p in the graph
                                               "}")
                           .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::maintainedBy()),
                                Soprano::Node::resourceToN3(appRes),
                                createResourceMetadataPropertyFilter(QLatin1String("?p2")),
                                createResourceMetadataPropertyFilter(QLatin1String("?p"))),
                           Soprano::Query::QueryLanguageSparql);
    while(it.next()) {
        graphsToRemove.insert(it["g"].uri());
        resourcesToCheckForRemoval.insert(it["r"].uri());
    }

    // remove the graphs we can remove without problems
    Q_FOREACH(const QUrl& g, graphsToRemove) {
        if(removeContext(g) != Soprano::Error::ErrorNone)
            return;
    }


    // Now step 2: get the graphs that contain data from the app but cannot be removed entirely
    QHash<QUrl, QPair<int, QUrl> > graphs;
    it = executeQuery(QString::fromLatin1("select distinct ?g (select count(distinct ?app) where { ?g %1 ?app . }) as ?c ?a where { "
                                          "graph ?g { ?r ?p ?o . } . "
                                          "?g %1 ?a . "
                                          "?a %2 %3 . }")
                      .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::maintainedBy()),
                           Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::identifier()),
                           Soprano::Node::literalToN3(app)),
                      Soprano::Query::QueryLanguageSparql);
    while(it.next()) {
        graphs.insert(it["g"].uri(), qMakePair(it["c"].literal().toInt(), it["a"].uri()));
    }

    const QDateTime now = QDateTime::currentDateTime();
    QUrl mtimeGraph;
    for(QHash<QUrl, QPair<int, QUrl> >::const_iterator it = graphs.constBegin(); it != graphs.constEnd(); ++it) {
        const QUrl& g = it.key();
        const int appCnt = it.value().first;
        const QUrl& appUri = it.value().second;
        if(appCnt == 1) {
            // remove the graph, but first check if we remove anything vital. This is tricky: it might only be vital to another graph we delete!

        }
        else {
            removeAllStatements(g, Soprano::Vocabulary::NAO::maintainedBy(), appUri);
        }
    }

    removeTrailingGraphs(QSet<QUrl>::fromList(graphs.keys()));
}

void Nepomuk::DataManagementModel::removePropertiesByApplication(const QList<QUrl> &resources, const QList<QUrl> &properties, const QString &app)
{
    setError("Not implemented yet");
}


void Nepomuk::DataManagementModel::mergeResources(const Nepomuk::SimpleResourceGraph &resources, const QString &app, const QHash<QUrl, QVariant> &additionalMetadata)
{

    QList<Soprano::Statement> allStatements;

    if(app.isEmpty()) {
        setError(QLatin1String("mergeResources: Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }

    clearError();

    if(resources.isEmpty()) {
        // nothing to do
        return;
    }

    Sync::ResourceIdentifier resIdent;
    
    foreach( const SimpleResource & res, resources ) {
        QList< Soprano::Statement > stList = res.toStatementList();
        allStatements << stList;

        if(stList.isEmpty()) {
            setError(QLatin1String("mergeResources: Encountered invalid resource"), Soprano::Error::ErrorInvalidArgument);
            return;
        }

        // trueg: IMHO fromStatementList should be able to handle an empty list and not assert on it.
        //        Instead it should simply return an invalid SimpleResource.
        Sync::SimpleResource simpleRes = Sync::SimpleResource::fromStatementList( stList );
        resIdent.addSimpleResource( simpleRes );
        // TODO: check if res is valid and if not: setError...
    }
    
    resIdent.setModel( this );
    resIdent.setMinScore( 1.0 );
    resIdent.identifyAll();

    if( resIdent.mappings().empty() ) {
        kDebug() << "Nothing was mapped merging everything as it is.";
        //vHanda: This means that everything should be pushed, right?
        //return;
    }

    ResourceMerger merger( this, app, additionalMetadata );
    merger.merge( Soprano::Graph(allStatements), resIdent.mappings() );

    //// TODO: do not allow to create properties or classes this way
    //setError("Not implemented yet");
}

namespace {
QVariant nodeToVariant(const Soprano::Node& node) {
    if(node.isResource())
        return node.uri();
    else if(node.isBlank())
        return QUrl(node.identifier());
    else
        return node.literal().variant();
}
}

Nepomuk::SimpleResourceGraph Nepomuk::DataManagementModel::describeResources(const QList<QUrl> &resources, bool includeSubResources) const
{
    //
    // check parameters
    //
    if(resources.isEmpty()) {
        setError(QLatin1String("describeResources: No resource specified."), Soprano::Error::ErrorInvalidArgument);
        return SimpleResourceGraph();
    }
    foreach( const QUrl & res, resources ) {
        if(res.isEmpty()) {
            setError(QLatin1String("describeResources: Encountered empty resource URI."), Soprano::Error::ErrorInvalidArgument);
            return SimpleResourceGraph();
        }
    }


    clearError();


    //
    // Split into file and non-file URIs so we can get all data in one query
    //
    QSet<QUrl> fileUrls;
    QSet<QUrl> resUris;
    foreach( const QUrl & res, resources ) {
        if(res.scheme() == QLatin1String("file"))
            fileUrls.insert(res);
        else
            resUris.insert(res);
    }


    //
    // Build the query
    //
    QStringList terms;
    if(!fileUrls.isEmpty()) {
        terms << QString::fromLatin1("?s ?p ?o . ?s %1 ?u . FILTER(?u in (%2)) . ")
                 .arg(Soprano::Node::resourceToN3(Vocabulary::NIE::url()),
                      resourcesToN3(fileUrls).join(QLatin1String(",")));
        if(includeSubResources) {
            terms << QString::fromLatin1("?s ?p ?o . ?r %1 ?s . ?r %2 ?u . FILTER(?u in (%3)) . ")
                     .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::hasSubResource()),
                          Soprano::Node::resourceToN3(Vocabulary::NIE::url()),
                          resourcesToN3(fileUrls).join(QLatin1String(",")));
        }
    }
    if(!resUris.isEmpty()) {
        terms << QString::fromLatin1("?s ?p ?o . FILTER(?s in (%1)) . ")
                 .arg(resourcesToN3(resUris).join(QLatin1String(",")));
        if(includeSubResources) {
            terms << QString::fromLatin1("?s ?p ?o . ?r %1 ?s . FILTER(?r in (%2)) . ")
                     .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::hasSubResource()),
                          resourcesToN3(resUris).join(QLatin1String(",")));
        }
    }

    QString query = QLatin1String("select distinct ?s ?p ?o where { ");
    if(terms.count() == 1) {
        query += terms.first();
    }
    else {
        query += QLatin1String("{ ") + terms.join(QLatin1String("} UNION { ")) + QLatin1String("}");
    }
    query += QLatin1String(" }");


    //
    // Build the graph
    //
    QHash<QUrl, SimpleResource> graph;
    Soprano::QueryResultIterator it = executeQuery(query, Soprano::Query::QueryLanguageSparql);
    while(it.next()) {
        const QUrl r = it["s"].uri();
        graph[r].setUri(r);
        graph[r].m_properties.insertMulti(it["p"].uri(), nodeToVariant(it["o"]));
    }
    if(it.lastError()) {
        setError(it.lastError());
        return SimpleResourceGraph();
    }

    return graph.values();
}


void Nepomuk::DataManagementModel::removeTrailingGraphs(const QSet<QUrl>& graphs)
{
    kDebug() << graphs;

    // The underlying Soprano::NRLModel will take care of the metadata graphs
    foreach( const QUrl& graph, graphs ) {
        Q_ASSERT(!graph.isEmpty());
        if( !containsAnyStatement( Soprano::Node(), Soprano::Node(), Soprano::Node(), graph) ) {
            removeContext(graph);
        }
    }
}


QUrl Nepomuk::DataManagementModel::createGraph(const QString &app, const QHash<QUrl, QVariant> &additionalMetadata)
{
    QHash<QUrl, Soprano::Node> graphMetaData;

    // determine the graph type
    bool haveGraphType = false;
    for(QHash<QUrl, QVariant>::const_iterator it = additionalMetadata.constBegin();
        it != additionalMetadata.constEnd(); ++it) {
        const QUrl& property = it.key();

        if(property == Soprano::Vocabulary::RDF::type()) {
            // check if it is a valid type
            if(it.value().type() != QVariant::Url) {
                setError(QString::fromLatin1("rdf:type has resource range. %1 was provided.").arg(it.value().type()), Soprano::Error::ErrorInvalidArgument);
                return QUrl();
            }
            else {
                if(d->m_classAndPropertyTree.isChildOf(it.value().toUrl(), Soprano::Vocabulary::NRL::Graph()))
                    haveGraphType = true;
            }
        }

        else if(property == Soprano::Vocabulary::NAO::created()) {
            if(!it.value().type() == QVariant::DateTime) {
                setError(QString::fromLatin1("nao:created has dateTime range. %1 was provided.").arg(it.value().type()), Soprano::Error::ErrorInvalidArgument);
                return QUrl();
            }
        }

        else {
            // FIXME: check property, domain, and range
        }

        Soprano::Node node = d->m_classAndPropertyTree.variantToNode(it.value(), property);
        if(node.isValid()) {
            graphMetaData.insert(it.key(), node);
        }
        else {
            setError(QString::fromLatin1("Cannot convert %1 to literal value.").arg(it.value().type()), Soprano::Error::ErrorInvalidArgument);
            return QUrl();
        }
    }

    // add missing metadata
    if(!haveGraphType) {
        graphMetaData.insert(Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::InstanceBase());
    }
    if(!graphMetaData.contains(Soprano::Vocabulary::NAO::created())) {
        graphMetaData.insert(Soprano::Vocabulary::NAO::created(), Soprano::LiteralValue(QDateTime::currentDateTime()));
    }
    if(!graphMetaData.contains(Soprano::Vocabulary::NAO::maintainedBy())) {
        graphMetaData.insert(Soprano::Vocabulary::NAO::maintainedBy(), findApplicationResource(app));
    }

    const QUrl graph = createUri(GraphUri);
    const QUrl metadatagraph = createUri(GraphUri);

    // add metadata graph itself
    addStatement( metadatagraph, Soprano::Vocabulary::NRL::coreGraphMetadataFor(), graph, metadatagraph );
    addStatement( metadatagraph, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::GraphMetadata(), metadatagraph );

    for(QHash<QUrl, Soprano::Node>::const_iterator it = graphMetaData.constBegin();
        it != graphMetaData.constEnd(); ++it) {
        addStatement(graph, it.key(), it.value(), metadatagraph);
    }

    return graph;
}

QUrl Nepomuk::DataManagementModel::findApplicationResource(const QString &app, bool create)
{
    Soprano::QueryResultIterator it =
            executeQuery(QString::fromLatin1("select ?r where { ?r a %1 . ?r %2 %3 . } LIMIT 1")
                         .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::Agent()),
                              Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::identifier()),
                              Soprano::Node::literalToN3(app)),
                         Soprano::Query::QueryLanguageSparql);
    if(it.next()) {
        return it[0].uri();
    }
    else if(create) {
        const QUrl graph = createUri(GraphUri);
        const QUrl metadatagraph = createUri(GraphUri);
        const QUrl uri = createUri(ResourceUri);

        // graph metadata
        addStatement( metadatagraph, Soprano::Vocabulary::NRL::coreGraphMetadataFor(), graph, metadatagraph );
        addStatement( metadatagraph, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::GraphMetadata(), metadatagraph );
        addStatement( graph, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::InstanceBase(), metadatagraph );
        addStatement( graph, Soprano::Vocabulary::NAO::created(), Soprano::LiteralValue(QDateTime::currentDateTime()), metadatagraph );

        // the app itself
        addStatement( uri, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NAO::Agent(), graph );
        addStatement( uri, Soprano::Vocabulary::NAO::identifier(), Soprano::LiteralValue(app), graph );

        KService::List services = KServiceTypeTrader::self()->query(QLatin1String("Application"),
                                                                    QString::fromLatin1("DesktopEntryName=='%1'").arg(app));
        if(services.count() == 1) {
            addStatement(uri, Soprano::Vocabulary::NAO::prefLabel(), Soprano::LiteralValue(services.first()->name()), graph);
        }

        return uri;
    }
    else {
        return QUrl();
    }
}

QUrl Nepomuk::DataManagementModel::createUri(Nepomuk::DataManagementModel::UriType type)
{
    QString typeToken;
    if(type == GraphUri)
        typeToken = QLatin1String("ctx");
    else
        typeToken = QLatin1String("res");

    while( 1 ) {
        QString uuid = QUuid::createUuid().toString();
        uuid = uuid.mid(1, uuid.length()-2);
        const QUrl uri = QUrl( QLatin1String("nepomuk:/") + typeToken + QLatin1String("/") + uuid );
        if ( !FilterModel::executeQuery( QString::fromLatin1("ask where { "
                                                             "{ %1 ?p1 ?o1 . } "
                                                             "UNION "
                                                             "{ ?s2 %1 ?o2 . } "
                                                             "UNION "
                                                             "{ ?s3 ?p3 %1 . } "
                                                             "UNION "
                                                             "{ graph %1 { ?s4 ?4 ?o4 . } . } "
                                                             "}")
                                        .arg( Soprano::Node::resourceToN3(uri) ), Soprano::Query::QueryLanguageSparql ).boolValue() ) {
            return uri;
        }
    }
}

void Nepomuk::DataManagementModel::updateTypeCachesAndSoOn()
{
    d->m_classAndPropertyTree.rebuildTree(parentModel());
}


void Nepomuk::DataManagementModel::addProperty(const QHash<QUrl, QUrl> &resources, const QUrl &property, const QHash<Soprano::Node, Soprano::Node> &nodes, const QString &app)
{
    Q_ASSERT(!resources.isEmpty());
    Q_ASSERT(!nodes.isEmpty());
    Q_ASSERT(!property.isEmpty());

    //
    // Check cardinality conditions
    //
    const int maxCardinality = d->m_classAndPropertyTree.maxCardinality(property);
    if( maxCardinality == 1 ) {
        if( nodes.size() != 1 ) {
            setError(QString::fromLatin1("%1 has cardinality of 1. Cannot set more then one value.").arg(property.toString()), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }


    clearError();


    //
    // Resolve file URLs
    //
    QUrl graph;
    QSet<Soprano::Node> resolvedNodes;
    QHash<Soprano::Node, Soprano::Node>::const_iterator end = nodes.constEnd();
    for(QHash<Soprano::Node, Soprano::Node>::const_iterator it = nodes.constBegin();
        it != end; ++it) {
        if(it.value().isEmpty()) {
            if(graph.isEmpty()) {
                graph = createGraph( app );
                if(!graph.isValid()) {
                    // error has been set in createGraph
                    return;
                }
            }
            QUrl uri = createUri(ResourceUri);
            addStatement(uri, Vocabulary::NIE::url(), it.key().uri(), graph);
            resolvedNodes.insert(uri);
        }
        else {
            resolvedNodes.insert(it.value());
        }
    }

    QSet<QPair<QUrl, Soprano::Node> > finalProperties;
    QList<QUrl> knownResources;
    for(QHash<QUrl, QUrl>::const_iterator it = resources.constBegin();
        it != resources.constEnd(); ++it) {
        QUrl uri = it.value();
        if(uri.isEmpty()) {
            if(graph.isEmpty()) {
                graph = createGraph( app );
                if(!graph.isValid()) {
                    // error has been set in createGraph
                    return;
                }
            }
            uri = createUri(ResourceUri);
            addStatement(uri, Vocabulary::NIE::url(), it.key(), graph);
        }
        else {
            knownResources << uri;
        }
        Q_FOREACH(const Soprano::Node& node, resolvedNodes) {
            finalProperties << qMakePair(uri, node);
        }
    }

    const QUrl appRes = findApplicationResource(app);

    //
    // Check if values already exist. If so remove the resources from the resourceSet and only add the application
    // as maintainedBy in a new graph (except if its the only statement in the graph)
    //
    if(!knownResources.isEmpty()) {
        const QString existingValuesQuery = QString::fromLatin1("select distinct ?r ?v ?g ?m "
                                                                "(select count(*) where { graph ?g { ?s ?p ?o . } . FILTER(%5) . }) as ?cnt "
                                                                "where { "
                                                                "graph ?g { ?r %2 ?v . } . "
                                                                "?m %1 ?g . "
                                                                "FILTER(?r in (%3)) . "
                                                                "FILTER(?v in (%4)) . }")
                .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::coreGraphMetadataFor()),
                     Soprano::Node::resourceToN3(property),
                     resourcesToN3(knownResources).join(QLatin1String(",")),
                     nodesToN3(resolvedNodes).join(QLatin1String(",")),
                     createResourceMetadataPropertyFilter(QLatin1String("?p")));
        QList<Soprano::BindingSet> existingValueBindings = executeQuery(existingValuesQuery, Soprano::Query::QueryLanguageSparql).allBindings();
        Q_FOREACH(const Soprano::BindingSet& binding, existingValueBindings) {
            kDebug() << "Existing value" << binding;
            const QUrl r = binding["r"].uri();
            const Soprano::Node v = binding["v"];
            const QUrl g = binding["g"].uri();
            const QUrl m = binding["m"].uri();
            const int cnt = binding["cnt"].literal().toInt();

            // we handle this property here - thus, no need to handle it below
            finalProperties.remove(qMakePair(r, v));

            // in case the app is the same there is no need to do anything
            if(containsStatement(g, Soprano::Vocabulary::NAO::maintainedBy(), appRes, m)) {
                continue;
            }
            else if(cnt == 1) {
                // we can reuse the graph
                addStatement(g, Soprano::Vocabulary::NAO::maintainedBy(), appRes, m);
            }
            else {
                // we need to split the graph
                const QUrl graph = createUri(GraphUri);
                const QUrl metadataGraph = createUri(GraphUri);

                // FIXME: do not split the same graph again and again. Check if the graph in question already is the one we created.

                // add metadata graph
                addStatement( metadataGraph, Soprano::Vocabulary::NRL::coreGraphMetadataFor(), graph, metadataGraph );
                addStatement( metadataGraph, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::GraphMetadata(), metadataGraph );

                // copy the metadata from the old graph to the new one
                executeQuery(QString::fromLatin1("insert into %1 { %2 ?p ?o . } where { graph %3 { %4 ?p ?o . } . }")
                             .arg(Soprano::Node::resourceToN3(metadataGraph),
                                  Soprano::Node::resourceToN3(graph),
                                  Soprano::Node::resourceToN3(m),
                                  Soprano::Node::resourceToN3(g)),
                             Soprano::Query::QueryLanguageSparql);

                // add the new app
                addStatement(graph, Soprano::Vocabulary::NAO::maintainedBy(), appRes, metadataGraph);

                // and finally move the actual property over to the new graph
                removeStatement(r, property, v, g);
                addStatement(r, property, v, graph);
            }
        }
    }


    //
    // All conditions have been checked - create the actual data
    //
    if(!finalProperties.isEmpty()) {
        if(graph.isEmpty()) {
            graph = createGraph( app );
            if(!graph.isValid()) {
                // error has been set in createGraph
                return;
            }
        }

        // add all the data
        // TODO: check if using one big sparql insert improves performance
        QSet<QUrl> finalResources;
        for(QSet<QPair<QUrl, Soprano::Node> >::const_iterator it = finalProperties.constBegin(); it != finalProperties.constEnd(); ++it) {
            Soprano::Node node = it->second;
            if(node.isResource()) {
                QUrl uri = resolveUrl(node.uri());
                if(uri.isEmpty()) {
                    uri = createUri(ResourceUri);
                    addStatement(uri, Vocabulary::NIE::url(), node.uri(), graph);
                }
                node = uri;
            }
            addStatement(it->first, property, node, graph);
            finalResources.insert(it->first);
        }

        // update modification date
        Q_FOREACH(const QUrl& res, finalResources) {
            updateModificationDate(res, graph);
        }
    }
}

bool Nepomuk::DataManagementModel::doesResourceExist(const QUrl &res, const QUrl& graph) const
{
    if(graph.isEmpty()) {
        return executeQuery(QString::fromLatin1("ask where { %1 ?p ?v . FILTER(%2) . }")
                            .arg(Soprano::Node::resourceToN3(res),
                                 createResourceMetadataPropertyFilter(QLatin1String("?p"))),
                            Soprano::Query::QueryLanguageSparql).boolValue();
    }
    else {
        return executeQuery(QString::fromLatin1("ask where { graph %1 { %2 ?p ?v . FILTER(%3) . } . }")
                            .arg(Soprano::Node::resourceToN3(graph),
                                 Soprano::Node::resourceToN3(res),
                                 createResourceMetadataPropertyFilter(QLatin1String("?p"))),
                            Soprano::Query::QueryLanguageSparql).boolValue();
    }
}

QHash<QUrl, QUrl> Nepomuk::DataManagementModel::resolveUrls(const QList<QUrl> &urls) const
{
    QHash<QUrl, QUrl> uriHash;
    Q_FOREACH(const QUrl& url, urls) {
        uriHash.insert(url, resolveUrl(url));
    }
    return uriHash;
}

QUrl Nepomuk::DataManagementModel::resolveUrl(const QUrl &url) const
{
    if(url.scheme() == QLatin1String("file")) {
        Soprano::QueryResultIterator it
                = executeQuery(QString::fromLatin1("select ?r where { ?r %1 %2 . } limit 1")
                               .arg(Soprano::Node::resourceToN3(Nepomuk::Vocabulary::NIE::url()),
                                    Soprano::Node::resourceToN3(url)),
                               Soprano::Query::QueryLanguageSparql);
        if(it.next()) {
            return it[0].uri();
        }
        else {
            return QUrl();
        }
    }
    else {
        return url;
    }
}

QHash<Soprano::Node, Soprano::Node> Nepomuk::DataManagementModel::resolveNodes(const QSet<Soprano::Node> &nodes) const
{
    QHash<Soprano::Node, Soprano::Node> resolvedNodes;
    Q_FOREACH(const Soprano::Node& node, nodes) {
        if(node.isResource()) {
            resolvedNodes.insert(node, resolveUrl(node.uri()));
        }
        else {
            resolvedNodes.insert(node, node);
        }
    }
    return resolvedNodes;
}

//void Nepomuk::DataManagementModel::insertStatements(const QSet<QUrl> &resources, const QUrl &property, const QSet<Soprano::Node> &values, const QUrl &graph)
//{
//    const QString propertyString = Soprano::Node::resourceToN3(property);

//    QString query = QString::fromLatin1("insert into %1 { ")
//            .arg(Soprano::Node::resourceToN3(graph));

//    foreach(const QUrl& res, resources) {
//        foreach(const Soprano::Node& value, values) {
//            query.append(QString::fromLatin1("%1 %2 %3 . ")
//                        .arg(Soprano::Node::resourceToN3(res),
//                             propertyString,
//                             value.toN3()));
//        }
//    }
//    query.append(QLatin1String("}"));

//    executeQuery(query, Soprano::Query::QueryLanguageSparql);
//}

#include "datamanagementmodel.moc"
