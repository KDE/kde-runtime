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
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Vocabulary/RDF>
#include <Nepomuk/Vocabulary/NIE>

#include <nepomuk/syncresource.h>
#include <KDebug>

using namespace Soprano::Vocabulary;
using namespace Nepomuk::Vocabulary;

Nepomuk::ResourceIdentifier::ResourceIdentifier()
{
    setMinScore( 1.0 );
    
    // Resource Metadata
    addOptionalProperty( NAO::created() );
    addOptionalProperty( NAO::lastModified() );
    addOptionalProperty( NAO::creator() );
    addOptionalProperty( NAO::userVisible() );
}


bool Nepomuk::ResourceIdentifier::exists(const KUrl& uri)
{
    QString query = QString::fromLatin1("ask { %1 ?p ?o . } ").arg( Soprano::Node::resourceToN3(uri) );
    return model()->executeQuery( query, Soprano::Query::QueryLanguageSparql ).boolValue();
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

bool Nepomuk::ResourceIdentifier::isIdentfyingProperty(const QUrl& uri)
{
    //- by default all properties with a literal range are identifying
    // - by default all properties with a resource range are non-identifying
    // - a few literal properties like nao:rating or nmo:imStatusMessage are marked as non-identifying
    // - a few resource properties are marked as identifying
    
    //TODO: Implement me properly!
    if( uri == NAO::created() || uri == NAO::creator() || uri == NAO::lastModified() 
        || uri == NAO::userVisible() )
        return false;
    
    if( uri == NIE::url() || uri == RDF::type() )
        return true;
    
    // TODO: Hanlde nxx:FluxProperty and nxx:resourceRangePropWhichCanIdentified
    const QString query = QString::fromLatin1("ask { %1 %2 ?range . "
                                              " %1 a %3 . "
                                              "{ FILTER( regex(str(?range), '^http://www.w3.org/2001/XMLSchema#') ) . }"
                                              " UNION { %1 a rdf:Property . } }") // rdf:Property should be nxx:resourceRangePropWhichCanIdentified
                          .arg( Soprano::Node::resourceToN3( uri ),
                                Soprano::Node::resourceToN3( RDFS::range() ),
                                Soprano::Node::resourceToN3( RDF::Property() ) );
                          
    return model()->executeQuery( query, Soprano::Query::QueryLanguageSparql ).boolValue();
}


bool Nepomuk::ResourceIdentifier::runIdentification(const KUrl& uri)
{
    kDebug() << "Identifying : " << uri;
    //
    // Check if a uri with the same name exists
    //
    if( exists( uri ) ) {
        manualIdentification( uri, uri );
        return true;
    }
    
    const Sync::SyncResource & res = simpleResource( uri );
    kDebug() << res;
    
    QString query = QString::fromLatin1("where { ?r ?p ?o. ");
    
    int num = 0;
    QStringList identifyingProperties;
    QHash< KUrl, Soprano::Node >::const_iterator it = res.constBegin();
    for( ; it != res.constEnd(); it ++ ) {
        const QUrl & prop = it.key();
        
        if( !isIdentfyingProperty( prop ) ) {
            continue;
        }
        
        identifyingProperties << Soprano::Node::resourceToN3( prop );
        
        const Soprano::Node & object = it.value();
        if( object.isBlank() || ( object.isResource() && object.uri().scheme() == QLatin1String("nepomuk") ) ) {
            const QUrl objectUri = object.isResource() ? object.uri() : "_:" + object.identifier();
            if( !identify( objectUri ) ) {
                kDebug() << "Identification of object " << objectUri << " failed";
                continue;
            }
        }
                
        query += QString::fromLatin1(" optional { ?r %1 ?o%3 . } . filter(!bound(?o%3) || ?o%3=%2). ")
                 .arg( Soprano::Node::resourceToN3( prop ), 
                       it.value().toN3(),
                       QString::number( num++ ) );
    }
    
    if( identifyingProperties.isEmpty() || num == 0 ) {
        kDebug() << "No identification properties found!";
        return false;
    }
    
    // Make sure atleast one of the identification properties has been matched
    query += QString::fromLatin1("filter( ");
    for( int i=0; i<num-1; i++ ) {
        query += QString::fromLatin1(" bound(?o%1) || ").arg( QString::number( i ) );
    }
    query += QString::fromLatin1(" bound(?o%1) ) . }").arg( QString::number( num - 1 ) );
    
    QString insideQuery = QString::fromLatin1("select count(*) where { ?r ?p ?o. filter( ?p in (%1) ). }")
                            .arg( identifyingProperties.join(",") );
    query = QString::fromLatin1("select distinct ?r (%1) as ?cnt ").arg( insideQuery ) + query + 
            QString::fromLatin1(" order by desc(?cnt)");
    
    kDebug() << query;
    
    //
    // Only store the results which have the maximum score
    //
    QSet<KUrl> results;
    int score = -1;
    Soprano::QueryResultIterator qit = model()->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    while( qit.next() ) {
        kDebug() << "RESULT: " << qit["r"] << " " << qit["cnt"];
        
        int count = qit["cnt"].literal().toInt();
        if( score == -1 ) {
            score = count;
        }
        else if( count < score )
            break;
        
        results << qit["r"].uri();
    }
    
    kDebug() << "Got " << results.size() << " results";
    if( results.empty() )
        return false;
    
    KUrl newUri;
    if( results.size() == 1 )
        newUri = *results.begin();
    else {
        kDebug() << "DUPLICATE RESULTS!";
        newUri = duplicateMatch( uri, results, 1.0 );
    }
    
    if( !newUri.isEmpty() ) {
        kDebug() << uri << " --> " << newUri;
        manualIdentification( uri, newUri );
        return true;
    }
    
    return false;
}
