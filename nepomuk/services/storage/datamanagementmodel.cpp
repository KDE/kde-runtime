/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-2011 Sebastian Trueg <trueg@kde.org>
   Copyright (C) 2011 Vishesh Handa <handa.vish@gmail.com>

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
#include "resourceidentifier.h"
#include "simpleresourcegraph.h"
#include "simpleresource.h"
#include "resourcewatchermanager.h"
#include "syncresource.h"
#include "nepomuktools.h"

#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>

#include <Soprano/Graph>
#include <Soprano/QueryResultIterator>
#include <Soprano/StatementIterator>
#include <Soprano/NodeIterator>
#include <Soprano/Error/ErrorCode>
#include <Soprano/Parser>
#include <Soprano/PluginManager>

#include <QtCore/QHash>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>
#include <QtCore/QUuid>
#include <QtCore/QSet>
#include <QtCore/QPair>
#include <QtCore/QFileInfo>

#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Vocabulary/NFO>

#include <KDebug>
#include <KService>
#include <KServiceTypeTrader>
#include <KProtocolInfo>

#include <KIO/NetAccess>

#define STRIGI_INDEX_GRAPH_FOR "http://www.strigi.org/fields#indexGraphFor"

using namespace Nepomuk::Vocabulary;
using namespace Soprano::Vocabulary;

//// TODO: do not allow to create properties or classes through the "normal" methods. Instead provide methods for it.
//// IDEAS:
//// 1. Somehow handle nie:hasPart (at least in describeResources - compare text annotations where we only want to annotate part of a text)

namespace {
    /// convert a hash of URL->URI mappings to N3, omitting empty URIs.
    /// This is a helper for the return type of DataManagementModel::resolveUrls
    QStringList resourceHashToN3(const QHash<QUrl, QUrl>& urls) {
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

    template<typename T> QString createResourceFilter(const T& resources, const QString& var, bool exclude = true) {
        QString filter = QString::fromLatin1("%1 in (%2)").arg(var, Nepomuk::resourcesToN3(resources).join(QLatin1String(",")));
        if(exclude) {
            filter = QString::fromLatin1("!(%1)").arg(filter);
        }
        return filter;
    }

    /*
     * Creates a filter (without the "FILTER" keyword) which either excludes the \p propVar
     * as once of the metadata properties or forces it to be one.
     */
    QString createResourceMetadataPropertyFilter(const QString& propVar, bool exclude = true) {
        return createResourceFilter(QList<QUrl>()
                                    << NAO::created()
                                    << NAO::lastModified()
                                    << NAO::creator()
                                    << NAO::userVisible()
                                    << NIE::url(),
                                    propVar,
                                    exclude);
    }

    enum UriState {
        /// the URI is a URL which points to an existing local file
        ExistingFileUrl,
        /// the URI is a file URL which points to a non-existing local file
        NonExistingFileUrl,
        /// the URI is a URL which uses on of the protocols supported by KIO
        SupportedUrl,
        /// the URI is a nepomuk:/ URI
        NepomukUri,
        /// A uri of the form "_:id"
        BlankUri,
        /// A uri in the ontology
        OntologyUri,
        /// the URI is some other non-supported URI
        OtherUri
    };

    /// Check if a URL points to a local file. This should be the only place where the local file is stat'ed
    inline UriState uriState(const QUrl& uri, bool statLocalFiles = true) {
        if(uri.scheme() == QLatin1String("nepomuk")) {
            return NepomukUri;
        }
        else if(uri.scheme() == QLatin1String("file")) {
            if(!statLocalFiles ||
                    QFile::exists(uri.toLocalFile())) {
                return ExistingFileUrl;
            }
            else {
                return NonExistingFileUrl;
            }
        }
        else if(Nepomuk::ClassAndPropertyTree::self()->contains(uri)) {
            return OntologyUri;
        }
        // if supported by kio
        else if( KProtocolInfo::isKnownProtocol(uri) ) {
            return SupportedUrl;
        }
        else if(uri.toString().startsWith("_:") ) {
            return BlankUri;
        }
        else {
            return OtherUri;
        }
    }

    inline Soprano::Node convertIfBlankUri(const QUrl &uri) {
        if( uri.toString().startsWith(QLatin1String("_:")) )
            return Soprano::Node( uri.toString().mid(2) );
        else
            return Soprano::Node( uri );
    }
}

class Nepomuk::DataManagementModel::Private
{
public:
    ClassAndPropertyTree* m_classAndPropertyTree;
    ResourceWatcherManager* m_watchManager;

