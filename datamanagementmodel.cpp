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
#include "simpleresourcegraph.h"
#include "simpleresource.h"
#include "transactionmodel.h"

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
#include <QtCore/QFileInfo>

#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Vocabulary/NFO>

#include <KDebug>
#include <KService>
#include <KServiceTypeTrader>
#include <Nepomuk/Variant>

using namespace Nepomuk::Vocabulary;
using namespace Soprano::Vocabulary;

//// TODO: do not allow to create properties or classes through the "normal" methods. Instead provide methods for it.
//// IDEAS:
//// 1. Somehow handle nie:hasPart (at least in describeResources - compare text annotations where we only want to annotate part of a text)

namespace {
    /// used to handle sets and lists of QUrls
    template<typename T> QStringList resourcesToN3(const T& urls) {
        QStringList n3;
        Q_FOREACH(const QUrl& url, urls) {
            n3 << Soprano::Node::resourceToN3(url);
        }
        return n3;
    }

    /// convert a hash of URL->URI mappings to N3, omitting empty URIs.
    /// This is a helper for the return type of DataManagementModel::resolveUrls
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
                     Soprano::Node::resourceToN3(NAO::created()),
                     Soprano::Node::resourceToN3(NAO::lastModified()),
                     Soprano::Node::resourceToN3(NAO::creator()),
                     Soprano::Node::resourceToN3(NAO::userVisible()),
                     Soprano::Node::resourceToN3(NIE::url()));
    }

    template<typename T> QString createResourceExcludeFilter(const T& resources, const QString& var) {
        QStringList terms = resourcesToN3(resources);
        for(int i = 0; i < terms.count(); ++i)
            terms[i].prepend(QString::fromLatin1("%1!=").arg(var));
        return terms.join(QLatin1String(" && "));
    }

    enum LocalFileState {
        ExistingLocalFile = 1,
        NonExistingLocalFile = 2,
        OtherResource = 3
    };

    /// Check if a URL points to a local file. This should be the only place where the local file is stat'ed
    inline LocalFileState isLocalFileUrl(const QUrl& url) {
        if(url.scheme() == QLatin1String("file")) {
            if(QFile::exists(url.toLocalFile())) {
                return ExistingLocalFile;
            }
            else {
                return NonExistingLocalFile;
            }
        }
        else {
            return OtherResource;
        }
    }
}

class Nepomuk::DataManagementModel::Private
{
public:
    ClassAndPropertyTree m_classAndPropertyTree;

    /// a set of properties that are maintained by the service and cannot be changed by clients
    QSet<QUrl> m_protectedProperties;
};

Nepomuk::DataManagementModel::DataManagementModel(Soprano::Model* model, QObject *parent)
    : Soprano::FilterModel(model),
      d(new Private())
{
    setParent(parent);
    updateTypeCachesAndSoOn();

    // meta data properties are protected as it is up to us to decide when they change
    d->m_protectedProperties.insert(NAO::created());
    d->m_protectedProperties.insert(NAO::creator());
    d->m_protectedProperties.insert(NAO::lastModified());
}

Nepomuk::DataManagementModel::~DataManagementModel()
{
    delete d;
}


