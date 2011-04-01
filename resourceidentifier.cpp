/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2011 Vishesh Handa <handa.vish@gmail.com>

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


#include "resourceidentifier.h"

#include <QtCore/QDateTime>
#include <QtCore/QSet>

#include <Soprano/Node>
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/StatementIterator>
#include <Soprano/NodeIterator>
#include <nepomuk/syncresource.h>
#include <KDebug>

using namespace Soprano::Vocabulary;

Nepomuk::ResourceIdentifier::ResourceIdentifier()
{
    setMinScore( 1.0 );
    
    // Resource Metadata
    addOptionalProperty( NAO::created() );
    addOptionalProperty( NAO::lastModified() );
    addOptionalProperty( NAO::creator() );
    addOptionalProperty( NAO::userVisible() );
}


KUrl Nepomuk::ResourceIdentifier::additionalIdentification(const KUrl& uri)
{
    QString query = QString::fromLatin1("ask { %1 ?p ?o . } ").arg( Soprano::Node::resourceToN3(uri) );
    bool exists = model()->executeQuery( query, Soprano::Query::QueryLanguageSparql ).boolValue();
    
    if( exists ) {
        return uri;
    }
    
    // Otherwise Identification fails
    return KUrl();
}

KUrl Nepomuk::ResourceIdentifier::duplicateMatch(const KUrl& origUri, const QSet<KUrl>& matchedUris, float score)
{
    // 
    // We return the uri that has the oldest nao:created
    //
    KUrl oldestUri;
    QDateTime naoCreated = QDateTime::currentDateTime();
    foreach( const KUrl & uri, matchedUris ) {
        QList< Soprano::Node > nodes = model()->listStatements( uri, NAO::created(), Soprano::Node() ).iterateObjects().allNodes();
        Q_ASSERT( nodes.size() == 1 );
        
        QDateTime dt = nodes.first().literal().toDateTime();
        if( dt < naoCreated ) {
            oldestUri = uri;
            naoCreated = dt;
        }
    }
    
    return oldestUri;
}

bool Nepomuk::ResourceIdentifier::isIdentfyingProperty(const QUrl& uri) const
{
    //- by default all properties with a literal range are identifying
    // - by default all properties with a resource range are non-identifying
    // - a few literal properties like nao:rating or nmo:imStatusMessage are marked as non-identifying
    // - a few resource properties are marked as identifying
    
    //TODO: Implement me!
    if( uri == NAO::created() || uri == NAO::creator() || uri == NAO::lastModified() 
        || uri == NAO::userVisible() )
        return false;
    
    return true;
}


bool Nepomuk::ResourceIdentifier::runIdentification(const KUrl& uri)
{
    kDebug() << "Identifying : " << uri;
    //
    // Check if a uri with the same name exists
    //
    QUrl newUri = additionalIdentification( uri );
    if( !newUri.isEmpty() ) {
        manualIdentification( uri, newUri );
        return true;
    }
    
    const Sync::SyncResource & res = simpleResource( uri );
    kDebug() << res;
    
    QString query = QString::fromLatin1("select distinct ?r where { ");
    
    int num = 0;
    QHash< KUrl, Soprano::Node >::const_iterator it = res.constBegin();
    for( ; it != res.constEnd(); it ++ ) {
        const QUrl & prop = it.key();
        
        if( !isIdentfyingProperty( prop ) )
            continue;
        
        const Soprano::Node & object = it.value();
        //FIXME: Identify the resources as well ( but only non-ontology )
        //if( object.isResource() && !identify( object.uri() ) )
        //    return false;
                
        query += QString::fromLatin1(" optional { ?r %1 ?o%3 . filter(?o%3=%2) . } . filter(bound(?o%3)). ")
                 .arg( Soprano::Node::resourceToN3( prop ), 
                       it.value().toN3(),
                       QString::number( num++ ) );
    }
    
    // Make sure atleast one of the identification properties has been matched
    query += QString::fromLatin1("filter( ");
    for( int i=0; i<num-1; i++ ) {
        query += QString::fromLatin1(" bound(?o%1) || ").arg( QString::number( i ) );
    }
    query += QString::fromLatin1(" bound(?o%1) ) . }").arg( QString::number( num - 1 ) );
    
    //
    // FIXME; We somehow need to sort the results based on the number of properties matched
    // Maybe we could use a sub query in the query above.
    // 
    kDebug() << query;
    QSet<KUrl> results;
    Soprano::QueryResultIterator qit = model()->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    while( qit.next() ) {
        kDebug() << "RESULT: " << qit["r"];
        results << qit["r"].uri();
    }
    
    kDebug() << "Got " << results.size() << " results";
    if( results.empty() )
        return false;
    
    KUrl result;
    if( results.size() == 1 )
        result = *results.begin();
    else {
        kDebug() << "DUPLICATE RESULTS!";
        result = duplicateMatch( uri, results, 1.0 );
    }
    
    if( !result.isEmpty() ) {
        kDebug() << uri << " --> " << result;
        manualIdentification( uri, result );
        return true;
    }
    
    return false;
}
