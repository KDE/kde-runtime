/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010-11  Vishesh Handa <handa.vish@gmail.com>

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


#include "resourcemerger.h"

#include <Soprano/Model>
#include <Soprano/Graph>

#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/RDF>

#include <Nepomuk/ResourceManager>

#include <KUrl>
#include <KDebug>

using namespace Soprano::Vocabulary;

class Nepomuk::Sync::ResourceMerger::Private {
public:
    Private( ResourceMerger * resMerger );
    
    Soprano::Model * m_model;

    KUrl m_graph;

    ResourceMerger * q;
    
    QHash<KUrl, KUrl> m_mappings;

    QMultiHash<QUrl, Soprano::Node> m_additionalMetadata;

    bool push( const Soprano::Statement & st );
    KUrl resolve( const Soprano::Node & n );
};

Nepomuk::Sync::ResourceMerger::Private::Private(Nepomuk::Sync::ResourceMerger* resMerger)
    : q( resMerger )
{
}


Nepomuk::Sync::ResourceMerger::ResourceMerger( Soprano::Model * model, const QHash<KUrl, KUrl> & mappings )
    : d( new Nepomuk::Sync::ResourceMerger::Private( this ) )
{
    setModel( model );
    setMappings( mappings );
}

Nepomuk::Sync::ResourceMerger::~ResourceMerger()
{
    delete d;
}

void Nepomuk::Sync::ResourceMerger::setModel(Soprano::Model* model)
{
    if( model == 0 ) {
        d->m_model = ResourceManager::instance()->mainModel();
        return;
    }
    d->m_model = model;
}

Soprano::Model* Nepomuk::Sync::ResourceMerger::model() const
{
    return d->m_model;
}

void Nepomuk::Sync::ResourceMerger::setMappings(const QHash< KUrl, KUrl >& mappings)
{
    d->m_mappings = mappings;
}

QHash< KUrl, KUrl > Nepomuk::Sync::ResourceMerger::mappings() const
{
    return d->m_mappings;
}

void Nepomuk::Sync::ResourceMerger::setAdditionalGraphMetadata(const QMultiHash< QUrl, Soprano::Node >& additionalMetadata)
{
    d->m_additionalMetadata = additionalMetadata;
}

QMultiHash< QUrl, Soprano::Node > Nepomuk::Sync::ResourceMerger::additionalMetadata() const
{
    return d->m_additionalMetadata;
}

KUrl Nepomuk::Sync::ResourceMerger::resolveUnidentifiedResource(const KUrl& uri)
{
    //TODO: The resource should also get its metadata -> nao:created, nao:lastModified
    KUrl newUri = createResourceUri();
    d->m_mappings.insert( uri, newUri );
    return newUri;
}


bool Nepomuk::Sync::ResourceMerger::merge( const Soprano::Graph& graph )
{   
    const QList<Soprano::Statement> statements = graph.toList();
    foreach( Soprano::Statement st, statements ) {
        if(!mergeStatement( st ))
            return false;
    }
    return true;
}


bool Nepomuk::Sync::ResourceMerger::resolveStatement(Soprano::Statement& st)
{
    if( !st.isValid() )
        return false;
    
    KUrl resolvedSubject = d->resolve( st.subject() );
    if( !resolvedSubject.isValid() ) {
        kDebug() << "Subject - " << st.subject() << " resolution failed";
        return false;
    }
    
    st.setSubject( resolvedSubject );
    Soprano::Node object = st.object();
    if( (object.isResource() && object.uri().scheme() == QLatin1String("nepomuk") )
        || object.isBlank() ) {
        KUrl resolvedObject = d->resolve( object );
        if( resolvedObject.isEmpty() ) {
            kDebug() << object << " resolution failed!";
            return false;
        }
        st.setObject( resolvedObject );
    }
    
    return true;
}


bool Nepomuk::Sync::ResourceMerger::mergeStatement(const Soprano::Statement& statement)
{
    Soprano::Statement st( statement );
    if( !resolveStatement( st ) )
        return false;
        
    return d->push( st );
}


KUrl Nepomuk::Sync::ResourceMerger::createGraph()
{
    KUrl graphUri = createGraphUri();
    KUrl metadataGraph = createResourceUri();

    addStatement( metadataGraph, RDF::type(), NRL::GraphMetadata(), metadataGraph );
    addStatement( metadataGraph, NRL::coreGraphMetadataFor(), graphUri, metadataGraph );

    if( !d->m_additionalMetadata.contains( RDF::type(), NRL::InstanceBase() ) )
        d->m_additionalMetadata.insert( RDF::type(), NRL::InstanceBase() );
    
    for(QHash<QUrl, Soprano::Node>::const_iterator it = d->m_additionalMetadata.constBegin();
        it != d->m_additionalMetadata.constEnd(); ++it) {
        addStatement(graphUri, it.key(), it.value(), metadataGraph);
    }
    
    return graphUri;
}


KUrl Nepomuk::Sync::ResourceMerger::graph()
{
    if( !d->m_graph.isValid() ) {
        d->m_graph = createGraph();
        if( !d->m_graph.isValid() ) {
            setError( QString::fromLatin1("Graph creation failed. A valid graph was not returned %1")
                      .arg( d->m_graph.url() ), Soprano::Error::ErrorInvalidArgument );
        }
    }
    return d->m_graph;
}


bool Nepomuk::Sync::ResourceMerger::Private::push(const Soprano::Statement& st)
{
    Soprano::Statement statement( st );
    if( m_model->containsAnyStatement( st.subject(), st.predicate(), st.object() ) ) {
        return q->resolveDuplicate( statement );
    }

    if( !m_graph.isValid() ) {
        m_graph = q->createGraph();
        if( !m_graph.isValid() )
            return false;
    }
    statement.setContext( m_graph );
    kDebug() << "Pushing - " << statement;
    return q->addStatement( statement ) == Soprano::Error::ErrorNone;
}


KUrl Nepomuk::Sync::ResourceMerger::Private::resolve(const Soprano::Node& n)
{
    const QUrl oldUri = n.isResource() ? n.uri() : QUrl( n.toN3() );
    
    // Find in mappings
    QHash< KUrl, KUrl >::const_iterator it = m_mappings.constFind( oldUri );
    if( it != m_mappings.constEnd() ) {
        return it.value();
    } else {
        return q->resolveUnidentifiedResource( oldUri );
    }
}

bool Nepomuk::Sync::ResourceMerger::resolveDuplicate(const Soprano::Statement& /*newSt*/)
{
    return true;
}

bool Nepomuk::Sync::ResourceMerger::push(const Soprano::Statement& st)
{
    return d->push( st );
}

QUrl Nepomuk::Sync::ResourceMerger::createResourceUri()
{
    return ResourceManager::instance()->generateUniqueUri("res");
}

QUrl Nepomuk::Sync::ResourceMerger::createGraphUri()
{
    return ResourceManager::instance()->generateUniqueUri("ctx");
}

Soprano::Error::ErrorCode Nepomuk::Sync::ResourceMerger::addStatement(const Soprano::Statement& st)
{
    return d->m_model->addStatement( st );
}

Soprano::Error::ErrorCode Nepomuk::Sync::ResourceMerger::addStatement(const Soprano::Node& subject, const Soprano::Node& property, const Soprano::Node& object, const Soprano::Node& graph)
{
    return d->m_model->addStatement( Soprano::Statement(subject, property, object, graph) );
}

