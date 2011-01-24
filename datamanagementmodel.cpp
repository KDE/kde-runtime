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

#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>

#include <Soprano/QueryResultIterator>
#include <Soprano/StatementIterator>
#include <Soprano/NodeIterator>

#include <QtCore/QHash>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>
#include <QtCore/QUuid>
#include <QtCore/QSet>

#include <Nepomuk/Types/Property>

namespace {
    
    //vHanda: How about using a Nepomuk::Variant internally? Nepomuk::Variant has a toNode() 
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

    QSet<Soprano::Node> variantListToNodeSet( const QVariantList & vl ) {
        QSet<Soprano::Node> nodes;
        foreach( const QVariant & var, vl ) {
            nodes.insert( variantToNode( var ) );
        }
        return nodes;
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

// Mostly Copied from Nepomuk::ResourceFilterModel::updateModificationDate
Soprano::Error::ErrorCode Nepomuk::DataManagementModel::updateModificationDate(const QUrl& resource, const QUrl & graph, const QDateTime& date)
{
    Soprano::Error::ErrorCode c = removeAllStatements( resource, Soprano::Vocabulary::NAO::lastModified(), Soprano::Node() );
    if ( c != Soprano::Error::ErrorNone )
        return c;
    
    return addStatement( resource, Soprano::Vocabulary::NAO::lastModified(), Soprano::LiteralValue( date ), graph  );
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

    int maxCardinality = Types::Property( property ).maxCardinality();
    if( maxCardinality == 1 ) {
        if( values.size() != 1 ) {
            //FIXME: Report some error
            return;
        }
    }

    // 3 & 4 both should get done by this
    QUrl graph = createGraph( app, QHash<QUrl, QVariant>() );

    foreach( const QUrl & res, resources ) {
        // Case 2.2
        if( maxCardinality == 1 && containsAnyStatement( res, property, Soprano::Node() ) ) {
            //FIXME: Report some error
            continue;
        }

        // 5 & 6
        addStatement( res, property, variantToNode( values.first() ), graph );

        //TODO: Do something with the error codes over here!
        updateModificationDate( res, graph );
    }

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

    int maxCardinality = Types::Property( property ).maxCardinality();
    if( maxCardinality == 1 ) {
        if( values.size() != 1 ) {
            //FIXME: Report some error
            return;
        }
    }

    QUrl graph = createGraph( app, QHash<QUrl, QVariant>() );
    foreach( const QUrl & res, resources ) {
        QSet<Soprano::Node> existingValuesSet = listStatements( res, property, Soprano::Node() ).iterateObjects().allNodes().toSet();
        QSet<Soprano::Node> valuesSet = variantListToNodeSet( values );
        
        Soprano::Error::ErrorCode c = Soprano::Error::ErrorNone;
        foreach( const Soprano::Node &node, existingValuesSet - valuesSet ) {
            c = removeAllStatements( res, property, node );
            if ( c != Soprano::Error::ErrorNone ) {
                //TODO: Report this error
                //return c;
            }
        }
        
        QSet<Soprano::Node> newNodes = valuesSet - existingValuesSet;
        if ( !newNodes.isEmpty() ) {
            foreach( const Soprano::Node &node, newNodes ) {
                c = addStatement( res, property, node, graph );
                if ( c != Soprano::Error::ErrorNone ) {
                    //TODO: Report this error
                    //return c;
                }
            }
            
            c = updateModificationDate( res, graph );
        }
    }

}

void Nepomuk::DataManagementModel::removeProperty(const QList<QUrl> &resources, const QUrl &property, const QVariantList &values, const QString &app)
{
    // 1. remove the triples
    // 2. remove trailing graphs
    // 3. update resource mtime

    //vHanda: Why the app value? Does that mean only the statements created by that 'app' will
    //        be removed?
    
    QSet<QUrl> graphs;
    foreach( const QUrl & res, resources ) {
        foreach( const QVariant value, values ) {
            Soprano::Node valueNode = variantToNode( value );
            const QUrl graph = listStatements( res, property, valueNode ).iterateContexts().allNodes().first().uri();
            graphs.insert( graph );

            removeAllStatements( res, property, valueNode );
        }
    }

    removeTrailingGraphs( graphs );
}

void Nepomuk::DataManagementModel::removeProperties(const QList<QUrl> &resources, const QList<QUrl> &properties, const QString &app)
{
    // 1. remove the triples
    // 2. remove trailing graphs
    // 3. update resource mtime

    QSet<QUrl> graphs;
    foreach( const QUrl & res, resources ) {
        foreach( const QUrl & prop, properties ) {
            const QUrl graph = listStatements( res, prop, Soprano::Node() ).iterateContexts().allNodes().first().uri();
            graphs.insert( graph );

            removeAllStatements( res, prop, Soprano::Node() );
        }
    }

    removeTrailingGraphs( graphs );
}

void Nepomuk::DataManagementModel::removeTrailingGraphs(const QSet<QUrl> graphs)
{
    using namespace Soprano::Vocabulary;
    
    // Remove trailing graphs
    foreach( const QUrl& graph, graphs ) {
        if( !containsAnyStatement( Soprano::Node(), Soprano::Node(), Soprano::Node(), graph) ) {
            
            const QString query = QString::fromLatin1("select ?g where { %1 %2 ?g . }")
                                  .arg( Soprano::Node::resourceToN3( graph ),
                                        Soprano::Node::resourceToN3( NRL::coreGraphMetadataFor() ) );
            Soprano::QueryResultIterator it = executeQuery( query, Soprano::Query::QueryLanguageSparql );
            
            while( it.next() ) {
                removeAllStatements( it["g"].uri(), Soprano::Node(), Soprano::Node() );
            }
            removeAllStatements( graph, Soprano::Node(), Soprano::Node() );
        }
    }
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

void Nepomuk::DataManagementModel::mergeResources(const Nepomuk::SimpleResourceGraph &resources, const QString &app, const QHash<QUrl, QVariant> &additionalMetadata)
{
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
