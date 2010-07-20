/*
  Copyright (C) 2010 Vishesh Handa <handa.vish@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this library; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/


#include "nepomukindexfeeder.h"
#include "util.h"
#include "nie.h"

#include <QtCore/QDateTime>

#include <Soprano/Model>
#include <Soprano/Statement>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/NRL>

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Resource>

#include <KDebug>


Nepomuk::IndexFeeder::IndexFeeder( QObject* parent )
    : QThread( parent )
{
    m_stopped = false;
    start();
}


Nepomuk::IndexFeeder::~IndexFeeder()
{
    stop();
    wait();
}


void Nepomuk::IndexFeeder::begin( const QUrl & url )
{
    //kDebug() << "BEGINING";
    Request req;
    req.uri = url;

    m_stack.push( req );
}


void Nepomuk::IndexFeeder::addStatement(const Soprano::Statement& st)
{
    Q_ASSERT( !m_stack.isEmpty() );
    Request & req = m_stack.top();

    const Soprano::Node & n = st.subject();

    QUrl uriOrId;
    if( n.isResource() )
        uriOrId = n.uri();
    else if( n.isBlank() )
        uriOrId = n.identifier();

    if( !req.hash.contains( uriOrId ) ) {
        ResourceStruct rs;
        if( n.isResource() )
            rs.uri = n.uri();

        req.hash.insert( uriOrId, rs );
    }

    ResourceStruct & rs = req.hash[ uriOrId ];
    rs.propHash.insert( st.predicate().uri(), st.object() );
}


void Nepomuk::IndexFeeder::addStatement(const Soprano::Node& subject, const Soprano::Node& predicate, const Soprano::Node& object)
{
    addStatement( Soprano::Statement( subject, predicate, object, Soprano::Node() ) );
}


void Nepomuk::IndexFeeder::end()
{
    if( m_stack.isEmpty() )
        return;
    //kDebug() << "ENDING";

    Request req = m_stack.pop();

    m_queueMutex.lock();
    m_updateQueue.enqueue( req );

    m_queueMutex.unlock();
    m_queueWaiter.wakeAll();
}


void Nepomuk::IndexFeeder::stop()
{
    QMutexLocker lock( &m_queueMutex );
    m_stopped = true;
    m_queueWaiter.wakeAll();
}


QString Nepomuk::IndexFeeder::buildResourceQuery(const Nepomuk::IndexFeeder::ResourceStruct& rs) const
{
    QString query = QString::fromLatin1("select distinct ?r where { ");

    QList<QUrl> keys = rs.propHash.uniqueKeys();
    foreach( const QUrl & prop, keys ) {
        const QList<Soprano::Node>& values = rs.propHash.values( prop );

        foreach( const Soprano::Node & n, values ) {
            query += " ?r " + Soprano::Node::resourceToN3( prop ) + " " + n.toN3() + " . ";
        }
    }
    query += " } LIMIT 1";
    return query;
}


void Nepomuk::IndexFeeder::addToModel(const Nepomuk::IndexFeeder::ResourceStruct& rs) const
{
    QUrl context = generateGraph( rs.uri );
    QHashIterator<QUrl, Soprano::Node> iter( rs.propHash );
    while( iter.hasNext() ) {
        iter.next();

        Soprano::Statement st( rs.uri, iter.key(), iter.value(), context );
        //kDebug() << "ADDING : " << st;
        ResourceManager::instance()->mainModel()->addStatement( st );
    }
}


//BUG: When indexing a file, there is one main uri ( in Request ) and other additional uris
//     If there is a statement connecting the main uri with the additional ones, it will be
//     resolved correctly, but not if one of the additional one links to another additional one.
void Nepomuk::IndexFeeder::run()
{
    m_stopped = false;
    while( !m_stopped ) {

        // lock for initial iteration
        m_queueMutex.lock();

        // work the queue
        while( !m_updateQueue.isEmpty() ) {
            Request request = m_updateQueue.dequeue();

            // unlock after queue utilization
            m_queueMutex.unlock();

            // Search for the resources or create them
            //kDebug() << " Searching for duplicates or creating them ... ";
            QMutableHashIterator<QUrl, ResourceStruct> it( request.hash );
            while( it.hasNext() ) {
                it.next();

                // If it already exists
                ResourceStruct & rs = it.value();
                if( !rs.uri.isEmpty() )
                    continue;

                QString query = buildResourceQuery( rs );
                //kDebug() << query;
                Soprano::QueryResultIterator it = ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );

                if( it.next() ) {
                    //kDebug() << "Found exact match " << rs.uri << " " << it[0].uri();
                    rs.uri = it[0].uri();
                }
                else {
                    //kDebug() << "Creating ..";
                    rs.uri = ResourceManager::instance()->generateUniqueUri( QString() );

                    // Add to the repository
                    addToModel( rs );
                }
            }

            // Fix links for main
            ResourceStruct & rs = request.hash[ request.uri ];
            QMutableHashIterator<QUrl, Soprano::Node> iter( rs.propHash );
            while( iter.hasNext() ) {
                iter.next();
                Soprano::Node & n = iter.value();

                if( n.isEmpty() )
                    continue;

                if( n.isBlank() ) {
                    const QString & id = n.identifier();
                    if( !request.hash.contains( id ) )
                        continue;
                    QUrl newUri = request.hash.value( id ).uri;
                    //kDebug() << id << " ---> " << newUri;
                    iter.value() = Soprano::Node( newUri );
                }
            }

            // Add main file to the repository
            addToModel( rs );

            // lock for next iteration
            m_queueMutex.lock();
        }

        // wait for more input
        kDebug() << "Waiting...";
        m_queueWaiter.wait( &m_queueMutex );
        m_queueMutex.unlock();
        kDebug() << "Woke up.";

    }
}


QUrl Nepomuk::IndexFeeder::generateGraph( const QUrl& resourceUri ) const
{
    Soprano::Model* model = ResourceManager::instance()->mainModel();
    QUrl context = Nepomuk::ResourceManager::instance()->generateUniqueUri( "ctx" );

    // create the provedance data for the data graph
    // TODO: add more data at some point when it becomes of interest
    QUrl metaDataContext = Nepomuk::ResourceManager::instance()->generateUniqueUri( "ctx" );
    model->addStatement( context,
                         Soprano::Vocabulary::RDF::type(),
                         Soprano::Vocabulary::NRL::DiscardableInstanceBase(),
                         metaDataContext );
    model->addStatement( context,
                         Soprano::Vocabulary::NAO::created(),
                         Soprano::LiteralValue( QDateTime::currentDateTime() ),
                         metaDataContext );
    model->addStatement( context,
                         Strigi::Ontology::indexGraphFor(),
                         resourceUri,
                         metaDataContext );
    model->addStatement( metaDataContext,
                         Soprano::Vocabulary::RDF::type(),
                         Soprano::Vocabulary::NRL::GraphMetadata(),
                         metaDataContext );
    model->addStatement( metaDataContext,
                         Soprano::Vocabulary::NRL::coreGraphMetadataFor(),
                         context,
                         metaDataContext );

    return context;
}


// static
bool Nepomuk::IndexFeeder::removeIndexedDataForUrl( const KUrl& url )
{
    if ( url.isEmpty() )
        return false;

    QString query = QString::fromLatin1( "select ?g where { "
                                         "{ "
                                         "?r %2 %1 . "
                                         "?g %3 ?r . } "
                                         "UNION "
                                         "{ ?g %3 %1 . }"
                                         "}")
                    .arg( Soprano::Node::resourceToN3( url ),
                          Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                          Soprano::Node::resourceToN3( Strigi::Ontology::indexGraphFor() ) );

    Soprano::QueryResultIterator result = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    while ( result.next() ) {
        // delete the indexed data (The Soprano::NRLModel in the storage service will take care of
        // the metadata graph)
        Nepomuk::ResourceManager::instance()->mainModel()->removeContext( result.binding( "g" ) );
    }

    return true;
}


// static
bool Nepomuk::IndexFeeder::removeIndexedDataForResourceUri( const KUrl& res )
{
    if ( res.isEmpty() )
        return false;

    QString query = QString::fromLatin1( "select ?g where { ?g %1 %2 . }" )
                    .arg( Soprano::Node::resourceToN3( Strigi::Ontology::indexGraphFor() ),
                          Soprano::Node::resourceToN3( res ) );

    Soprano::QueryResultIterator result = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    while ( result.next() ) {
        // delete the indexed data (The Soprano::NRLModel in the storage service will take care of
        // the metadata graph)
        Nepomuk::ResourceManager::instance()->mainModel()->removeContext( result.binding( "g" ) );
    }

    return true;
}


void Nepomuk::IndexFeeder::quickFeed()
{
    if( m_stack.isEmpty() )
        return;

    Request & req = m_stack.top();
    QMutableHashIterator<QUrl, ResourceStruct> it( req.hash );
    while( it.hasNext() ) {
        it.next();

        addToModel( it.value() );
        it.remove();
    }
}
