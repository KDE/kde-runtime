/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

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
#include <Soprano/QueryResultIterator>
#include <Soprano/Graph>

#include <QtCore/QHash>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>
#include <QtCore/QUuid>
#include <QtCore/QSet>

#include <Nepomuk/Resource>
#include <Soprano/StatementIterator>
#include <Soprano/NodeIterator>


namespace {
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
}

Nepomuk::DataManagementModel::~DataManagementModel()
{
    delete d;
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
}

void Nepomuk::DataManagementModel::setProperty(const QList<QUrl> &resources, const QUrl &property, const QVariantList &values, const QString &app)
{
    // 1. check cardinality (via property cache/tree)
    // 2. if cardinality 1 (we only have 1 and none):
    //    2.1. make sure values.count() == 1 - otherwise fail
    // 3. get existing values
    // 4. make the diffs between existing and new ones
    // 5. remove the ones that are not valid anymore
    // 6. create a new graph with all the necessary data and the app
    // 7. check if the app exists, if not create it in the new graph
    // 8. add the new triples to the new graph
    // 9. update resources' mtime
}

void Nepomuk::DataManagementModel::removeProperty(const QList<QUrl> &resources, const QUrl &property, const QVariantList &values, const QString &app)
{
    // 1. remove the triples
    // 2. remove trailing graphs
    // 3. update resource mtime
}

void Nepomuk::DataManagementModel::removeProperties(const QList<QUrl> &resources, const QList<QUrl> &properties, const QString &app)
{
    // 1. remove the triples
    // 2. remove trailing graphs
    // 3. update resource mtime
}

QUrl Nepomuk::DataManagementModel::createResource(const QList<QUrl> &types, const QString &label, const QString &description, const QString &app)
{
    // 1. create an new graph
    // 2. check if the app exists, if not create it in the new graph
    // 3. create the new resource in the new graph
    // 4. return the resource's URI

    return QUrl();
}

void Nepomuk::DataManagementModel::removeResources(const QList<QUrl> &resources, const QString &app, bool force)
{
    Q_UNUSED(app);
    // 1. get all sub-resources and check if they are used by some other resource (not in the list of resources to remove)
    //    for the latter one can use a bif:exists and a !filter(?s in <s>, <s>, ...) - based on the value of force
    // 2. remove the resources and the sub-resources
    // 3. drop trailing graphs (could be optimized by enumerating the graphs up front)
}

void Nepomuk::DataManagementModel::removeDataByApplication(const QList<QUrl> &resources, const QString &app, bool force)
{
}

void Nepomuk::DataManagementModel::removeDataByApplication(const QString &app, bool force)
{
}

void Nepomuk::DataManagementModel::removePropertiesByApplication(const QList<QUrl> &resources, const QList<QUrl> &properties, const QString &app)
{
}


namespace {
    Nepomuk::Sync::SimpleResource convert( const Nepomuk::SimpleResource & s ) {
        return Nepomuk::Sync::SimpleResource::fromStatementList( s.toStatementList() );
    }

    class ResourceMerger : public Nepomuk::Sync::ResourceMerger {
    protected:
        virtual void resolveDuplicate(const Soprano::Statement& newSt);
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
        if( model()->containsAnyStatement( oldGraph, RDFS::subClassOf(), NRL::DiscardableInstanceBase() )
            && model()->containsAnyStatement( newGraph, RDFS::subClassOf(), NRL::InstanceBase() )
            && !model()->containsAnyStatement( newGraph, RDFS::subClassOf(), NRL::DiscardableInstanceBase() ) ) {
            model()->removeStatement( newSt.subject(), newSt.predicate(), newSt.object() );
            model()->addStatement( newSt );
        }

        // Case 2
        // keep the old graph -> do nothing
        
    }

}

void Nepomuk::DataManagementModel::mergeResources(const Nepomuk::SimpleResourceGraph &resources, const QString &app, const QHash<QUrl, QVariant> &additionalMetadata)
{
    Sync::ResourceIdentifier resIdent;
    foreach( const SimpleResource & res, resources ) {
        resIdent.addSimpleResource( convert(res) );
    }

    resIdent.setMinScore( 1.0 );
    resIdent.identifyAll();

    QUrl graph = createGraph( app, additionalMetadata );

    ResourceMerger merger;
    merger.setModel( this );
    merger.setGraph( graph );
    
    foreach( const KUrl & url, resIdent.unidentified() ) {
        merger.merge( resIdent.statements(url), resIdent.mappings() );
    }
}


Nepomuk::SimpleResourceGraph Nepomuk::DataManagementModel::describeResources(const QList<QUrl> &resources, bool includeSubResources)
{
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
            if(!it.value().type() == QVariant::Url) {
                // FIXME: set some error
                return QUrl();
            }
            else {
                if(d->m_classAndPropertyTree.isSubClassOf(it.value().toUrl(), Soprano::Vocabulary::NRL::Graph()))
                    haveGraphType = true;
            }
        }

        else if(property == Soprano::Vocabulary::NAO::created()) {
            if(!it.value().type() == QVariant::DateTime) {
                // FIXME: set some error
                return QUrl();
            }
        }

        Soprano::Node node = variantToNode(it.value());
        if(node.isValid()) {
            graphMetaData.insert(it.key(), node);
        }
        else {
            // FIXME: set some error
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

    // FIXME: handle app

    const QUrl graph = createUri(GraphUri);
    const QUrl metadatagraph = createUri(GraphUri);

    // add metadata graph itself
    FilterModel::addStatement( metadatagraph, Soprano::Vocabulary::NRL::coreGraphMetadataFor(), graph, metadatagraph );
    FilterModel::addStatement( metadatagraph, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::GraphMetadata(), metadatagraph );

    for(QHash<QUrl, Soprano::Node>::const_iterator it = graphMetaData.constBegin();
        it != graphMetaData.constEnd(); ++it) {
        FilterModel::addStatement(graph, it.key(), it.value(), metadatagraph);
    }

    return graph;
}

QUrl Nepomuk::DataManagementModel::createUri(Nepomuk::DataManagementModel::UriType type)
{
    QString typeToken;
    if(type == GraphUri)
        typeToken = QLatin1String("ctx");
    else
        typeToken = QLatin1String("res");

    while( 1 ) {
        const QString uuid = QUuid::createUuid().toString().mid(1, uuid.length()-2);;
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

#include "datamanagementmodel.moc"
