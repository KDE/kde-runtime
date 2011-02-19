/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2011  Vishesh Handa <handa.vish@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "resourcemerger.h"
#include "datamanagementmodel.h"
#include "classandpropertytree.h"

#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/NAO>

#include <Soprano/StatementIterator>
#include <Soprano/FilterModel>
#include <Soprano/NodeIterator>

#include <Nepomuk/Variant>
#include <KDebug>
#include <Soprano/Graph>

using namespace Soprano::Vocabulary;

Nepomuk::ResourceMerger::ResourceMerger(Nepomuk::DataManagementModel* model, const QString& app,
                                        const QHash< QUrl, QVariant >& additionalMetadata)
{
    m_app = app;
    m_additionalMetadata = additionalMetadata;
    m_model = model;
    
    setModel( m_model );
}

Nepomuk::ResourceMerger::~ResourceMerger()
{
}

KUrl Nepomuk::ResourceMerger::createGraph()
{
    return m_model->createGraph( m_app, m_additionalMetadata );
}

void Nepomuk::ResourceMerger::resolveDuplicate(const Soprano::Statement& newSt)
{
    kDebug() << newSt;
    // Merge rules
    // 1. If old graph is of type discardable and new is non-discardable
    //    -> Then update the graph
    // 2. Otherwsie
    //    -> Keep the old graph
    
    const QUrl oldGraph = model()->listStatements( newSt.subject(), newSt.predicate(), newSt.object() ).iterateContexts().allNodes().first().uri();

    kDebug() << m_model->statementCount();
    QUrl newGraph = mergeGraphs( oldGraph );
    if( newGraph.isValid() ) {
        kDebug() << "Is valid!";
        model()->removeAllStatements( newSt.subject(), newSt.predicate(), newSt.object() );
        
        Soprano::Statement st( newSt );
        st.setContext( newGraph );
        addStatement( st );
    }
    kDebug() << m_model->statementCount();
}

QUrl Nepomuk::ResourceMerger::mergeGraphs(const QUrl& oldGraph)
{
    using namespace Soprano::Vocabulary;
    
    QList<Soprano::Statement> oldGraphStatements = model()->listStatements( oldGraph, Soprano::Node(), Soprano::Node() ).allStatements();

    kDebug() << oldGraphStatements;
    
    //Convert to prop hash
    QMultiHash<QUrl, Soprano::Node> oldPropHash;
    foreach( const Soprano::Statement & st, oldGraphStatements )
        oldPropHash.insert( st.predicate().uri(), st.object() );

    QHash< QUrl, Soprano::Node >::iterator iter = oldPropHash.find( NAO::maintainedBy() );
    QUrl oldAppUri;
    if( iter != oldPropHash.end() )
        oldAppUri = iter.value().uri();
    
    QHash< QUrl, QUrl >::const_iterator fit = m_graphHash.constFind( oldAppUri );
    if( fit != m_graphHash.constEnd() )
        return fit.value();
    
    if( m_appUri.isEmpty() )
        m_appUri = m_model->findApplicationResource( m_app );

    //
    // If both the graphs have been made by the same application - do nothing
    // FIXME: There might be an additional statement in the new graph
    if( oldAppUri == m_appUri ) {
        return QUrl();
    }
        
    QMultiHash<QUrl, Soprano::Node> newPropHash;
    QHash< QUrl, QVariant >::const_iterator it = m_additionalMetadata.begin();
    for( ; it != m_additionalMetadata.end(); it++ )
        newPropHash.insert( it.key(), Nepomuk::Variant( it.value() ).toNode() );

    QMultiHash<QUrl, Soprano::Node> finalPropHash;

    finalPropHash.insert( RDF::type(), NRL::InstanceBase() );
    if( oldPropHash.contains( RDF::type(), NRL::DiscardableInstanceBase() ) &&
        newPropHash.contains( RDF::type(), NRL::DiscardableInstanceBase() ) )
        finalPropHash.insert( RDF::type(), NRL::DiscardableInstanceBase() );

    oldPropHash.remove( RDF::type() );
    newPropHash.remove( RDF::type() );

    //TODO: Should check for cardinality in the properties
    finalPropHash.unite( oldPropHash );
    finalPropHash.unite( newPropHash );

    finalPropHash.insert( NAO::maintainedBy(), m_appUri );

    const QUrl graph = createGraphUri();
    const QUrl metadatagraph = createGraphUri();
    
    // add metadata graph itself
    m_model->addStatement( metadatagraph, NRL::coreGraphMetadataFor(), graph, metadatagraph );
    m_model->addStatement( metadatagraph, RDF::type(), NRL::GraphMetadata(), metadatagraph );
    
    for(QHash<QUrl, Soprano::Node>::const_iterator it = finalPropHash.constBegin();
        it != finalPropHash.constEnd(); ++it) {

        Soprano::Statement st( graph, it.key(), it.value(), metadatagraph );
        addStatement( st );
    }

    m_graphHash.insert( oldAppUri, graph );
    return graph;
}