Soprano::Error::ErrorCode Nepomuk::DataManagementModel::updateModificationDate(const QUrl& resource, const QUrl & graph, const QDateTime& date, bool includeCreationDate)
{
    Q_ASSERT(!graph.isEmpty());

    QSet<QUrl> mtimeGraphs;
    Soprano::QueryResultIterator it = executeQuery(QString::fromLatin1("select ?g where { graph ?g { %1 %2 ?d . } . }")
                                                   .arg(Soprano::Node::resourceToN3(resource),
                                                        Soprano::Node::resourceToN3(NAO::lastModified())),
                                                   Soprano::Query::QueryLanguageSparql);
    while(it.next()) {
        mtimeGraphs << it[0].uri();
    }

    Soprano::Error::ErrorCode c = removeAllStatements(resource, NAO::lastModified(), Soprano::Node());
    if (c != Soprano::Error::ErrorNone)
        return c;

    removeTrailingGraphs(mtimeGraphs);

    addStatement(resource, NAO::lastModified(), Soprano::LiteralValue( date ), graph);
    if(includeCreationDate && !containsAnyStatement(resource, NAO::created(), Soprano::Node())) {
        addStatement(resource, NAO::created(), Soprano::LiteralValue( date ), graph);
    }

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
    if(d->m_protectedProperties.contains(property)) {
        setError(QString::fromLatin1("addProperty: %1 is a protected property which can only be changed by the data management service itself.").arg(property.toString()),
                 Soprano::Error::ErrorInvalidArgument);
        return;
    }


    //
    // Convert all values to RDF nodes. This includes the property range check and conversion of local file paths to file URLs
    //
    const QSet<Soprano::Node> nodes = d->m_classAndPropertyTree.variantListToNodeSet(values, property);
    if(nodes.isEmpty()) {
        setError(QString::fromLatin1("addProperty: At least one value could not be converted into an RDF node."), Soprano::Error::ErrorInvalidArgument);
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
    if(d->m_protectedProperties.contains(property)) {
        setError(QString::fromLatin1("setProperty: %1 is a protected property which can only be changed by the data management service itself.").arg(property.toString()),
                 Soprano::Error::ErrorInvalidArgument);
        return;
    }


    //
    // Convert all values to RDF nodes. This includes the property range check and conversion of local file paths to file URLs
    //
    const QSet<Soprano::Node> nodes = d->m_classAndPropertyTree.variantListToNodeSet(values, property);
    if(nodes.isEmpty()) {
        setError(QString::fromLatin1("setProperty: At least one value could not be converted into an RDF node."), Soprano::Error::ErrorInvalidArgument);
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
    const QSet<Soprano::Node> existingValues = QSet<Soprano::Node>::fromList(resolvedNodes.values());
    QSet<QUrl> graphs;
    QList<Soprano::BindingSet> existing
            = executeQuery(QString::fromLatin1("select ?r ?v ?g where { graph ?g { ?r %1 ?v . FILTER(?r in (%2)) . } . }")
                           .arg(Soprano::Node::resourceToN3(property),
                                resourcesToN3(uriHash).join(QLatin1String(","))),
                           Soprano::Query::QueryLanguageSparql).allBindings();
    Q_FOREACH(const Soprano::BindingSet& binding, existing) {
        if(!existingValues.contains(binding["v"])) {
            removeAllStatements(binding["r"], property, binding["v"]);
            graphs.insert(binding["g"].uri());
        }
    }
    removeTrailingGraphs(graphs);

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
    if(d->m_protectedProperties.contains(property)) {
        setError(QString::fromLatin1("removeProperty: %1 is a protected property which can only be changed by the data management service itself.").arg(property.toString()),
                 Soprano::Error::ErrorInvalidArgument);
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

    // create new URIs and a new graph
    const QUrl graph = createGraph(app);
    const QUrl resUri = createUri(ResourceUri);

    // add provided metadata
    foreach(const QUrl& type, types) {
        addStatement(resUri, RDF::type(), type, graph);
    }
    if(!label.isEmpty()) {
        addStatement(resUri, NAO::prefLabel(), Soprano::LiteralValue(label), graph);
    }
    if(!description.isEmpty()) {
        addStatement(resUri, NAO::description(), Soprano::LiteralValue(description), graph);
    }

    // add basic metadata to the new resource
    const QDateTime now = QDateTime::currentDateTime();
    addStatement(resUri, NAO::created(), Soprano::LiteralValue(now), graph);
    addStatement(resUri, NAO::lastModified(), Soprano::LiteralValue(now), graph);

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


    //
    // Resolve file URLs, we can simply ignore the non-existing file resources which are reflected by empty resolved URIs
    //
    QSet<QUrl> resolvedResources = QSet<QUrl>::fromList(resolveUrls(resources).values());
    resolvedResources.remove(QUrl());
    if(resolvedResources.isEmpty()) {
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
                               .arg(Soprano::Node::resourceToN3(NAO::hasSubResource()),
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
                               .arg(Soprano::Node::resourceToN3(NAO::hasSubResource()),
                                    resourcesToN3(resolvedResources).join(QLatin1String(",")),
                                    Soprano::Node::resourceToN3(NAO::maintainedBy()),
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
                           .arg(Soprano::Node::resourceToN3(NAO::maintainedBy()),
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
                                                Soprano::Node::resourceToN3(NIE::url())),
                                           Soprano::Query::QueryLanguageSparql);
                    if(nieUrlIt.next()) {
                        nieUrl = nieUrlIt[0].uri();
                    }

                    removeAllStatements(res, Soprano::Node(), Soprano::Node(), g);
                    removeAllStatements(Soprano::Node(), Soprano::Node(), res, g);

                    if(!nieUrl.isEmpty()) {
                        addStatement(res, NIE::url(), nieUrl, g);
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
            removeAllStatements(g, NAO::maintainedBy(), appRes);
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
                           .arg(Soprano::Node::resourceToN3(NAO::maintainedBy()),
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
                      .arg(Soprano::Node::resourceToN3(NAO::maintainedBy()),
                           Soprano::Node::resourceToN3(NAO::identifier()),
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
            removeAllStatements(g, NAO::maintainedBy(), appUri);
        }
    }

    removeTrailingGraphs(QSet<QUrl>::fromList(graphs.keys()));
}

void Nepomuk::DataManagementModel::removePropertiesByApplication(const QList<QUrl> &resources, const QList<QUrl> &properties, const QString &app)
{
    setError("Not implemented yet");
}

void Nepomuk::DataManagementModel::storeResources(const Nepomuk::SimpleResourceGraph &resources, const QString &app, const QHash<QUrl, QVariant> &additionalMetadata)
{
    if(app.isEmpty()) {
        setError(QLatin1String("storeResources: Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    if(resources.isEmpty()) {
        return;
    }

    /// Holds the mapping of <file://url> to <nepomuk:/res/> uris
    QHash<QUrl, QUrl> resolvedNodes;

    //
    // Resolve file URLs in resource URIs
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
        
        // Handle file uris
        const LocalFileState localFileState = isLocalFileUrl(res.uri());
        if(localFileState == NonExistingLocalFile) {
            setError(QString::fromLatin1("Cannot store information about non-existing local files. File '%1' does not exist.").arg(res.uri().toLocalFile()), Soprano::Error::ErrorInvalidArgument);
            return;
        }
        else if(localFileState == ExistingLocalFile) {
            QUrl fileUrl = res.uri();
            QUrl newResUri = resolveUrl( fileUrl );
            if( newResUri.isEmpty() ) {
                // Resolution of one file failed. Assign it a random blank uri
                newResUri = resGraph.createBlankNode();
            }
            resolvedNodes.insert( fileUrl, newResUri );

            res.setUri( newResUri );

            if( !res.contains( NIE::url(), fileUrl ) )
                res.addProperty( NIE::url(), fileUrl );
            if( !res.contains( RDF::type(), NFO::FileDataObject() ) )
                res.addProperty( RDF::type(), NFO::FileDataObject() );
            if( QFileInfo( fileUrl.toString() ).isDir() && !res.contains( RDF::type(), NFO::Folder() )) {
                res.addProperty( RDF::type(), NFO::Folder() );
            }
        }
        else {
            allNonFileResources << res.uri();
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


    Sync::ResourceIdentifier resIdent;
    QList<Soprano::Statement> allStatements;
    QList<Sync::SimpleResource> extraResources;

    
    //
    // Resolve file URLs in property values and prepare resource identifier
    //
    foreach( const SimpleResource& res, resGraph.toList() ) {
        SimpleResource resolvedRes(res.uri());
        QHashIterator<QUrl, QVariant> it( res.properties() );
        while( it.hasNext() ) {
            it.next();

            Nepomuk::Variant var( it.value() );
            if( var.isResource() && it.key() != NIE::url() ) {
                const LocalFileState localFileState = isLocalFileUrl(var.toUrl());
                if(localFileState == NonExistingLocalFile) {
                    setError(QString::fromLatin1("Cannot store information about non-existing local files. File '%1' does not exist.").arg(var.toUrl().toLocalFile()),
                             Soprano::Error::ErrorInvalidArgument);
                    return;
                }
                else if(localFileState == ExistingLocalFile) {
                    const QUrl fileUrl = var.toUrl();
                    // Need to resolve it
                    QHash< QUrl, QUrl >::const_iterator findIter = resolvedNodes.find( fileUrl );
                    if( findIter != resolvedNodes.end() ) {
                        resolvedRes.addProperty(it.key(), findIter.value());
                    }
                    else {
                        // It doesn't exist, create it
                        QUrl resolvedUri = resolveUrl( fileUrl );
                        if( resolvedUri.isEmpty() )
                            resolvedUri = resGraph.createBlankNode();

                        Sync::SimpleResource newRes;
                        newRes.setUri( resolvedUri );
                        newRes.insert( RDF::type(), NFO::FileDataObject() );
                        newRes.insert( NIE::url(), fileUrl );
                        if( QFileInfo( fileUrl.toString() ).isDir() )
                            newRes.insert( RDF::type(), NFO::Folder() );

                        extraResources.append( newRes );

                        resolvedRes.addProperty(it.key(), resolvedUri);
                    }
                }
                else {
                    resolvedRes.addProperty(it.key(), it.value());
                }
            }
            else {
                resolvedRes.addProperty(it.key(), it.value());
            }
        }
        
        QList< Soprano::Statement > stList = resolvedRes.toStatementList();
        allStatements << stList;

        if(stList.isEmpty()) {
            setError(QLatin1String("storeResources: Encountered invalid resource"), Soprano::Error::ErrorInvalidArgument);
            return;
        }

        Sync::SimpleResource simpleRes = Sync::SimpleResource::fromStatementList( stList );
        if( !simpleRes.isValid() ) {
            setError(QLatin1String("storeResources: Contains invalid resources."), Soprano::Error::ErrorParsingFailed);
            return;
        }
        resIdent.addSimpleResource( simpleRes );
    }
    

    clearError();


    //
    // Perform the actual identification
    //
    resIdent.setModel( this );
    resIdent.setMinScore( 1.0 );
    // Are there any more properties would count as resource metadata?
    resIdent.addOptionalProperty( NAO::created() );
    resIdent.addOptionalProperty( NAO::lastModified() );
    resIdent.addOptionalProperty( NAO::creator() );
    resIdent.addOptionalProperty( NAO::userVisible() );

    resIdent.identifyAll();

    if( resIdent.mappings().empty() ) {
        kDebug() << "Nothing was mapped merging everything as it is.";
        //vHanda: This means that everything should be pushed, right?
        //return;
    }

    foreach( const KUrl & url, resIdent.unidentified() ) {
        Soprano::Node dateTime( Soprano::LiteralValue( QDateTime::currentDateTime() ) );
        allStatements << Soprano::Statement( url, NAO::created(), dateTime );
        allStatements << Soprano::Statement( url, NAO::lastModified(), dateTime );
    }

    foreach( const Sync::SimpleResource & res, extraResources ) {
        allStatements << res.toStatementList();
    }

    TransactionModel trModel(this);
    ResourceMerger merger( this, app, additionalMetadata );
    merger.setMappings( resIdent.mappings() );
    merger.merge( Soprano::Graph(allStatements) );

    if( lastError() != Soprano::Error::ErrorNone ) {
        kDebug() << " MERGING FAILED! ";
        return;
    }
    
    trModel.commit();
    
    //// TODO: do not allow to create properties or classes this way
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
        if(d->m_classAndPropertyTree.maxCardinality(prop) != 1 ||
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
    removeResources(QList<QUrl>() << res2, app);
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
        const LocalFileState localFileState = isLocalFileUrl(res);
        if(localFileState == NonExistingLocalFile) {
            setError(QString::fromLatin1("Cannot store information about non-existing local files. File '%1' does not exist.").arg(res.toLocalFile()),
                     Soprano::Error::ErrorInvalidArgument);
            return SimpleResourceGraph();
        }
        else if(localFileState == ExistingLocalFile) {
            fileUrls.insert(res);
        }
        else {
            resUris.insert(res);
        }
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
                     .arg(Soprano::Node::resourceToN3(NAO::hasSubResource()),
                          Soprano::Node::resourceToN3(Vocabulary::NIE::url()),
                          resourcesToN3(fileUrls).join(QLatin1String(",")));
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

    for(QHash<QUrl, QVariant>::const_iterator it = additionalMetadata.constBegin();
        it != additionalMetadata.constEnd(); ++it) {
        Soprano::Node node = d->m_classAndPropertyTree.variantToNode(it.value(), it.key());
        if(node.isValid()) {
            graphMetaData.insert(it.key(), node);
        }
        else {
            setError(QString::fromLatin1("Cannot convert %1 to literal value.").arg(it.value().type()), Soprano::Error::ErrorInvalidArgument);
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
                if(d->m_classAndPropertyTree.isChildOf(it.value().uri(), NRL::Graph()))
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
    if(!graphMetaData.contains(NAO::maintainedBy())) {
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

void Nepomuk::DataManagementModel::updateTypeCachesAndSoOn()
{
    d->m_classAndPropertyTree.rebuildTree(parentModel());
    emit typeAndPropertyUpdate();
}


void Nepomuk::DataManagementModel::addProperty(const QHash<QUrl, QUrl> &resources, const QUrl &property, const QHash<Soprano::Node, Soprano::Node> &nodes, const QString &app)
{
    kDebug() << resources << property << nodes << app;
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
                const QUrl graph = createUri(GraphUri);
                const QUrl metadataGraph = createUri(GraphUri);

                // FIXME: do not split the same graph again and again. Check if the graph in question already is the one we created.

                // add metadata graph
                addStatement( metadataGraph, NRL::coreGraphMetadataFor(), graph, metadataGraph );
                addStatement( metadataGraph, RDF::type(), NRL::GraphMetadata(), metadataGraph );

                // copy the metadata from the old graph to the new one
                executeQuery(QString::fromLatin1("insert into %1 { %2 ?p ?o . } where { graph %3 { %4 ?p ?o . } . }")
                             .arg(Soprano::Node::resourceToN3(metadataGraph),
                                  Soprano::Node::resourceToN3(graph),
                                  Soprano::Node::resourceToN3(m),
                                  Soprano::Node::resourceToN3(g)),
                             Soprano::Query::QueryLanguageSparql);

                // add the new app
                addStatement(graph, NAO::maintainedBy(), appRes, metadataGraph);

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
            addStatement(it->first, property, it->second, graph);
            finalResources.insert(it->first);
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
        uriHash.insert(url, resolveUrl(url, true));
    }
    return uriHash;
}

QUrl Nepomuk::DataManagementModel::resolveUrl(const QUrl &url, bool statLocalFiles) const
{
    LocalFileState localFileState = OtherResource;
    if(statLocalFiles) {
        localFileState = isLocalFileUrl(url);
    }
    else if(url.scheme() == QLatin1String("file")) {
        localFileState = ExistingLocalFile;
    }

    if(localFileState != OtherResource) {
        Soprano::QueryResultIterator it
                = executeQuery(QString::fromLatin1("select ?r where { ?r %1 %2 . } limit 1")
                               .arg(Soprano::Node::resourceToN3(NIE::url()),
                                    Soprano::Node::resourceToN3(url)),
                               Soprano::Query::QueryLanguageSparql);
        if(it.next()) {
            return it[0].uri();
        }
        else {
            // we only throw an error if the file:/ URL points to a non-existing file AND it does not exist in the database.
            if(localFileState == NonExistingLocalFile) {
                setError(QString::fromLatin1("Cannot store information about non-existing local files. File '%1' does not exist.").arg(url.toLocalFile()),
                         Soprano::Error::ErrorInvalidArgument);
            }
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
    return &d->m_classAndPropertyTree;
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

#include "datamanagementmodel.moc"