    /// a set of properties that are maintained by the service and cannot be changed by clients
    QSet<QUrl> m_protectedProperties;
};

Nepomuk::DataManagementModel::DataManagementModel(Nepomuk::ClassAndPropertyTree* tree, Soprano::Model* model, QObject *parent)
    : Soprano::FilterModel(model),
      d(new Private())
{
    d->m_classAndPropertyTree = tree;
    d->m_watchManager = new ResourceWatcherManager(this);

    setParent(parent);

    // meta data properties are protected. This means they cannot be removed. But they
    // can be set.
    d->m_protectedProperties.insert(NAO::created());
    d->m_protectedProperties.insert(NAO::creator());
    d->m_protectedProperties.insert(NAO::lastModified());
    d->m_protectedProperties.insert(NAO::userVisible());
    d->m_protectedProperties.insert(NIE::url());
}

Nepomuk::DataManagementModel::~DataManagementModel()
{
    delete d;
}


Soprano::Error::ErrorCode Nepomuk::DataManagementModel::updateModificationDate(const QUrl& resource, const QUrl & graph, const QDateTime& date, bool includeCreationDate)
{
    return updateModificationDate(QSet<QUrl>() << resource, graph, date, includeCreationDate);
}


Soprano::Error::ErrorCode Nepomuk::DataManagementModel::updateModificationDate(const QSet<QUrl>& resources, const QUrl & graph, const QDateTime& date, bool includeCreationDate)
{
    if(resources.isEmpty()) {
        return Soprano::Error::ErrorNone;
    }

    QUrl metadataGraph(graph);
    if(metadataGraph.isEmpty()) {
        metadataGraph = createGraph();
    }

    QSet<QUrl> mtimeGraphs;
    Soprano::QueryResultIterator it = executeQuery(QString::fromLatin1("select distinct ?g where { graph ?g { ?r %1 ?d . FILTER(?r in (%2)) . } . }")
                                                   .arg(Soprano::Node::resourceToN3(NAO::lastModified()),
                                                        resourcesToN3(resources).join(QLatin1String(","))),
                                                   Soprano::Query::QueryLanguageSparql);
    while(it.next()) {
        mtimeGraphs << it[0].uri();
    }

    foreach(const QUrl& resource, resources) {
        Soprano::Error::ErrorCode c = removeAllStatements(resource, NAO::lastModified(), Soprano::Node());
        if (c != Soprano::Error::ErrorNone)
            return c;
        addStatement(resource, NAO::lastModified(), Soprano::LiteralValue( date ), metadataGraph);
        if(includeCreationDate && !containsAnyStatement(resource, NAO::created(), Soprano::Node())) {
            addStatement(resource, NAO::created(), Soprano::LiteralValue( date ), metadataGraph);
        }
    }

    removeTrailingGraphs(mtimeGraphs);

    return Soprano::Error::ErrorNone;
}

void Nepomuk::DataManagementModel::addProperty(const QList<QUrl> &resources, const QUrl &property, const QVariantList &values, const QString &app)
{
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
    if(values.isEmpty()) {
        setError(QLatin1String("addProperty: Values needs to be specified."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    if(d->m_protectedProperties.contains(property)) {
        setError(QString::fromLatin1("addProperty: %1 is a protected property which can only be set.").arg(property.toString()),
                 Soprano::Error::ErrorInvalidArgument);
        return;
    }


    //
    // Convert all values to RDF nodes. This includes the property range check and conversion of local file paths to file URLs
    //
    const QSet<Soprano::Node> nodes = d->m_classAndPropertyTree->variantListToNodeSet(values, property);
    if(nodes.isEmpty()) {
        setError(d->m_classAndPropertyTree->lastError());
        return;
    }


    //
    // We need to ensure that no client removes any ontology constructs or graphs,
    // we can check this before resolving file URLs since no protected resource will
    // ever have a nie:url
    //
    if(containsResourceWithProtectedType(QSet<QUrl>::fromList(resources))) {
        return;
    }


    clearError();


    //
    // Hash to keep mapping from provided URL/URI to resource URIs
    //
    QHash<Soprano::Node, Soprano::Node> resolvedNodes;


    if(property == NIE::url()) {
        if(resources.count() != 1) {
            setError(QLatin1String("addProperty: no two resources can have the same nie:url."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
        else if(nodes.count() > 1) {
            setError(QLatin1String("addProperty: One resource can only have one nie:url."), Soprano::Error::ErrorInvalidArgument);
            return;
        }

        if(!nodes.isEmpty()) {
            // check if another resource already uses the URL - no two resources can have the same URL at the same time
            // CAUTION: There is one theoretical situation in which this breaks (more than this actually):
            //          A file is moved and before the nie:url is updated data is added to the file in the new location.
            //          At this point the file is there twice and the data should ideally be merged. But how to decide that
            //          and how to distiguish between that situation and a file overwrite?
            if(containsAnyStatement(Soprano::Node(), NIE::url(), *nodes.constBegin())) {
                setError(QLatin1String("addProperty: No two resources can have the same nie:url at the same time."), Soprano::Error::ErrorInvalidArgument);
                return;
            }
            else if(containsAnyStatement(resources.first(), NIE::url(), Soprano::Node())) {
                // TODO: this can be removed as soon as nie:url gets a max cardinality of 1
                // vHanda: nie:url as of KDE 4.6 has a max cardinality of 1. This is due to the
                //         nepomuk resource identification ontology
                setError(QLatin1String("addProperty: One resource can only have one nie:url."), Soprano::Error::ErrorInvalidArgument);
                return;
            }

            // nie:url is the only property for which we do not want to resolve URLs
            resolvedNodes.insert(*nodes.constBegin(), *nodes.constBegin());
        }
    }
    else {
        resolvedNodes = resolveNodes(nodes);
        if(lastError()) {
            return;
        }
    }


    //
    // Resolve local file URLs (we need to hash the values since we do not want to write anything yet)
    //
    QHash<QUrl, QUrl> uriHash = resolveUrls(resources);
    if(lastError()) {
        return;
    }


    //
    // Check cardinality conditions
    //
    const int maxCardinality = d->m_classAndPropertyTree->maxCardinality(property);
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
    // Special case: setting to the empty list
    //
    if(values.isEmpty()) {
        removeProperties(resources, QList<QUrl>() << property, app);
        return;
    }

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


    //
    // Convert all values to RDF nodes. This includes the property range check and conversion of local file paths to file URLs
    //
    const QSet<Soprano::Node> nodes = d->m_classAndPropertyTree->variantListToNodeSet(values, property);
    if(nodes.isEmpty()) {
        setError(d->m_classAndPropertyTree->lastError());
        return;
    }


    //
    // We need to ensure that no client removes any ontology constructs or graphs,
    // we can check this before resolving file URLs since no protected resource will
    // ever have a nie:url
    //
    if(containsResourceWithProtectedType(QSet<QUrl>::fromList(resources))) {
        return;
    }


    clearError();


    //
    // Hash to keep mapping from provided URL/URI to resource URIs
    //
    QHash<Soprano::Node, Soprano::Node> resolvedNodes;

    //
    // Setting nie:url on file resources also means updating nfo:fileName and possibly nie:isPartOf
    //
    if(property == NIE::url()) {
        if(resources.count() != 1) {
            setError(QLatin1String("setProperty: no two resources can have the same nie:url."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
        else if(nodes.count() > 1) {
            setError(QLatin1String("setProperty: One resource can only have one nie:url."), Soprano::Error::ErrorInvalidArgument);
            return;
        }

        // check if another resource already uses the URL - no two resources can have the same URL at the same time
        // CAUTION: There is one theoretical situation in which this breaks (more than this actually):
        //          A file is moved and before the nie:url is updated data is added to the file in the new location.
        //          At this point the file is there twice and the data should ideally be merged. But how to decide that
        //          and how to distiguish between that situation and a file overwrite?
        if(containsAnyStatement(Soprano::Node(), NIE::url(), *nodes.constBegin())) {
            setError(QLatin1String("setProperty: No two resources can have the same nie:url at the same time."), Soprano::Error::ErrorInvalidArgument);
            return;
        }

        if(updateNieUrlOnLocalFile(resources.first(), nodes.constBegin()->uri())) {
            return;
        }

        // nie:url is the only property for which we do not want to resolve URLs
        resolvedNodes.insert(*nodes.constBegin(), *nodes.constBegin());
    }
    else {
        resolvedNodes = resolveNodes(nodes);
        if(lastError()) {
            return;
        }
    }

    //
    // Resolve local file URLs
    //
    QHash<QUrl, QUrl> uriHash = resolveUrls(resources);
    if(lastError()) {
        return;
    }

    //
    // Remove values that are not wanted anymore
    //
    const QStringList uriHashN3 = resourceHashToN3(uriHash);
    if(!uriHashN3.isEmpty()) {
        const QSet<Soprano::Node> existingValues = QSet<Soprano::Node>::fromList(resolvedNodes.values());
        QSet<QUrl> graphs;
        QList<Soprano::BindingSet> existing
                = executeQuery(QString::fromLatin1("select ?r ?v ?g where { graph ?g { ?r %1 ?v . FILTER(?r in (%2)) . } . }")
                               .arg(Soprano::Node::resourceToN3(property),
                                    uriHashN3.join(QLatin1String(","))),
                               Soprano::Query::QueryLanguageSparql).allBindings();
        Q_FOREACH(const Soprano::BindingSet& binding, existing) {
            if(!existingValues.contains(binding["v"])) {
                removeAllStatements(binding["r"], property, binding["v"]);
                graphs.insert(binding["g"].uri());
                d->m_watchManager->removeProperty(binding["r"], property, binding["v"]);
            }
        }
        removeTrailingGraphs(graphs);
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
    if(values.isEmpty()) {
        setError(QLatin1String("removeProperty: Values needs to be specified."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    if(d->m_protectedProperties.contains(property)) {
        setError(QString::fromLatin1("removeProperty: %1 is a protected property which can only be changed by the data management service itself.").arg(property.toString()),
                 Soprano::Error::ErrorInvalidArgument);
        return;
    }

    const QSet<Soprano::Node> valueNodes = d->m_classAndPropertyTree->variantListToNodeSet(values, property);
    if(valueNodes.isEmpty()) {
        setError(d->m_classAndPropertyTree->lastError());
        return;
    }


    clearError();


    //
    // Resolve file URLs, we can simply ignore the non-existing file resources which are reflected by empty resolved URIs
    //
    QSet<QUrl> resolvedResources = QSet<QUrl>::fromList(resolveUrls(resources).values());
    resolvedResources.remove(QUrl());
    if(resolvedResources.isEmpty() || lastError()) {
        return;
    }

    QSet<Soprano::Node> resolvedNodes = QSet<Soprano::Node>::fromList(resolveNodes(valueNodes).values());
    resolvedNodes.remove(Soprano::Node());
    if(resolvedNodes.isEmpty() || lastError()) {
        return;
    }


    //
    // We need to ensure that no client removes any ontology constructs or graphs
    //
    if(containsResourceWithProtectedType(resolvedResources)) {
        return;
    }


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
            d->m_watchManager->removeProperty( res, property, binding["v"]);
        }

        // we only update the mtime in case we actually remove anything
        if(!valueGraphs.isEmpty()) {
            // If the resource is now empty we remove it completely
            if(!doesResourceExist(res)) {
                removeResources(QList<QUrl>() << res, NoRemovalFlags, app);
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
        else if(d->m_protectedProperties.contains(property)) {
            setError(QString::fromLatin1("removeProperties: %1 is a protected property which can only be changed by the data management service itself.").arg(property.toString()),
                     Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }


    clearError();


    //
    // Resolve file URLs, we can simply ignore the non-existing file resources which are reflected by empty resolved URIs
    //
    QSet<QUrl> resolvedResources = QSet<QUrl>::fromList(resolveUrls(resources).values());
    resolvedResources.remove(QUrl());
    if(resolvedResources.isEmpty() || lastError()) {
        return;
    }


    //
    // We need to ensure that no client removes any ontology constructs or graphs
    //
    if(containsResourceWithProtectedType(resolvedResources)) {
        return;
    }


    //
    // Actually change data
    //
    QUrl mtimeGraph;
    QSet<QUrl> graphs;
    foreach( const QUrl & res, resolvedResources ) {
        QSet<Soprano::Node> propertiesToRemove;
        QList<QPair<Soprano::Node, Soprano::Node> > propertyValues;
        Soprano::QueryResultIterator it
                = executeQuery(QString::fromLatin1("select distinct ?g ?p ?v where { graph ?g { %1 ?p ?v . } . FILTER(?p in (%2)) . }")
                               .arg(Soprano::Node::resourceToN3(res),
                                    resourcesToN3(properties).join(QLatin1String(","))),
                               Soprano::Query::QueryLanguageSparql);
        while(it.next()) {
            graphs.insert(it["g"].uri());
            propertiesToRemove.insert(it["p"]);
            propertyValues.append(qMakePair(it["p"], it["v"]));
        }

        // remove the data
        foreach(const Soprano::Node& property, propertiesToRemove) {
            removeAllStatements( res, property, Soprano::Node() );
        }

        // inform interested parties
        for(QList<QPair<Soprano::Node, Soprano::Node> >::const_iterator it = propertyValues.constBegin();
            it != propertyValues.constEnd(); ++it) {
            d->m_watchManager->removeProperty(res, it->first.uri(), it->second);
        }

        // we only update the mtime in case we actually remove anything
        if(!propertiesToRemove.isEmpty()) {
            // If the resource is now empty we remove it completely
            if(!doesResourceExist(res)) {
                removeResources(QList<QUrl>() << res, NoRemovalFlags, app);
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
    else if(types.isEmpty()) {
        setError(QLatin1String("createResource: No type specified. Cannot create resources without a type."), Soprano::Error::ErrorInvalidArgument);
        return QUrl();
    }
    else {
        foreach(const QUrl& type, types) {
            if(type.isEmpty()) {
                setError(QLatin1String("createResource: Encountered empty type URI."), Soprano::Error::ErrorInvalidArgument);
                return QUrl();
            }
            else if(!d->m_classAndPropertyTree->isKnownClass(type)) {
                setError(QLatin1String("createResource: Encountered invalid type URI."), Soprano::Error::ErrorInvalidArgument);
                return QUrl();
            }
        }
    }

    clearError();

    // create new URIs and a new graph
    const QUrl graph = createGraph(app);
    const QUrl resUri = createUri(ResourceUri);

    // add provided metadata
    foreach(const QUrl& type, types) {
        addStatement(resUri, RDF::type(), type, graph);
    }
    if(!label.isEmpty()) {
        addStatement(resUri, NAO::prefLabel(), Soprano::LiteralValue::createPlainLiteral(label), graph);
    }
    if(!description.isEmpty()) {
        addStatement(resUri, NAO::description(), Soprano::LiteralValue::createPlainLiteral(description), graph);
    }

    // add basic metadata to the new resource
    const QDateTime now = QDateTime::currentDateTime();
    addStatement(resUri, NAO::created(), Soprano::LiteralValue(now), graph);
    addStatement(resUri, NAO::lastModified(), Soprano::LiteralValue(now), graph);

    // inform interested parties
    d->m_watchManager->createResource(resUri, types);

    return resUri;
}

void Nepomuk::DataManagementModel::removeResources(const QList<QUrl> &resources, RemovalFlags flags, const QString &app)
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


    //
    // Resolve file URLs, we can simply ignore the non-existing file resources which are reflected by empty resolved URIs
    //
    QSet<QUrl> resolvedResources = QSet<QUrl>::fromList(resolveUrls(resources).values());
    resolvedResources.remove(QUrl());
    if(resolvedResources.isEmpty() || lastError()) {
        return;
    }


    //
    // We need to ensure that no client removes any ontology constructs or graphs
    //
    if(containsResourceWithProtectedType(resolvedResources)) {
        return;
    }


    clearError();


    //
    // Actually remove the data
    //
    removeAllResources(resolvedResources, flags);
}

void Nepomuk::DataManagementModel::removeDataByApplication(const QList<QUrl> &resources, RemovalFlags flags, const QString &app)
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


    // we handle one special case below: legacy data from the file indexer
    const QUrl appRes = findApplicationResource(app, false);
    if(appRes.isEmpty() && app != QLatin1String("nepomukindexer")) {
        return;
    }


    //
    // Resolve file URLs, we can simply ignore the non-existing file resources which are reflected by empty resolved URIs
    //
    QSet<QUrl> resolvedResources = QSet<QUrl>::fromList(resolveUrls(resources).values());
    resolvedResources.remove(QUrl());
    if(resolvedResources.isEmpty() || lastError()) {
        return;
    }


    //
    // Handle the sub-resources: we can delete all sub-resources of the deleted ones that are entirely defined by our app
    // and are not related by other resources.
    // this has to be done before deleting the resouces in resolvedResources. Otherwise the nao:hasSubResource relationships are already gone!
    //
    // Explanation of the query:
    // The query selects all subresources of the resources in resolvedResources.
    // It then filters out the sub-resources that have properties defined by other apps which are not metadata.
    // It then filters out the sub-resources that are related from other resources that are not the ones being deleted.
    //
    if(flags & RemoveSubResoures && !appRes.isEmpty()) {
        QSet<QUrl> subResources = resolvedResources;
        int resCount = 0;
        do {
            resCount = resolvedResources.count();
            Soprano::QueryResultIterator it
                    = executeQuery(QString::fromLatin1("select ?r where { graph ?g { ?r ?p ?o . } . "
                                                       "?parent %1 ?r . "
                                                       "FILTER(?parent in (%2)) . "
                                                       "?g %3 %4 . "
                                                       "FILTER(!bif:exists((select (1) where { graph ?g2 { ?r ?p2 ?o2 . } . ?g2 %3 ?a2 . FILTER(?a2!=%4) . FILTER(%6) . }))) . "
                                                       "FILTER(!bif:exists((select (1) where { graph ?g2 { ?r2 ?p3 ?r . FILTER(%5) . } . FILTER(!bif:exists((select (1) where { ?x %1 ?r2 . FILTER(?x in (%2)) . }))) . }))) . "
                                                       "}")
                                   .arg(Soprano::Node::resourceToN3(NAO::hasSubResource()),
                                        resourcesToN3(subResources).join(QLatin1String(",")),
                                        Soprano::Node::resourceToN3(NAO::maintainedBy()),
                                        Soprano::Node::resourceToN3(appRes),
                                        createResourceFilter(subResources, QLatin1String("?r2")),
                                        createResourceMetadataPropertyFilter(QLatin1String("?p2"))),
                                   Soprano::Query::QueryLanguageSparql);
            subResources.clear();
            while(it.next()) {
                subResources << it[0].uri();
            }
            resolvedResources += subResources;
        } while(resCount < resolvedResources.count());
    }

    QList<Soprano::BindingSet> graphRemovalCandidates;

    if(!appRes.isEmpty()) {
        //
        // Get the graphs we need to check with removeTrailingGraphs later on.
        // query all graphs the app maintains which contain any information about one of the resources
        // count the number of apps maintaining those graphs (later we will only remove the app as maintainer but keep the graph)
        // count the number of metadata properties defined in those graphs (we will keep that information in case the resource is not fully removed)
        //
        // We combine the results of 2 queries since Virtuoso can optimize FILTER(?r in (...)) but cannot optimize FILTER(?r in (...) || ?o in (...))
        //
        graphRemovalCandidates += executeQuery(QString::fromLatin1("select distinct "
                                                                   "?g "
                                                                   "(select count(distinct ?app) where { ?g %1 ?app . }) as ?c "
                                                                   "where { "
                                                                   "graph ?g { ?r ?p ?o . FILTER(?r in (%3)) . } . "
                                                                   "?g %1 %2 . }")
                                               .arg(Soprano::Node::resourceToN3(NAO::maintainedBy()),
                                                    Soprano::Node::resourceToN3(appRes),
                                                    resourcesToN3(resolvedResources).join(QLatin1String(","))),
                                               Soprano::Query::QueryLanguageSparql).allElements();
        graphRemovalCandidates += executeQuery(QString::fromLatin1("select distinct "
                                                                   "?g "
                                                                   "(select count(distinct ?app) where { ?g %1 ?app . }) as ?c "
                                                                   "where { "
                                                                   "graph ?g { ?r ?p ?o . FILTER(?o in (%3)) . } . "
                                                                   "?g %1 %2 . }")
                                               .arg(Soprano::Node::resourceToN3(NAO::maintainedBy()),
                                                    Soprano::Node::resourceToN3(appRes),
                                                    resourcesToN3(resolvedResources).join(QLatin1String(","))),
                                               Soprano::Query::QueryLanguageSparql).allElements();
    }


    //
    // Handle legacy file indexer data which was stored in graphs marked via a special property
    //
    if(app == QLatin1String("nepomukindexer")) {
        graphRemovalCandidates += executeQuery(QString::fromLatin1("select distinct ?g (0) as ?c where { "
                                                                   "?g <"STRIGI_INDEX_GRAPH_FOR"> ?r . "
                                                                   "FILTER(?r in (%1)) . }")
                                               .arg(Nepomuk::resourcesToN3(resolvedResources).join(QLatin1String(","))),
                                               Soprano::Query::QueryLanguageSparql).allElements();
    }

    //
    // Split the list of graph removal candidates into those we actually remove and those we only stop maintaining
    //
    QSet<QUrl> graphsToRemove;
    QSet<QUrl> graphsToStopMaintaining;
    for(QList<Soprano::BindingSet>::const_iterator it = graphRemovalCandidates.constBegin(); it != graphRemovalCandidates.constEnd(); ++it) {
        if(it->value("c").literal().toInt() <= 1) {
            graphsToRemove << it->value("g").uri();
        }
        else {
            graphsToStopMaintaining << it->value("g").uri();
        }
    }
    // free some mem
    graphRemovalCandidates.clear();


    // the set of resources that we did modify but not remove entirely
    QSet<QUrl> modifiedResources;


    //
    // Fetch all resources that are changed, ie. that are related to the deleted resource in some way.
    // We need to update the mtime of those resources, too.
    //
    if(!appRes.isEmpty()) {
        Soprano::QueryResultIterator relatedResIt = executeQuery(QString::fromLatin1("select distinct ?r where { "
                                                                                     "graph ?g { ?r ?p ?rr . } . "
                                                                                     "?g %1 %2 . "
                                                                                     "FILTER(?rr in (%3)) . "
                                                                                     "FILTER(!(?r in (%3))) . }")
                                                                 .arg(Soprano::Node::resourceToN3(NAO::maintainedBy()),
                                                                      Soprano::Node::resourceToN3(appRes),
                                                                      resourcesToN3(resolvedResources).join(QLatin1String(","))),
                                                                 Soprano::Query::QueryLanguageSparql);
        while(relatedResIt.next()) {
            modifiedResources.insert(relatedResIt[0].uri());
        }
    }


    //
    // remove the resources
    // Other apps might be maintainer, too. In that case only remove the app as a maintainer but keep the data
    //

    //
    // We cannot remove the metadata if the resource is not removed completely.
    // Thus, we cache it and deal with it later. But we only need to cache the metadata from graphs
    // we will actually delete.
    //
    // Tests showed that one query per graph is faster than one query for all graphs!
    //
    Soprano::Graph metadata;
    foreach(const QUrl& g, graphsToRemove) {
        Soprano::QueryResultIterator mdIt
                = executeQuery(QString::fromLatin1("select ?r ?p ?o where { graph %3 { ?r ?p ?o . FILTER(?r in (%1)) . FILTER(%2) . } . }")
                               .arg(resourcesToN3(resolvedResources).join(QLatin1String(",")),
                                    createResourceMetadataPropertyFilter(QLatin1String("?p"), false),
                                    Soprano::Node::resourceToN3(g)),
                               Soprano::Query::QueryLanguageSparql);

        while(mdIt.next()) {
            metadata.addStatement(mdIt["r"], mdIt["p"], mdIt["o"]);
        }
    }


    //
    // Gather the resources that we actually change, ie. those which have non-metadata props in the removed graphs
    //
    Soprano::QueryResultIterator mResIt
            = executeQuery(QString::fromLatin1("select ?r where { graph ?g { ?r ?p ?o . FILTER(?r in (%1)) . FILTER(%2) . } . FILTER(?g in (%3)) . }")
                                        .arg(resourcesToN3(resolvedResources).join(QLatin1String(",")),
                                             createResourceMetadataPropertyFilter(QLatin1String("?p"), true),
                                             resourcesToN3(graphsToRemove).join(QLatin1String(","))),
                                        Soprano::Query::QueryLanguageSparql);
    while(mResIt.next()) {
        modifiedResources.insert(mResIt[0].uri());
    }


    //
    // Remove the actual data. This has to be done using removeAllStatements. Otherwise the crappy inferencer cannot follow the changes.
    //
    foreach(const QUrl& g, graphsToRemove) {
        foreach(const QUrl& res, resolvedResources) {
            removeAllStatements(res, Soprano::Node(), Soprano::Node(), g);
            removeAllStatements(Soprano::Node(), Soprano::Node(), res, g);
        }
    }


    //
    // Remove the app as mainainer from the rest of the graphs
    //
    foreach(const QUrl& g, graphsToStopMaintaining) {
        const int otherCnt = executeQuery(QString::fromLatin1("select count (distinct ?other) where { "
                                                              "graph %1 { ?other ?op ?oo . "
                                                              "FILTER(!(?other in (%2)) && !(?oo in (%2))) . "
                                                              "} . }")
                                          .arg(Soprano::Node::resourceToN3(g),
                                               resourcesToN3(resolvedResources).join(QLatin1String(","))),
                                          Soprano::Query::QueryLanguageSparql).iterateBindings(0).allElements().first().literal().toInt();
        if(otherCnt > 0) {
            //
            // if the graph contains anything else besides the data we want to delete
            // we need to keep the app as maintainer of that data. That is only possible
            // by splitting the graph.
            //
            const QUrl newGraph = splitGraph(g, QUrl(), QUrl());
            Q_ASSERT(!newGraph.isEmpty());

            //
            // we now have two graphs the the same metadata: g and newGraph.
            // we now move the resources we delete into the new graph and only
            // remove the app as maintainer from that graph.
            // (we can use fancy queries since we do not actually change data. Thus
            // the crappy inferencer and other models which act on statement commands
            // are not really interested)
            //
            executeQuery(QString::fromLatin1("insert into %1 { ?r ?p ?o . } where { graph %2 { ?r ?p ?o . FILTER(?r in (%3) || ?o in (%3)) . } . }")
                         .arg(Soprano::Node::resourceToN3(newGraph),
                              Soprano::Node::resourceToN3(g),
                              resourcesToN3(resolvedResources).join(QLatin1String(","))),
                         Soprano::Query::QueryLanguageSparql);
            executeQuery(QString::fromLatin1("delete from %1 { ?r ?p ?o . } where { ?r ?p ?o . FILTER(?r in (%2) || ?o in (%2)) . }")
                         .arg(Soprano::Node::resourceToN3(g),
                              resourcesToN3(resolvedResources).join(QLatin1String(","))),
                         Soprano::Query::QueryLanguageSparql);

            // and finally remove the app as maintainer of the new graph
            removeAllStatements(newGraph, NAO::maintainedBy(), appRes);
        }
        else {
            // we simply remove the app as maintainer of this graph
            removeAllStatements(g, NAO::maintainedBy(), appRes);
        }
    }


    //
    // Determine the resources we did not remove completely.
    // This includes resource that have still other properties than the metadata ones defined
    // and those that have relations from other resources created by other applications and have a nie:url.
    // The latter is a special case which has two reasons:
    // 1. A nie:url identifies the resource even outside of Nepomuk
    // 2. The indexers use this method to update indexed data. If the resource has incoming relations
    //    they should be kept between updates. (The only exception is the legacy index graph relation.)
    //
    QSet<QUrl> resourcesToRemoveCompletely(resolvedResources);
    Soprano::QueryResultIterator resComplIt
            = executeQuery(QString::fromLatin1("select ?r where { ?r ?p ?o . FILTER(?r in (%1)) . FILTER(%2) . }")
                           .arg(resourcesToN3(resolvedResources).join(QLatin1String(",")),
                                createResourceMetadataPropertyFilter(QLatin1String("?p"))),
                           Soprano::Query::QueryLanguageSparql);
    while(resComplIt.next()) {
        resourcesToRemoveCompletely.remove(resComplIt[0].uri());
    }
    resComplIt = executeQuery(QString::fromLatin1("select ?r where { ?o ?p ?r . FILTER(?r in (%1)) . FILTER(?p != <"STRIGI_INDEX_GRAPH_FOR">) . }")
                              .arg(resourcesToN3(resolvedResources).join(QLatin1String(","))),
                              Soprano::Query::QueryLanguageSparql);
    while(resComplIt.next()) {
        const QUrl r = resComplIt[0].uri();
        if(metadata.containsAnyStatement(r, NIE::url(), Soprano::Node())) {
            resourcesToRemoveCompletely.remove(resComplIt[0].uri());
        }
    }

    // no need to update metadata on resource we remove completely
    modifiedResources -= resourcesToRemoveCompletely;


    //
    // Re-add the metadata for the resources that we did not remove entirely
    //
    // 1. use the same time for all mtime changes
    const QDateTime now = QDateTime::currentDateTime();
    // 2. Remove the metadata for the resources we remove completely
    foreach(const QUrl& res, resourcesToRemoveCompletely) {
        metadata.removeAllStatements(res, Soprano::Node(), Soprano::Node());
    }
    // 3. Update the mtime for modifiedResources as those are the only ones we did touch
    foreach(const QUrl& res, modifiedResources) {
        if(metadata.containsAnyStatement(res, NAO::lastModified(), Soprano::Node())) {
            metadata.removeAllStatements(res, NAO::lastModified(), Soprano::Node());
            metadata.addStatement(res, NAO::lastModified(), Soprano::LiteralValue(now));
            modifiedResources.remove(res);
        }
    }
    // 4. add all the updated metadata into a new metadata graph
    if(!metadata.isEmpty() || !modifiedResources.isEmpty()) {
        const QUrl metadataGraph = createGraph();
        if(lastError()) {
            return;
        }

        foreach(const Soprano::Statement& s, metadata.toList()) {
            addStatement(s.subject(), s.predicate(), s.object(), metadataGraph);
        }

        // update mtime of all resources we did touch and not handle above yet
        updateModificationDate(modifiedResources, metadataGraph, now);
    }


    //
    // Remove the resources that are gone completely
    //
    if(!resourcesToRemoveCompletely.isEmpty()){
        removeAllResources(resourcesToRemoveCompletely, flags);
    }

    // make sure we do not leave trailing graphs behind
    removeTrailingGraphs(graphsToRemove);
}

namespace {
struct GraphRemovalCandidate {
    int appCnt;
    QHash<QUrl, int> resources;
};
}

void Nepomuk::DataManagementModel::removeDataByApplication(RemovalFlags flags, const QString &app)
{
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


    //
    // Get the graphs we need to check with removeTrailingGraphs later on.
    // query all graphs the app maintains
    // count the number of apps maintaining those graphs (later we will only remove the app as maintainer but keep the graph)
    // count the number of metadata properties defined in those graphs (we will keep that information in case the resource is not fully removed)
    //
    QHash<QUrl, GraphRemovalCandidate> graphRemovalCandidates;
    Soprano::QueryResultIterator it
            = executeQuery(QString::fromLatin1("select distinct "
                                               "?g ?r "
                                               "(select count(distinct ?app) where { ?g %1 ?app . }) as ?c "
                                               "(select count (*) where { graph ?g { ?r ?mp ?mo . FILTER(%3) . } . }) as ?mc "
                                               "where { "
                                               "graph ?g { ?r ?p ?o . } . "
                                               "?g %1 %2 . "
                                               "}")
                           .arg(Soprano::Node::resourceToN3(NAO::maintainedBy()),
                                Soprano::Node::resourceToN3(appRes),
                                createResourceMetadataPropertyFilter(QLatin1String("?mp"), false)),
                           Soprano::Query::QueryLanguageSparql);
    while(it.next()) {
        GraphRemovalCandidate& g = graphRemovalCandidates[it["g"].uri()];
        g.appCnt = it["c"].literal().toInt();
        g.resources[it["r"].uri()] = it["mc"].literal().toInt();
    }

    QSet<QUrl> allResources;
    QSet<QUrl> graphs;
    const QDateTime now = QDateTime::currentDateTime();
    QUrl mtimeGraph;
    for(QHash<QUrl, GraphRemovalCandidate>::const_iterator it = graphRemovalCandidates.constBegin(); it != graphRemovalCandidates.constEnd(); ++it) {
        const QUrl& g = it.key();
        const int appCnt = it.value().appCnt;
        if(appCnt == 1) {
            //
            // Run through all resources and see what needs to be done
            // TODO: this can surely be improved by moving at least one query
            // one layer above
            //
            for(QHash<QUrl, int>::const_iterator resIt = it->resources.constBegin(); resIt != it->resources.constEnd(); ++resIt) {
                const QUrl& res = resIt.key();
                const int metadataPropCount = resIt.value();
                if(doesResourceExist(res, g)) {
                    //
                    // We cannot remove the metadata if the resource is not removed completely
                    // Thus, we re-add them later on.
                    // trueg: as soon as we have real inference and do not rely on the crappy inferencer to update types via the
                    //        Soprano statement manipulation methods we can use powerful queries like this one:
                    //                    executeQuery(QString::fromLatin1("delete from %1 { %2 ?p ?o . } where { %2 ?p ?o . FILTER(%3) . }")
                    //                                 .arg(Soprano::Node::resourceToN3(g),
                    //                                      Soprano::Node::resourceToN3(res),
                    //                                      createResourceMetadataPropertyFilter(QLatin1String("?p"))),
                    //                                 Soprano::Query::QueryLanguageSparql);
                    //
                    QList<Soprano::BindingSet> metadataProps;
                    if(metadataPropCount > 0) {
                        // remember the metadata props
                        metadataProps = executeQuery(QString::fromLatin1("select ?p ?o where { graph %1 { %2 ?p ?o . FILTER(%3) . } . }")
                                                     .arg(Soprano::Node::resourceToN3(g),
                                                          Soprano::Node::resourceToN3(res),
                                                          createResourceMetadataPropertyFilter(QLatin1String("?p"), false)),
                                                     Soprano::Query::QueryLanguageSparql).allBindings();
                    }

                    removeAllStatements(res, Soprano::Node(), Soprano::Node(), g);
                    removeAllStatements(Soprano::Node(), Soprano::Node(), res, g);

                    foreach(const Soprano::BindingSet& set, metadataProps)  {
                        addStatement(res, set["p"], set["o"], g);
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

                allResources.insert(res);
            }


            graphs.insert(g);
        }
        else {
            // we simply remove the app as maintainer of this graph
            removeAllStatements(g, NAO::maintainedBy(), appRes);
        }
    }


    // make sure we do not leave anything empty trailing around and propery update the mtime
    QList<QUrl> resourcesToRemoveCompletely;
    foreach(const QUrl& res, allResources) {
        if(!doesResourceExist(res)) {
            resourcesToRemoveCompletely << res;
        }
    }
    if(!resourcesToRemoveCompletely.isEmpty()){
        removeResources(resourcesToRemoveCompletely, flags, app);
    }


    removeTrailingGraphs(graphs);
}


//// TODO: do not allow to create properties or classes this way
void Nepomuk::DataManagementModel::storeResources(const Nepomuk::SimpleResourceGraph &resources,
                                                  const QString &app,
                                                  Nepomuk::StoreIdentificationMode identificationMode,
                                                  Nepomuk::StoreResourcesFlags flags,
                                                  const QHash<QUrl, QVariant> &additionalMetadata)
{
    if(app.isEmpty()) {
        setError(QLatin1String("storeResources: Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    if(resources.isEmpty()) {
        clearError();
        return;
    }

    /// Holds the mapping of <file://url> to <nepomuk:/res/> uris
    QHash<QUrl, QUrl> resolvedNodes;

    //
    // Resolve the nie URLs which are present as resource uris
    //
    QSet<QUrl> allNonFileResources;
    SimpleResourceGraph resGraph( resources );
    QList<SimpleResource> resGraphList = resGraph.toList();
    QMutableListIterator<SimpleResource> iter( resGraphList );
    while( iter.hasNext() ) {
        SimpleResource & res = iter.next();

        if( !res.isValid() ) {
            setError(QLatin1String("storeResources: One of the resources is Invalid."), Soprano::Error::ErrorInvalidArgument);
            return;
        }

        const UriState state = uriState(res.uri());
        if(state == NepomukUri || state == BlankUri) {
            allNonFileResources << res.uri();
        }
        // Handle nie urls
        else if(state == NonExistingFileUrl) {
            setError(QString::fromLatin1("Cannot store information about non-existing local files. File '%1' does not exist.").arg(res.uri().toLocalFile()), Soprano::Error::ErrorInvalidArgument);
            return;
        }
        else if(state == ExistingFileUrl || state == SupportedUrl) {
            const QUrl nieUrl = res.uri();
            QUrl newResUri = resolveUrl( nieUrl );
            if( lastError() )
                return;

            if( newResUri.isEmpty() ) {
                // Resolution of one url failed. Assign it a random blank uri
                newResUri = SimpleResource().uri(); // HACK: improveme

                res.addProperty( NIE::url(), nieUrl );
                res.addProperty( RDF::type(), NFO::FileDataObject() );
                if( QFileInfo( nieUrl.toLocalFile() ).isDir() )
                    res.addProperty( RDF::type(), NFO::Folder() );
            }
            resolvedNodes.insert( nieUrl, newResUri );

            res.setUri( newResUri );
        }
        else if( state == OtherUri ) {
            // Legacy support - Sucks but we need it
            const QUrl legacyUri = resolveUrl( res.uri() );
            if( lastError() )
                return;

            allNonFileResources << legacyUri;
        }
        else if( state == OntologyUri ) {
            setError(QLatin1String("It is not allowed to add classes or properties through this API."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }
    resGraph = resGraphList;


    //
    // We need to ensure that no client removes any ontology constructs or graphs
    //
    if(!allNonFileResources.isEmpty() &&
            containsResourceWithProtectedType(allNonFileResources)) {
        return;
    }


    ResourceIdentifier resIdent( identificationMode, this );
    QList<Soprano::Statement> allStatements;
    QList<Sync::SyncResource> extraResources;


    //
    // Resolve URLs in property values and prepare the resource identifier
    //
    foreach( const SimpleResource& res, resGraph.toList() ) {
        // Convert to a Sync::SyncResource
        //
        Sync::SyncResource syncRes( res.uri() ); //vHanda: Will this set the uri properly?
        QHashIterator<QUrl, QVariant> hit( res.properties() );
        while( hit.hasNext() ) {
            hit.next();

            Soprano::Node n = d->m_classAndPropertyTree->variantToNode( hit.value(), hit.key() );
            // The ClassAndPropertyTree returns blank nodes as URIs. It does not understand the
            // concept of blank nodes, and since only storeResources requires blank nodes,
            // it's easier to keep the blank node logic outside.
            if( n.isResource() )
                n = convertIfBlankUri( n.uri() );

            const Soprano::Error::Error error = d->m_classAndPropertyTree->lastError();
            if( error ) {
                setError( error.message(), error.code() );
                return;
            }
            syncRes.insert( hit.key(), n );
        }

        QMutableHashIterator<KUrl, Soprano::Node> it( syncRes );
        while( it.hasNext() ) {
            it.next();

            const Soprano::Node object = it.value();
            if( object.isResource() && it.key() != NIE::url() ) {
                const UriState state = uriState(object.uri());
                if(state==NepomukUri || state==BlankUri || state == OntologyUri) {
                    continue;
                }
                else if(state == NonExistingFileUrl) {
                    setError(QString::fromLatin1("Cannot store information about non-existing local files. File '%1' does not exist.").arg(object.uri().toLocalFile()),
                             Soprano::Error::ErrorInvalidArgument);
                    return;
                }
                else if(state == ExistingFileUrl || state==SupportedUrl) {
                    const QUrl nieUrl = object.uri();
                    // Need to resolve it
                    QHash< QUrl, QUrl >::const_iterator findIter = resolvedNodes.constFind( nieUrl );
                    if( findIter != resolvedNodes.constEnd() ) {
                        it.setValue( convertIfBlankUri(findIter.value()) );
                    }
                    else {
                        Sync::SyncResource newRes;

                        // It doesn't exist, create it
                        QUrl resolvedUri = resolveUrl( nieUrl );
                        if( resolvedUri.isEmpty() ) {
                            resolvedUri = SimpleResource().uri(); // HACK: improveme

                            newRes.insert( RDF::type(), NFO::FileDataObject() );
                            newRes.insert( NIE::url(), nieUrl );
                            if( QFileInfo( nieUrl.toLocalFile() ).isDir() )
                                newRes.insert( RDF::type(), NFO::Folder() );
                        }

                        newRes.setUri( resolvedUri );
                        extraResources.append( newRes );

                        resolvedNodes.insert( nieUrl, resolvedUri );
                        it.setValue( convertIfBlankUri(resolvedUri) );
                    }
                }
                else if(state == OtherUri) {
                    // We use resolveUrl to check if the otherUri exists. If it doesn't exist,
                    // then resolveUrl which set the last error
                    const QUrl legacyUri = resolveUrl( object.uri() );
                    if( lastError() )
                        return;

                    // It apparently exists, so we must support it
                }
            }
        } // while( it.hasNext() )

        // The resource is now ready.
        // Push it into the Resource Identifier
        QList< Soprano::Statement > stList = syncRes.toStatementList();
        allStatements << stList;

        if(stList.isEmpty()) {
            setError(d->m_classAndPropertyTree->lastError());
            return;
        }

        if( !syncRes.isValid() ) {
            setError(QLatin1String("storeResources: Contains invalid resources."), Soprano::Error::ErrorParsingFailed);
            return;
        }
        resIdent.addSyncResource( syncRes );
    }


    //
    // Check the created statements
    //
    foreach(const Soprano::Statement& s, allStatements) {
        if(!s.isValid()) {
            kDebug() << "Invalid statement after resource conversion:" << s;
            setError(QLatin1String("storeResources: Encountered invalid statement after resource conversion."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }

    //
    // For better identification add rdf:type and nie:url to resolvedNodes
    //
    QHashIterator<QUrl, QUrl> it( resolvedNodes );
    while( it.hasNext() ) {
        it.next();

        const QUrl fileUrl = it.key();
        const QUrl uri = it.value();

        const Sync::SyncResource existingRes = resIdent.simpleResource( uri );

        Sync::SyncResource res;
        res.setUri( uri );

        if( !existingRes.contains( RDF::type(), NFO::FileDataObject() ) )
            res.insert( RDF::type(), NFO::FileDataObject() );

        if( !existingRes.contains( NIE::url(), fileUrl ) )
            res.insert( NIE::url(), fileUrl );

        if( QFileInfo( fileUrl.toString() ).isDir() && !existingRes.contains( RDF::type(), NFO::Folder() ) )
            res.insert( RDF::type(), NFO::Folder() );

        resIdent.addSyncResource( res );
    }

    clearError();


    //
    // Perform the actual identification
    //
    resIdent.identifyAll();

    if( resIdent.mappings().empty() ) {
        kDebug() << "Nothing was mapped merging everything as it is.";
    }

    foreach( const Sync::SyncResource & res, extraResources ) {
        allStatements << res.toStatementList();
    }

    ResourceMerger merger( this, app, additionalMetadata, flags );
    merger.setMappings( resIdent.mappings() );
    if( !merger.merge( Soprano::Graph(allStatements) ) ) {
        kDebug() << " MERGING FAILED! ";
        kDebug() << "Setting error!" << merger.lastError();
        setError( merger.lastError() );
    }
}

void Nepomuk::DataManagementModel::importResources(const QUrl &url,
                                                   const QString &app,
                                                   Soprano::RdfSerialization serialization,
                                                   const QString &userSerialization,
                                                   Nepomuk::StoreIdentificationMode identificationMode,
                                                   Nepomuk::StoreResourcesFlags flags,
                                                   const QHash<QUrl, QVariant>& additionalMetadata)
{
    // download the file
    QString tmpFileName;
    if(!KIO::NetAccess::download(url, tmpFileName, 0)) {
        setError(QString::fromLatin1("Failed to download '%1'.").arg(url.toString()));
        return;
    }

    // guess the serialization
    if(serialization == Soprano::SerializationUnknown) {
        const QString extension = KUrl(url).fileName().section('.', -1).toLower();
        if(extension == QLatin1String("trig"))
            serialization = Soprano::SerializationTrig;
        else if(extension == QLatin1String("n3"))
            serialization = Soprano::SerializationNTriples;
        else if(extension == QLatin1String("xml"))
            serialization = Soprano::SerializationRdfXml;
    }

    const Soprano::Parser* parser = Soprano::PluginManager::instance()->discoverParserForSerialization(serialization, userSerialization);
    if(!parser) {
        setError(QString::fromLatin1("Failed to create parser for serialization '%1'").arg(Soprano::serializationMimeType(serialization, userSerialization)));
    }
    else {
        SimpleResourceGraph graph;
        Soprano::StatementIterator it = parser->parseFile(tmpFileName, QUrl(), serialization, userSerialization);
        while(it.next()) {
            graph.addStatement(*it);
        }
        if(parser->lastError()) {
            setError(parser->lastError());
        }
        else if(it.lastError()) {
            setError(it.lastError());
        }
        else {
            storeResources(graph, app, identificationMode, flags, additionalMetadata);
        }
    }

    KIO::NetAccess::removeTempFile(tmpFileName);
}

void Nepomuk::DataManagementModel::mergeResources(const QUrl &res1, const QUrl &res2, const QString &app)
{
    if(res1.isEmpty() || res2.isEmpty()) {
        setError(QLatin1String("mergeResources: Encountered empty resource URI."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    if(app.isEmpty()) {
        setError(QLatin1String("mergeResources: Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }

    //
    // We need to ensure that no client removes any ontology constructs or graphs,
    // we can check this before resolving file URLs since no protected resource will
    // ever have a nie:url
    //
    if(containsResourceWithProtectedType(QSet<QUrl>() << res1 << res2)) {
        return;
    }

    clearError();


    // TODO: Is it correct that all metadata stays the same?

    //
    // Copy all property values of res2 that are not also defined for res1
    //
    const QList<Soprano::BindingSet> res2Properties
            = executeQuery(QString::fromLatin1("select ?g ?p ?v (select count(distinct ?v2) where { %2 ?p ?v2 . }) as ?c where { "
                                               "graph ?g { %1 ?p ?v . } . "
                                               "FILTER(!bif:exists((select (1) where { %2 ?p ?v . }))) . "
                                               "}")
                           .arg(Soprano::Node::resourceToN3(res2),
                                Soprano::Node::resourceToN3(res1)),
                           Soprano::Query::QueryLanguageSparql).allBindings();
    foreach(const Soprano::BindingSet& binding, res2Properties) {
        const QUrl& prop = binding["p"].uri();
        // if the property has no cardinality of 1 or no value is defined yet we can simply convert the value to res1
        if(d->m_classAndPropertyTree->maxCardinality(prop) != 1 ||
                binding["c"].literal().toInt() == 0) {
            addStatement(res1, prop, binding["v"], binding["g"]);
        }
    }


    //
    // Copy all backlinks from res2 that are not valid for res1
    //
    const QList<Soprano::BindingSet> res2Backlinks
            = executeQuery(QString::fromLatin1("select ?g ?p ?r where { graph ?g { ?r ?p %1 . } . "
                                               "FILTER(?r!=%2) . "
                                               "FILTER(!bif:exists((select (1) where { ?r ?p %2 . }))) . "
                                               "}")
                           .arg(Soprano::Node::resourceToN3(res2),
                                Soprano::Node::resourceToN3(res1)),
                           Soprano::Query::QueryLanguageSparql).allBindings();
    foreach(const Soprano::BindingSet& binding, res2Backlinks) {
        addStatement(binding["r"], binding["p"], res1, binding["g"]);
    }


    //
    // Finally delete res2 as it is now merged into res1
    //
    removeResources(QList<QUrl>() << res2, NoRemovalFlags, app);
}


namespace {
QVariant nodeToVariant(const Soprano::Node& node) {
    if(node.isResource())
        return node.uri();
    else if(node.isBlank())
        return QUrl(QLatin1String("_:") + node.identifier());
    else
        return node.literal().variant();
}
}

Nepomuk::SimpleResourceGraph Nepomuk::DataManagementModel::describeResources(const QList<QUrl> &resources, bool includeSubResources) const
{
    kDebug() << resources << includeSubResources;

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
    // Split into nie:urls and URIs so we can get all data in one query
    //
    QSet<QUrl> nieUrls;
    QSet<QUrl> resUris;
    foreach( const QUrl & res, resources ) {
        const UriState state = uriState(res);
        if(state == NonExistingFileUrl) {
            setError(QString::fromLatin1("Cannot store information about non-existing local files. File '%1' does not exist.").arg(res.toLocalFile()),
                     Soprano::Error::ErrorInvalidArgument);
            return SimpleResourceGraph();
        }
        else if(state == ExistingFileUrl || state == SupportedUrl) {
            nieUrls.insert(res);
        }
        else if(state == NepomukUri || state == OtherUri){
            resUris.insert(res);
        }
        else if(state == BlankUri) {
            setError(QString::fromLatin1("Cannot give information about blank uris. '%1' does not exist").arg(res.toString()),
                     Soprano::Error::ErrorInvalidArgument);
            return SimpleResourceGraph();
        }
    }


    //
    // Build the query
    //
    QStringList terms;
    if(!nieUrls.isEmpty()) {
        terms << QString::fromLatin1("?s ?p ?o . ?s %1 ?u . FILTER(?u in (%2)) . ")
                 .arg(Soprano::Node::resourceToN3(Vocabulary::NIE::url()),
                      resourcesToN3(nieUrls).join(QLatin1String(",")));
        if(includeSubResources) {
            terms << QString::fromLatin1("?s ?p ?o . ?r %1 ?s . ?r %2 ?u . FILTER(?u in (%3)) . ")
                     .arg(Soprano::Node::resourceToN3(NAO::hasSubResource()),
                          Soprano::Node::resourceToN3(Vocabulary::NIE::url()),
                          resourcesToN3(nieUrls).join(QLatin1String(",")));
        }
    }
    if(!resUris.isEmpty()) {
        terms << QString::fromLatin1("?s ?p ?o . FILTER(?s in (%1)) . ")
                 .arg(resourcesToN3(resUris).join(QLatin1String(",")));
        if(includeSubResources) {
            terms << QString::fromLatin1("?s ?p ?o . ?r %1 ?s . FILTER(?r in (%2)) . ")
                     .arg(Soprano::Node::resourceToN3(NAO::hasSubResource()),
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
        graph[r].addProperty(it["p"].uri(), nodeToVariant(it["o"]));
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

#ifndef NDEBUG
    if(graphs.contains(QUrl()))
        kDebug() << "Encountered the empty graph. This most likely stems from buggy data which was generated by some older (hopefully) version of Nepomuk. Ignoring.";
#endif

    QSet<QUrl> gl(graphs);
    gl.remove(QUrl());

    if(!gl.isEmpty()) {
        // TODO: for some weird reason the filter statement in the query below does not work. Thus, we use a count instead.
        QList<Soprano::Node> allMetadataGraphs;
//                = executeQuery(QString::fromLatin1("select ?mg where { ?mg %1 ?g . FILTER(?g in (%2)) . FILTER(!bif:exists((select (1) where { graph ?g { ?s ?p ?o . } . }))) . }")
//                               .arg(Soprano::Node::resourceToN3(NRL::coreGraphMetadataFor()),
//                                    resourcesToN3(gl).join(QLatin1String(","))),
//                               Soprano::Query::QueryLanguageSparql).iterateBindings(0).allNodes();

        Soprano::QueryResultIterator it
                = executeQuery(QString::fromLatin1("select ?mg (select count(*) where { graph ?g { ?s ?p ?o . } . }) as ?cnt where { ?mg %1 ?g . FILTER(?g in (%2)) . }")
                               .arg(Soprano::Node::resourceToN3(NRL::coreGraphMetadataFor()),
                                    resourcesToN3(gl).join(QLatin1String(","))),
                               Soprano::Query::QueryLanguageSparql);
        while(it.next()) {
            if(it[1].literal().toInt() == 0)
                allMetadataGraphs << it[0];
        }

        foreach(const Soprano::Node& mg, allMetadataGraphs) {
            executeQuery(QString::fromLatin1("clear graph %1").arg(mg.toN3()),
                         Soprano::Query::QueryLanguageSparql);
        }
    }
}


QUrl Nepomuk::DataManagementModel::createGraph(const QString &app, const QHash<QUrl, QVariant> &additionalMetadata)
{
    QHash<QUrl, Soprano::Node> graphMetaData;

    for(QHash<QUrl, QVariant>::const_iterator it = additionalMetadata.constBegin();
        it != additionalMetadata.constEnd(); ++it) {
        Soprano::Node node = d->m_classAndPropertyTree->variantToNode(it.value(), it.key());
        if(node.isValid()) {
            graphMetaData.insert(it.key(), node);
        }
        else {
            setError(d->m_classAndPropertyTree->lastError());
            return QUrl();
        }
    }

    return createGraph( app, graphMetaData );
}

QUrl Nepomuk::DataManagementModel::createGraph(const QString& app, const QMultiHash< QUrl, Soprano::Node >& additionalMetadata)
{
    QHash<QUrl, Soprano::Node> graphMetaData = additionalMetadata;

    // determine the graph type
    bool haveGraphType = false;
    for(QHash<QUrl, Soprano::Node>::const_iterator it = additionalMetadata.constBegin();
        it != additionalMetadata.constEnd(); ++it) {
        const QUrl& property = it.key();

        if(property == RDF::type()) {
            // check if it is a valid type
            if(!it.value().isResource()) {
                setError(QString::fromLatin1("rdf:type has resource range. '%1' does not have a resource type.").arg(it.value().toN3()), Soprano::Error::ErrorInvalidArgument);
                return QUrl();
            }
            else {
                if(d->m_classAndPropertyTree->isChildOf(it.value().uri(), NRL::Graph()))
                    haveGraphType = true;
            }
        }

        else if(property == NAO::created()) {
            if(!it.value().literal().isDateTime()) {
                setError(QString::fromLatin1("nao:created has xsd:dateTime range. '%1' is not convertable to a dateTime.").arg(it.value().toN3()), Soprano::Error::ErrorInvalidArgument);
                return QUrl();
            }
        }

        else {
            // FIXME: check property, domain, and range
            // Reuse code from ResourceMerger::checkGraphMetadata
        }
    }

    // add missing metadata
    if(!haveGraphType) {
        graphMetaData.insert(RDF::type(), NRL::InstanceBase());
    }
    if(!graphMetaData.contains(NAO::created())) {
        graphMetaData.insert(NAO::created(), Soprano::LiteralValue(QDateTime::currentDateTime()));
    }
    if(!graphMetaData.contains(NAO::maintainedBy()) && !app.isEmpty()) {
        graphMetaData.insert(NAO::maintainedBy(), findApplicationResource(app));
    }

    const QUrl graph = createUri(GraphUri);
    const QUrl metadatagraph = createUri(GraphUri);

    // add metadata graph itself
    addStatement(metadatagraph, NRL::coreGraphMetadataFor(), graph, metadatagraph);
    addStatement(metadatagraph, RDF::type(), NRL::GraphMetadata(), metadatagraph);

    for(QHash<QUrl, Soprano::Node>::const_iterator it = graphMetaData.constBegin();
        it != graphMetaData.constEnd(); ++it) {
        addStatement(graph, it.key(), it.value(), metadatagraph);
    }

    return graph;
}


QUrl Nepomuk::DataManagementModel::splitGraph(const QUrl &graph, const QUrl& metadataGraph_, const QUrl &appRes)
{
    const QUrl newGraph = createUri(GraphUri);
    const QUrl newMetadataGraph = createUri(GraphUri);

    QUrl metadataGraph(metadataGraph_);
    if(metadataGraph.isEmpty()) {
        Soprano::QueryResultIterator it = executeQuery(QString::fromLatin1("select ?m where { ?m %1 %2 . } LIMIT 1")
                                                       .arg(Soprano::Node::resourceToN3(NRL::coreGraphMetadataFor()),
                                                            Soprano::Node::resourceToN3(graph)),
                                                       Soprano::Query::QueryLanguageSparql);
        if(it.next()) {
            metadataGraph = it[0].uri();
        }
        else {
            kError() << "Failed to get metadata graph for" << graph;
            return QUrl();
        }
    }

    // add metadata graph
    addStatement( newMetadataGraph, NRL::coreGraphMetadataFor(), newGraph, newMetadataGraph );
    addStatement( newMetadataGraph, RDF::type(), NRL::GraphMetadata(), newMetadataGraph );

    // copy the metadata from the old graph to the new one
    executeQuery(QString::fromLatin1("insert into %1 { %2 ?p ?o . } where { graph %3 { %4 ?p ?o . } . }")
                 .arg(Soprano::Node::resourceToN3(newMetadataGraph),
                      Soprano::Node::resourceToN3(newGraph),
                      Soprano::Node::resourceToN3(metadataGraph),
                      Soprano::Node::resourceToN3(graph)),
                 Soprano::Query::QueryLanguageSparql);

    // add the new app
    if(!appRes.isEmpty())
        addStatement(newGraph, NAO::maintainedBy(), appRes, newMetadataGraph);

    return newGraph;
}


QUrl Nepomuk::DataManagementModel::findApplicationResource(const QString &app, bool create)
{
    Soprano::QueryResultIterator it =
            executeQuery(QString::fromLatin1("select ?r where { ?r a %1 . ?r %2 %3 . } LIMIT 1")
                         .arg(Soprano::Node::resourceToN3(NAO::Agent()),
                              Soprano::Node::resourceToN3(NAO::identifier()),
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
        addStatement( metadatagraph, NRL::coreGraphMetadataFor(), graph, metadatagraph );
        addStatement( metadatagraph, RDF::type(), NRL::GraphMetadata(), metadatagraph );
        addStatement( graph, RDF::type(), NRL::InstanceBase(), metadatagraph );
        addStatement( graph, NAO::created(), Soprano::LiteralValue(QDateTime::currentDateTime()), metadatagraph );

        // the app itself
        addStatement( uri, RDF::type(), NAO::Agent(), graph );
        addStatement( uri, NAO::identifier(), Soprano::LiteralValue(app), graph );

        KService::List services = KServiceTypeTrader::self()->query(QLatin1String("Application"),
                                                                    QString::fromLatin1("DesktopEntryName=='%1'").arg(app));
        if(services.count() == 1) {
            addStatement(uri, NAO::prefLabel(), Soprano::LiteralValue(services.first()->name()), graph);
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


// TODO: emit resource watcher resource creation signals
void Nepomuk::DataManagementModel::addProperty(const QHash<QUrl, QUrl> &resources, const QUrl &property, const QHash<Soprano::Node, Soprano::Node> &nodes, const QString &app)
{
    kDebug() << resources << property << nodes << app;
    Q_ASSERT(!resources.isEmpty());
    Q_ASSERT(!nodes.isEmpty());
    Q_ASSERT(!property.isEmpty());

    //
    // Check cardinality conditions
    //
    const int maxCardinality = d->m_classAndPropertyTree->maxCardinality(property);
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
            const QUrl uri = createUri(ResourceUri);
            addStatement(uri, Vocabulary::NIE::url(), it.key(), graph);
            if(it.key().uri().scheme() == QLatin1String("file")) {
                addStatement(uri, RDF::type(), NFO::FileDataObject(), graph);
            }
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
            if(it.key().scheme() == QLatin1String("file")) {
                addStatement(uri, RDF::type(), NFO::FileDataObject(), graph);
            }
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
                .arg(Soprano::Node::resourceToN3(NRL::coreGraphMetadataFor()),
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
            if(containsStatement(g, NAO::maintainedBy(), appRes, m)) {
                continue;
            }
            else if(cnt == 1) {
                // we can reuse the graph
                addStatement(g, NAO::maintainedBy(), appRes, m);
            }
            else {
                // we need to split the graph
                // FIXME: do not split the same graph again and again. Check if the graph in question already is the one we created.
                const QUrl newGraph = splitGraph(g, m, appRes);

                // and finally move the actual property over to the new graph
                removeStatement(r, property, v, g);
                addStatement(r, property, v, newGraph);
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
            addStatement(it->first, property, it->second, graph);
            finalResources.insert(it->first);
            d->m_watchManager->addProperty( it->first, property, it->second);
        }

        // update modification date
        Q_FOREACH(const QUrl& res, finalResources) {
            updateModificationDate(res, graph, QDateTime::currentDateTime(), true);
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
        const QUrl resolved = resolveUrl(url, true);
        if(url.isEmpty() && lastError()) {
            break;
        }
        uriHash.insert(url, resolved);
    }
    return uriHash;
}

QUrl Nepomuk::DataManagementModel::resolveUrl(const QUrl &url, bool statLocalFiles) const
{
    const UriState state = uriState( url, statLocalFiles );

    if( state == NepomukUri || state == OntologyUri ) {
        // nothing to resolve over here
        return url;
    }

    //
    // First check if the URL does exists as resource URI
    //
    else if( executeQuery(QString::fromLatin1("ask where { %1 ?p ?o . }")
                     .arg(Soprano::Node::resourceToN3(url)),
                     Soprano::Query::QueryLanguageSparql).boolValue() ) {
        return url;
    }

    //
    // we resolve all URLs except nepomuk:/ URIs. While the DMS does only use nie:url
    // on local files libnepomuk used to use it for everything but nepomuk:/ URIs.
    // Thus, we need to handle that legacy data by checking if url does exist as nie:url
    //
    else {
        Soprano::QueryResultIterator it
                = executeQuery(QString::fromLatin1("select ?r where { ?r %1 %2 . } limit 1")
                               .arg(Soprano::Node::resourceToN3(NIE::url()),
                                    Soprano::Node::resourceToN3(url)),
                               Soprano::Query::QueryLanguageSparql);

        // if the URL is used as a nie:url return the corresponding resource URI
        if(it.next()) {
            return it[0].uri();
        }

        // non-existing unsupported URL
        else if( state == OtherUri ) {
            QString error = QString::fromLatin1("Unknown protocol '%1' encountered in %2")
                            .arg(url.scheme(), url.toString());
            setError(error, Soprano::Error::ErrorInvalidArgument);
            return QUrl();
        }

        // if there is no existing URI return an empty match for local files (since we do always want to use nie:url here)
        else {
            // we only throw an error if the file:/ URL points to a non-existing file AND it does not exist in the database.
            if(state == NonExistingFileUrl) {
                setError(QString::fromLatin1("Cannot store information about non-existing local files. File '%1' does not exist.").arg(url.toLocalFile()),
                         Soprano::Error::ErrorInvalidArgument);
            }

            return QUrl();
        }
    }

    // fallback
    return url;
}

QHash<Soprano::Node, Soprano::Node> Nepomuk::DataManagementModel::resolveNodes(const QSet<Soprano::Node> &nodes) const
{
    QHash<Soprano::Node, Soprano::Node> resolvedNodes;
    Q_FOREACH(const Soprano::Node& node, nodes) {
        if(node.isResource()) {
            resolvedNodes.insert(node, resolveUrl(node.uri(), true));
        }
        else {
            resolvedNodes.insert(node, node);
        }
    }
    return resolvedNodes;
}

bool Nepomuk::DataManagementModel::updateNieUrlOnLocalFile(const QUrl &resource, const QUrl &nieUrl)
{
    kDebug() << resource << "->" << nieUrl;

    //
    // Since KDE 4.4 we use nepomuk:/res/<UUID> Uris for all resources including files. Thus, moving a file
    // means updating two things:
    // 1. the nie:url property
    // 2. the nie:isPartOf relation (only necessary if the file and not the whole folder was moved)
    //

    //
    // Now update the nie:url, nfo:fileName, and nie:isPartOf relations.
    //
    // We do NOT use setProperty to avoid the overhead and data clutter of creating
    // new metadata graphs for the changed data.
    //

    // remember for url, filename, and parent the graph they are defined in
    QUrl resUri, oldNieUrl, oldNieUrlGraph, oldParentResource, oldParentResourceGraph, oldFileNameGraph;
    QString oldFileName;

    // we do not use isLocalFileUrl() here since we also handle already moved files
    if(resource.scheme() == QLatin1String("file")) {
        oldNieUrl = resource;
        Soprano::QueryResultIterator it
                = executeQuery(QString::fromLatin1("select distinct ?gu ?gf ?gp ?r ?f ?p where { "
                                                   "graph ?gu { ?r %2 %1 . } . "
                                                   "OPTIONAL { graph ?gf { ?r %3 ?f . } . } . "
                                                   "OPTIONAL { graph ?gp { ?r %4 ?p . } . } . "
                                                   "} LIMIT 1")
                               .arg(Soprano::Node::resourceToN3(resource),
                                    Soprano::Node::resourceToN3(NIE::url()),
                                    Soprano::Node::resourceToN3(NFO::fileName()),
                                    Soprano::Node::resourceToN3(NIE::isPartOf())),
                               Soprano::Query::QueryLanguageSparql);
        if(it.next()) {
            resUri= it["r"].uri();
            oldNieUrlGraph = it["gu"].uri();
            oldParentResource = it["p"].uri();
            oldParentResourceGraph = it["gp"].uri();
            oldFileName = it["f"].toString();
            oldFileNameGraph = it["gf"].uri();
        }
    }
    else {
        resUri = resource;
        Soprano::QueryResultIterator it
                = executeQuery(QString::fromLatin1("select distinct ?gu ?gf ?gp ?u ?f ?p where { "
                                                   "graph ?gu { %1 %2 ?u . } . "
                                                   "OPTIONAL { graph ?gf { %1 %3 ?f . } . } . "
                                                   "OPTIONAL { graph ?gp { %1 %4 ?p . } . } . "
                                                   "} LIMIT 1")
                               .arg(Soprano::Node::resourceToN3(resource),
                                    Soprano::Node::resourceToN3(NIE::url()),
                                    Soprano::Node::resourceToN3(NFO::fileName()),
                                    Soprano::Node::resourceToN3(NIE::isPartOf())),
                               Soprano::Query::QueryLanguageSparql);
        if(it.next()) {
            oldNieUrl = it["u"].uri();
            oldNieUrlGraph = it["gu"].uri();
            oldParentResource = it["p"].uri();
            oldParentResourceGraph = it["gp"].uri();
            oldFileName = it["f"].toString();
            oldFileNameGraph = it["gf"].uri();
        }
    }

    if (!oldNieUrlGraph.isEmpty()) {
        removeStatement(resUri, NIE::url(), oldNieUrl, oldNieUrlGraph);
        addStatement(resUri, NIE::url(), nieUrl, oldNieUrlGraph);

        if (!oldFileNameGraph.isEmpty()) {
            // we only update the filename if it actually changed
            if(KUrl(oldNieUrl).fileName() != KUrl(nieUrl).fileName()) {
                removeStatement(resUri, NFO::fileName(), Soprano::LiteralValue(oldFileName), oldFileNameGraph);
                addStatement(resUri, NFO::fileName(), Soprano::LiteralValue(KUrl(nieUrl).fileName()), oldFileNameGraph);
            }
        }

        if (!oldParentResourceGraph.isEmpty()) {
            // we only update the parent folder if it actually changed
            const KUrl nieUrlParent = KUrl(nieUrl).directory(KUrl::IgnoreTrailingSlash);
            const KUrl oldUrlParent = KUrl(oldNieUrl).directory(KUrl::IgnoreTrailingSlash);
            if(nieUrlParent != oldUrlParent) {
                removeStatement(resUri, NIE::isPartOf(), oldParentResource, oldParentResourceGraph);
                const QUrl newParentRes = resolveUrl(nieUrlParent);
                if (!newParentRes.isEmpty()) {
                    addStatement(resUri, NIE::isPartOf(), newParentRes, oldParentResourceGraph);
                }
            }
        }

        //
        // Update all children
        // We only need to update the nie:url properties. Filenames and nie:isPartOf relations cannot change
        // due to a renaming of the parent folder.
        //
        // CAUTION: The trailing slash on the from URL is essential! Otherwise we might match the newly added
        //          URLs, too (in case a rename only added chars to the name)
        //
        const QString query = QString::fromLatin1("select distinct ?r ?u ?g where { "
                                                  "graph ?g { ?r %1 ?u . } . "
                                                  "FILTER(REGEX(STR(?u),'^%2')) . "
                                                  "}")
                .arg(Soprano::Node::resourceToN3(NIE::url()),
                     KUrl(oldNieUrl).url(KUrl::AddTrailingSlash));

        const QString oldBasePath = KUrl(oldNieUrl).path(KUrl::AddTrailingSlash);
        const QString newBasePath = KUrl(nieUrl).path(KUrl::AddTrailingSlash);

        //
        // We cannot use one big loop since our updateMetadata calls below can change the iterator
        // which could have bad effects like row skipping. Thus, we handle the urls in chunks of
        // cached items.
        //
        forever {
            const QList<Soprano::BindingSet> urls = executeQuery(query + QLatin1String( " LIMIT 500" ),
                                                                 Soprano::Query::QueryLanguageSparql)
                    .allBindings();
            if (urls.isEmpty())
                break;

            for (int i = 0; i < urls.count(); ++i) {
                const KUrl u = urls[i]["u"].uri();
                const QUrl r = urls[i]["r"].uri();
                const QUrl g = urls[i]["g"].uri();

                // now construct the new URL
                const QString oldRelativePath = u.path().mid(oldBasePath.length());
                const KUrl newUrl(newBasePath + oldRelativePath);

                removeStatement(r, NIE::url(), u, g);
                addStatement(r, NIE::url(), newUrl, g);
            }
        }

        return true;
    }
    else {
        // no old nie:url found
        return false;
    }
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

Nepomuk::ClassAndPropertyTree* Nepomuk::DataManagementModel::classAndPropertyTree()
{
    return d->m_classAndPropertyTree;
}

bool Nepomuk::DataManagementModel::containsResourceWithProtectedType(const QSet<QUrl> &resources) const
{
    if(executeQuery(QString::fromLatin1("ask where { ?r a ?t . FILTER(?r in (%1)) . FILTER(?t in (%2,%3,%4)) . }")
            .arg(resourcesToN3(resources).join(QLatin1String(",")),
                 Soprano::Node::resourceToN3(RDFS::Class()),
                 Soprano::Node::resourceToN3(RDF::Property()),
                 Soprano::Node::resourceToN3(NRL::Graph())),
                    Soprano::Query::QueryLanguageSparql).boolValue()) {
        setError(QLatin1String("It is not allowed to remove classes, properties, or graphs through this API."), Soprano::Error::ErrorInvalidArgument);
        return true;
    }
    else {
        return false;
    }
}

Nepomuk::ResourceWatcherManager* Nepomuk::DataManagementModel::resourceWatcherManager() const
{
    return d->m_watchManager;
}

void Nepomuk::DataManagementModel::removeAllResources(const QSet<QUrl> &resources, RemovalFlags flags)
{
    QSet<QUrl> resolvedResources(resources);

    //
    // Handle the sub-resources:
    // this has to be done before deleting the resouces in resolvedResources. Otherwise the nao:hasSubResource relationships are already gone!
    //
    // Explanation of the query:
    // The query selects all subresources of the resources in resolvedResources.
    // It then filters out the sub-resources that are related from other resources that are not the ones being deleted.
    //
    if(flags & RemoveSubResoures) {
        QSet<QUrl> subResources = resolvedResources;
        int resCount = 0;
        do {
            resCount = resolvedResources.count();
            Soprano::QueryResultIterator it
                    = executeQuery(QString::fromLatin1("select ?r where { ?r ?p ?o . "
                                                       "?parent %1 ?r . "
                                                       "FILTER(?parent in (%2)) . "
                                                       "FILTER(!bif:exists((select (1) where { ?r2 ?p3 ?r . FILTER(%3) . FILTER(!bif:exists((select (1) where { ?x %1 ?r2 . FILTER(?x in (%2)) . }))) . }))) . "
                                                       "}")
                                   .arg(Soprano::Node::resourceToN3(NAO::hasSubResource()),
                                        resourcesToN3(subResources).join(QLatin1String(",")),
                                        createResourceFilter(resolvedResources, QLatin1String("?r2"))),
                                   Soprano::Query::QueryLanguageSparql);
            subResources.clear();
            while(it.next()) {
                subResources << it[0].uri();
            }
            resolvedResources += subResources;
        } while(resCount < resolvedResources.count());
    }


    // get the graphs we need to check with removeTrailingGraphs later on
    QSet<QUrl> graphs;
    QSet<QUrl> actuallyRemovedResources;
    Soprano::QueryResultIterator it
            = executeQuery(QString::fromLatin1("select distinct ?g ?r where { graph ?g { ?r ?p ?o . } . FILTER(?r in (%1)) . }")
                           .arg(resourcesToN3(resolvedResources).join(QLatin1String(","))),
                           Soprano::Query::QueryLanguageSparql);
    while(it.next()) {
        graphs << it[0].uri();
        actuallyRemovedResources << it[1].uri();
    }

    // get the resources that we modify by removing relations to one of the deleted ones
    QSet<QUrl> modifiedResources;
    it = executeQuery(QString::fromLatin1("select distinct ?g ?o where { graph ?g { ?o ?p ?r . } . FILTER(?r in (%1)) . }")
                      .arg(resourcesToN3(resolvedResources).join(QLatin1String(","))),
                      Soprano::Query::QueryLanguageSparql);
    while(it.next()) {
        graphs << it[0].uri();
        modifiedResources << it[1].uri();
    }
    modifiedResources -= actuallyRemovedResources;


    // remove the resources
    foreach(const Soprano::Node& res, actuallyRemovedResources) {
        removeAllStatements(res, Soprano::Node(), Soprano::Node());
        removeAllStatements(Soprano::Node(), Soprano::Node(), res);
    }
    updateModificationDate(modifiedResources);
    removeTrailingGraphs(graphs);

    // inform interested parties
    // TODO: ideally we should also report the types the removed resources had
    foreach(const Soprano::Node& res, actuallyRemovedResources) {
        d->m_watchManager->removeResource(res.uri(), QList<QUrl>());
    }
}

#include "datamanagementmodel.moc"
