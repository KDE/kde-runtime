/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

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

#include "resourceidentifier.h"
#include "resourceidentifier_p.h"
#include "backupsync.h"

#include <QtCore/QDir>
#include <QtCore/QSet>

#include <Soprano/Statement>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/StatementIterator>
#include <Soprano/NodeIterator>
#include <Soprano/Node>

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Resource>
#include <Nepomuk/Variant>

#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Vocabulary/NFO>
#include <Soprano/Vocabulary/NAO>

#include <KDebug>
#include <KUrl>

Nepomuk::Sync::ResourceIdentifier::Private::Private( ResourceIdentifier * parent )
    : q( parent ),
      m_model(0),
      m_minScore( 0.60 )
{;}


void Nepomuk::Sync::ResourceIdentifier::Private::init( Soprano::Model * model )
{
    m_model = model ? model : ResourceManager::instance()->mainModel();

    // rdf:type is by default vital
    m_vitalProperties.append( Soprano::Vocabulary::RDF::type() );
}


bool Nepomuk::Sync::ResourceIdentifier::Private::identify( const KUrl& oldUri )
{
    kDebug() << oldUri;
    
    if( m_hash.contains( oldUri ) )
        return true;

    const SimpleResource & res = m_resourceHash[ oldUri ];
    KUrl resourceUri = findMatch( res );
    
    if( resourceUri.isEmpty() ) {
        resourceUri = q->additionalIdentification( oldUri ).resourceUri();
        if( resourceUri.isEmpty() )
            return false;
    }
    m_hash[ oldUri ] = resourceUri;
    
    kDebug() << oldUri << " ---> " << resourceUri;
    return true;
}



bool Nepomuk::Sync::ResourceIdentifier::Private::queryIdentify(const KUrl& oldUri)
{
    if( m_beingIdentified.contains( oldUri ) )
        return false;
    bool result = identify( oldUri );

    if( result )
        m_notIdentified.remove( oldUri );

    return result;
}


//TODO: Optimize
KUrl Nepomuk::Sync::ResourceIdentifier::Private::findMatch(const Nepomuk::Sync::SimpleResource& simpleRes)
{
    kDebug() << "SimpleResource: " << simpleRes;
    //
    // Vital Properties
    //
    int numOfVitalStatements = 0;
    QString query = QString::fromLatin1("select distinct ?r where {");
    kDebug() << m_vitalProperties;
    foreach( const KUrl & prop, m_vitalProperties ) {
        QList<Soprano::Node> objects = simpleRes.property( prop );
        foreach( const Soprano::Node & obj, objects ) {
            query += QString::fromLatin1(" ?r %1 %2. ")
                     .arg( Soprano::Node::resourceToN3( prop ),
                           obj.toN3() );
            numOfVitalStatements++;
        }
    }
    query += " }";

    kDebug() << "Number of Vital Statements : " << numOfVitalStatements;
    kDebug() << query;
    //
    // Insert them in resourceCount with count = 0
    //
    Soprano::QueryResultIterator it = m_model->executeQuery( query, Soprano::Query::QueryLanguageSparql );

    // The first int is the score while the second int is the additional
    // maxScore ( for optional properties )
    QHash<KUrl, QPair<int, int> > resourceCount;
    while( it.next() ) {
        resourceCount.insert( it[0].uri(), QPair<int, int>(0,0) );
    }

    // No match
    if( resourceCount.isEmpty() )
        return KUrl();
    

    //
    // Get all the other properties, and increase resourceCount accordingly.
    // Ignore vital properties. Don't increment the maxScore when an optional property is not found. 
    //
    int numStatementsCompared = 0;
    QList<KUrl> properties = simpleRes.uniqueKeys();
    foreach( const KUrl & propUri, properties ) {

        if( m_vitalProperties.contains( propUri ) )
            continue;

        bool isOptionalProp = m_optionalProperties.contains( propUri );
        
        Soprano::Statement statement( Soprano::Node(), propUri, Soprano::Node(), Soprano::Node() );

        QList<Soprano::Node> objList = simpleRes.values( propUri );
        foreach( const Soprano::Node& n, objList ) {
            if( n.isResource() && n.uri().scheme() == QLatin1String("nepomuk") ) {
                if( !queryIdentify( n.uri() ) ) {
                    continue;
                }
            }
            statement.setObject( n );

            Soprano::NodeIterator iter = m_model->listStatements( statement ).iterateSubjects();
            while( iter.next() ) {
                QHash< KUrl, QPair<int,int> >::iterator it = resourceCount.find( iter.current().uri() );

                if( it != resourceCount.end() ) {
                    if( isOptionalProp ) {
                        // It is an optional property and it has matched
                        // -> Increase the score ( first )
                        // -> and increase the max score ( second )
                        // ( optional properties don't contribute to the max score unless matched )
                        it.value().first++;
                        it.value().second++;
                    }
                    else {
                        if( it != resourceCount.end() ) {
                            it.value().first++; // only increase the score
                        }
                    }
                }
            }
            if( !isOptionalProp )
                numStatementsCompared++;
        }
    }
    
    //
    // Find the resources with the max score
    //
    QSet<KUrl> maxResources;
    float maxScore = -1;
    
    foreach( const KUrl & key, resourceCount.keys() ) {
        QPair<int, int> scorePair = resourceCount.value( key );

        // The divisor will be the total number of statements it was compared to
        // ie optional-properties-matched ( stored in scorePair.second ) + numStatementsCompared
        float divisor = scorePair.second + numStatementsCompared;
        float score = 0;
        if( divisor )
            score = scorePair.first / divisor;
        
        if( score > maxScore ) {
            maxScore = score;
            maxResources.clear();
            maxResources.insert( key );
        }
        else if( score == maxScore ) {
            maxResources.insert( key );
        }
    }
    
    if( maxScore < m_minScore ) {
        return KUrl();
    }

    if( maxResources.empty() )
        return KUrl();

    if( maxResources.size() > 1 ) {
        kDebug() << "WE GOT A PROBLEM!!";
        kDebug() << "More than one resource with the exact same score found";
        kDebug() << "NOT IDENTIFYING IT! Do it manually!";

        return q->duplicateMatch( simpleRes.uri(), maxResources, maxScore );
    }

    return (*maxResources.begin());
}

