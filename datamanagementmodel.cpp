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

#include <Nepomuk/Resource>

#include <KDebug>
#include <KService>
#include <KServiceTypeTrader>


namespace {
    /// returns an invalid node if the variant could not be converted
    Soprano::Node variantToNode(const QVariant& v) {
        if(v.type() == QVariant::Url) {
            return v.toUrl();
        }
        else {
            Soprano::LiteralValue slv(v);
            if(slv.isValid())
                return slv;
            else
                return Soprano::Node();
        }
    }

    /// returns an empty set if at least one of the values could not be converted
    QSet<Soprano::Node> variantListToNodeSet( const QVariantList & vl ) {
        QSet<Soprano::Node> nodes;
        foreach( const QVariant & var, vl ) {
            Soprano::Node node = variantToNode(var);
            if(!node.isValid())
                return QSet<Soprano::Node>();
            nodes.insert( node );
        }
        return nodes;
    }

    QStringList resourcesToN3(const QList<QUrl>& urls) {
        QStringList n3;
        Q_FOREACH(const QUrl& url, urls) {
            n3 << Soprano::Node::resourceToN3(url);
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
        setError(QLatin1String("Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    foreach( const QUrl & res, resources ) {
        if(res.isEmpty()) {
            setError(QLatin1String("Encountered empty resource URI."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }
    if(property.isEmpty()) {
        setError(QLatin1String("Property needs to be specified."), Soprano::Error::ErrorInvalidArgument);
        return;
    }


    //
    // Check cardinality conditions
    //
    const int maxCardinality = d->m_classAndPropertyTree.maxCardinality(property);
    if( maxCardinality == 1 ) {
        // check if any of the resources already has a value set
        QStringList terms;
        Q_FOREACH(const QUrl& res, resources) {
            terms << QString::fromLatin1("%1 %2 ?v . FILTER(?v!=%3) . ")
                     .arg(Soprano::Node::resourceToN3(res),
                          Soprano::Node::resourceToN3(property),
                          variantToNode(values.first()).toN3());
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
    // Check the integrity of the values
    //
    const QSet<Soprano::Node> nodes = variantListToNodeSet(values);
    if(nodes.isEmpty()) {
        setError(QString::fromLatin1("At least one value could not be converted into an RDF node."), Soprano::Error::ErrorInvalidArgument);
        return;
    }


    //
    // Do the actual work
    //
    addProperty(resources, property, nodes, app);
}


// setting a property can be implemented by way of addProperty. All we have to do before calling addProperty is to remove all
// the values that are not in the setProperty call
void Nepomuk::DataManagementModel::setProperty(const QList<QUrl> &resources, const QUrl &property, const QVariantList &values, const QString &app)
{
    //
    // Check parameters
    //
    if(app.isEmpty()) {
        setError(QLatin1String("Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    foreach( const QUrl & res, resources ) {
        if(res.isEmpty()) {
            setError(QLatin1String("Encountered empty resource URI."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }
    if(property.isEmpty()) {
        setError(QLatin1String("Property needs to be specified."), Soprano::Error::ErrorInvalidArgument);
        return;
    }

    const QSet<Soprano::Node> nodes = variantListToNodeSet(values);
    if(nodes.isEmpty()) {
        setError(QString::fromLatin1("At least one value could not be converted into an RDF node."), Soprano::Error::ErrorInvalidArgument);
        return;
    }

    //
    // Remove values that are not wanted anymore
    //
    QList<Soprano::BindingSet> existing
            = executeQuery(QString::fromLatin1("select ?r ?v where { ?r %1 ?v . FILTER(?r in (%2)) . }")
                           .arg(Soprano::Node::resourceToN3(property),
                                resourcesToN3(resources).join(QLatin1String(","))),
                           Soprano::Query::QueryLanguageSparql).allBindings();
    Q_FOREACH(const Soprano::BindingSet& binding, existing) {
        if(!nodes.contains(binding["v"])) {
            removeAllStatements(binding["r"], property, binding["v"]);
        }
    }

    //
    // And finally add the rest of the statements
    //
    addProperty(resources, property, nodes, app);
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
        setError(QLatin1String("Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    foreach( const QUrl & res, resources ) {
        if(res.isEmpty()) {
            setError(QLatin1String("Encountered empty resource URI."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }
    if(property.isEmpty()) {
        setError(QLatin1String("Property needs to be specified."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    const QSet<Soprano::Node> valueNodes = variantListToNodeSet(values);
    if(valueNodes.isEmpty()) {
        setError(QString::fromLatin1("At least one value could not be converted into an RDF node."), Soprano::Error::ErrorInvalidArgument);
        return;
    }


    clearError();


    //
    // Actually change data
    //
    QUrl mtimeGraph;
    QSet<QUrl> graphs;
    const QString propertyN3 = Soprano::Node::resourceToN3(property);
    foreach( const QUrl & res, resources ) {
        const QList<Soprano::BindingSet> valueGraphs
                = executeQuery(QString::fromLatin1("select ?g ?v where { graph ?g { %1 %2 ?v . } . FILTER(?v in (%3)) . }")
                               .arg(Soprano::Node::resourceToN3(res),
                                    propertyN3,
                                    nodesToN3(valueNodes).join(QLatin1String(","))),
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
        setError(QLatin1String("Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    foreach( const QUrl & res, resources ) {
        if(res.isEmpty()) {
            setError(QLatin1String("Encountered empty resource URI."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }
    foreach(const QUrl& property, properties) {
        if(property.isEmpty()) {
            setError(QLatin1String("Encountered empty property URI."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }


    clearError();


    //
    // Actually change data
    //
    QUrl mtimeGraph;
    QSet<QUrl> graphs;
    foreach( const QUrl & res, resources ) {
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
        setError(QLatin1String("Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return QUrl();
    }
    foreach(const QUrl& type, types) {
        if(type.isEmpty()) {
            setError(QLatin1String("Encountered empty type URI."), Soprano::Error::ErrorInvalidArgument);
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

void Nepomuk::DataManagementModel::removeResources(const QList<QUrl> &resources, const QString &app, bool force)
{
    kDebug() << resources << app << force;

    Q_UNUSED(app);
    // 1. get all sub-resources and check if they are used by some other resource (not in the list of resources to remove)
    //    for the latter one can use a bif:exists and a !filter(?s in <s>, <s>, ...) - based on the value of force
    // 2. remove the resources and the sub-resources
    // 3. drop trailing graphs (could be optimized by enumerating the graphs up front)

    //
    // Check parameters
    //
    if(app.isEmpty()) {
        setError(QLatin1String("Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }
    foreach( const QUrl & res, resources ) {
        if(res.isEmpty()) {
            setError(QLatin1String("Encountered empty resource URI."), Soprano::Error::ErrorInvalidArgument);
            return;
        }
    }


    clearError();


    // get the graphs we need to check with removeTrailingGraphs later on
    QSet<QUrl> graphs;
    Soprano::QueryResultIterator it
            = executeQuery(QString::fromLatin1("select distinct ?g where { graph ?g { ?r ?p ?o . } . FILTER(?r in (%1)) . }")
                           .arg(resourcesToN3(resources).join(QLatin1String(","))),
                           Soprano::Query::QueryLanguageSparql);
    while(it.next()) {
        graphs << it[0].uri();
    }

    // remove the resources
    foreach( const QUrl & res, resources ) {
        removeAllStatements(res, Soprano::Node(), Soprano::Node());
        removeAllStatements(Soprano::Node(), Soprano::Node(), res);
    }


    // FIXME: handle the sub-resources.


    removeTrailingGraphs(graphs);
}

void Nepomuk::DataManagementModel::removeDataByApplication(const QList<QUrl> &resources, const QString &app, bool force)
{
    setError("Not implemented yet");
}

void Nepomuk::DataManagementModel::removeDataByApplication(const QString &app, bool force)
{
    setError("Not implemented yet");
}

void Nepomuk::DataManagementModel::removePropertiesByApplication(const QList<QUrl> &resources, const QList<QUrl> &properties, const QString &app)
{
    setError("Not implemented yet");
}


namespace {
  
    class ResourceMerger : public Nepomuk::Sync::ResourceMerger {
    protected:
        virtual void resolveDuplicate(const Soprano::Statement& newSt);
        virtual KUrl resolveUnidentifiedResource(const KUrl& uri);
    };

    void ResourceMerger::resolveDuplicate(const Soprano::Statement& newSt)
    {
        using namespace Soprano::Vocabulary;
        
        // Merge rules
        // 1. If old graph is of type discardable and new is non-discardable
        //    -> Then update the graph
        // 2. Otherwsie
        //    -> Keep the old graph

        const QUrl oldGraph = model()->listStatements( newSt.subject(), newSt.predicate(), newSt.object() ).iterateContexts().allNodes().first().uri();
        const QUrl newGraph = newSt.context().uri();

        // Case 1
        if( model()->containsAnyStatement( oldGraph, RDF::type(), NRL::DiscardableInstanceBase() )
            && model()->containsAnyStatement( newGraph, RDF::type(), NRL::InstanceBase() )
            && !model()->containsAnyStatement( newGraph, RDF::type(), NRL::DiscardableInstanceBase() ) ) {

            model()->removeAllStatements( newSt.subject(), newSt.predicate(), newSt.object() );
            model()->addStatement( newSt );
        }

        // Case 2
        // keep the old graph -> do nothing
        
    }

    KUrl ResourceMerger::resolveUnidentifiedResource(const KUrl& uri)
    {
        // If it is a nepomuk:/ uri, just add it as it is.
        if( uri.scheme() == QLatin1String("nepomuk") )
            return uri;
        else if( uri.url().startsWith("_:") ) // Blank node
            return Nepomuk::Sync::ResourceMerger::resolveUnidentifiedResource( uri );
        return KUrl();
    }

}

void Nepomuk::DataManagementModel::mergeResources(const Nepomuk::SimpleResourceGraph &resources, const QString &app, const QHash<QUrl, QVariant> &additionalMetadata)
{

    QList<Soprano::Statement> allStatements;

    if(app.isEmpty()) {
        setError(QLatin1String("Empty application specified. This is not supported."), Soprano::Error::ErrorInvalidArgument);
        return;
    }

    clearError();

    Sync::ResourceIdentifier resIdent;
    
    foreach( const SimpleResource & res, resources ) {
        QList< Soprano::Statement > stList = res.toStatementList();
        allStatements << stList;
        
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
    
    //FIXME: They may be cases where this graph is created just for the heck of it!
    QUrl graph = createGraph( app, additionalMetadata );
    if(lastError()) {
        return;
    }

    ResourceMerger merger;
    merger.setModel( this );
    merger.setGraph( graph );

    merger.merge( Soprano::Graph(allStatements), resIdent.mappings() );
    
    //// TODO: do not allow to create properties or classes this way
    //setError("Not implemented yet");
}


Nepomuk::SimpleResourceGraph Nepomuk::DataManagementModel::describeResources(const QList<QUrl> &resources, bool includeSubResources)
{
    setError("Not implemented yet");
    return SimpleResourceGraph();
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

        Soprano::Node node = variantToNode(it.value());
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
        graphMetaData.insert(Soprano::Vocabulary::NAO::maintainedBy(), createApplication(app));
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

QUrl Nepomuk::DataManagementModel::createApplication(const QString &app)
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
    else {
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

bool Nepomuk::DataManagementModel::checkRange(const QUrl &property, const QSet<Soprano::Node> &values) const
{
    const QUrl range = d->m_classAndPropertyTree.propertyRange(property);

    return true;
}

void Nepomuk::DataManagementModel::addProperty(const QList<QUrl> &resources, const QUrl &property, const QSet<Soprano::Node> &nodes, const QString &app)
{
    Q_ASSERT(!nodes.isEmpty());

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


    //
    // Check the integrity of the values
    //
    if(!checkRange(property, nodes)) {
        setError(QString::fromLatin1("At least one value has a type incompatible with property %1").arg(Soprano::Node::resourceToN3(property)), Soprano::Error::ErrorInvalidArgument);
        return;
    }


    clearError();


    //
    // Check if values already exist. If so remove the resources from the resourceSet and only add the application
    // as maintainedBy in a new graph (except if its the only statement in the graph)
    //
    QSet<QPair<QUrl, Soprano::Node> > finalProperties;
    Q_FOREACH(const QUrl& res, resources) {
        Q_FOREACH(const Soprano::Node& node, nodes) {
            finalProperties << qMakePair(res,node);
        }
    }

    const QUrl appRes = createApplication(app);

    const QString existingValuesQuery = QString::fromLatin1("select ?r ?v ?g "
                                                            "(select count(*) where { graph ?g { ?s ?p ?o . } . }) as ?cnt "
                                                            "(select ?mg where { ?mg %1 ?g .}) as ?m "
                                                            "where { "
                                                            "graph ?g { ?r %2 ?v . } . "
                                                            "FILTER(?r in (%3)) . "
                                                            "FILTER(?v in (%4)) . }")
            .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::coreGraphMetadataFor()),
                 Soprano::Node::resourceToN3(property),
                 resourcesToN3(resources).join(QLatin1String(",")),
                 nodesToN3(nodes).join(QLatin1String(",")));
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

        if(cnt == 1) {
            // we can reuse the graph
            addStatement(g, Soprano::Vocabulary::NAO::maintainedBy(), appRes, m);
        }
        else {
            // we need to split the graph
            const QUrl graph = createUri(GraphUri);
            const QUrl metadataGraph = createUri(GraphUri);

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


    //
    // All conditions have been checked - create the actual data
    //
    const QUrl graph = createGraph( app );
    if(!graph.isValid()) {
        // error has been set in createGraph
        return;
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
        updateModificationDate(res, graph);
    }
}

bool Nepomuk::DataManagementModel::doesResourceExist(const QUrl &res) const
{
    return executeQuery(QString::fromLatin1("ask where { %1 ?p ?v . FILTER(?p!=%2 && ?p!=%3 && ?p!=%4 && ?p!=%5) . }")
                        .arg(Soprano::Node::resourceToN3(res),
                             Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::created()),
                             Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::lastModified()),
                             Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::creator()),
                             Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::userVisible())),
                        Soprano::Query::QueryLanguageSparql).boolValue();
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