KUrl Nepomuk::ResourceMerger::resolveUnidentifiedResource(const KUrl& uri)
{
    // If it is a nepomuk:/ uri, just add it as it is.
    if( uri.scheme() == QLatin1String("nepomuk") )
        return uri;
    else if( uri.url().startsWith("_:") ) // Blank node
        return Nepomuk::Sync::ResourceMerger::resolveUnidentifiedResource( uri );
    return KUrl();
}

QUrl Nepomuk::ResourceMerger::createResourceUri()
{
    return m_model->createUri( DataManagementModel::ResourceUri );
}

QUrl Nepomuk::ResourceMerger::createGraphUri()
{
    return m_model->createUri( DataManagementModel::GraphUri );
}

Soprano::Error::ErrorCode Nepomuk::ResourceMerger::addStatement(const Soprano::Statement& st)
{
    const QUrl & propUri = st.predicate().uri();

    if( propUri == RDF::type() ) {
        m_model->addStatement( st );
        return Soprano::Error::ErrorNone;
    }
    
    //
    // Cardinality checks
    //
    ClassAndPropertyTree * tree = m_model->classAndPropertyTree();
    int maxCardinality = tree->maxCardinality( propUri );

    if( maxCardinality > 0 ) {
        int existing = m_model->listStatements( st.subject(), st.predicate(), Soprano::Node() ).allStatements().size();

        if( existing == maxCardinality ) {
            m_model->setError("Max Cardinality error");
            return Soprano::Error::ErrorInvalidStatement;
        }
    }
    
    //
    // rdfs:domain and range checks
    //
    QUrl domain = tree->propertyDomain( propUri );
    QUrl range = tree->propertyRange( propUri );

    kDebug() << "Domain : " << domain;
    kDebug() << "Range : " << range;

    // domain
    if( !domain.isEmpty() && !isOfType( st.subject().uri(), domain ) ) {
        kDebug() << "invalid domain range";
        m_model->setError("Invalid domain");
        return Soprano::Error::ErrorInvalidStatement;
    }

    // range
    if( !range.isEmpty() ) {
        if( st.object().isResource() ) {
            if( !isOfType( st.object().uri(), range ) ) {
                kDebug() << "Invalid resource range";
                m_model->setError("Invalid range");
                return Soprano::Error::ErrorInvalidStatement;
            }
        }
        else if( st.object().isLiteral() ) {
            const Soprano::LiteralValue lv = st.object().literal();
            if( lv.dataTypeUri() != range ) {
                kDebug() << "Invalid literal range";
                m_model->setError("Invalid range");
                return Soprano::Error::ErrorInvalidStatement;
            }
        }
    }
    
    return m_model->addStatement( st );
}

bool Nepomuk::ResourceMerger::isOfType(const QUrl& uri, const QUrl& type) const
{
    kDebug() << "Checking " << uri << " for type " << type;
    ClassAndPropertyTree * tree = m_model->classAndPropertyTree();

    QList<Soprano::Node> types = m_model->listStatements( uri, RDF::type(), Soprano::Node() ).iterateObjects().allNodes();

    if( types.isEmpty() ) {
        kDebug() << uri << " does not have a type!!";
        return false;
    }

    foreach( const Soprano::Node & node, types ) {
        if( node.uri() == type || tree->isChildOf( node.uri(), type ) ) {
            return true;
        }
    }
    
    return false;
}

void Nepomuk::ResourceMerger::merge(const Soprano::Graph& graph )
{
    //
    // First merge all the statements predicate rdf:type.
    // That way the domain/range checks in addStatement wont fail

    QList<Soprano::Statement> remainingStatements;
    QList<Soprano::Statement> allStatements( graph.toList() );

    foreach( const Soprano::Statement & st, allStatements ) {
        if( st.predicate() == RDF::type() )
            mergeStatement( st );
        else
            remainingStatements << st;
    }

    //
    // Merge the remaining statements
    //
    foreach( const Soprano::Statement & st, remainingStatements ) {
        mergeStatement( st );
    }
}


