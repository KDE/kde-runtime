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

#include "resourceidentifier.h"
#include "resourceidentifier_p.h"
#include "nrio.h"

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

using namespace Nepomuk::Vocabulary;
using namespace Soprano::Vocabulary;

Nepomuk::Sync::ResourceIdentifier::Private::Private( ResourceIdentifier * parent )
    : q( parent ),
      m_model(0)
{
}


void Nepomuk::Sync::ResourceIdentifier::Private::init( Soprano::Model * model )
{
    m_model = model ? model : ResourceManager::instance()->mainModel();
}


bool Nepomuk::Sync::ResourceIdentifier::Private::identify( const KUrl& oldUri )
{
    //kDebug() << oldUri;
    const SyncResource & res = m_resourceHash[ oldUri ];
    KUrl resourceUri = findMatch( res );
    if( resourceUri.isEmpty() )
        return false;

    m_hash[ oldUri ] = resourceUri;

    kDebug() << oldUri << " ---> " << resourceUri;
    return true;
}


KUrl Nepomuk::Sync::ResourceIdentifier::Private::findMatch(const Nepomuk::Sync::SyncResource& res)
{
    // Make sure that the res has some rdf:type statements
    if( !res.contains( RDF::type() ) ) {
        kDebug() << "No rdf:type statements - Not identifying";
        return KUrl();
    }

    QString query;

    int numIdentifyingProperties = 0;
    QStringList identifyingProperties;

    QHash< KUrl, Soprano::Node >::const_iterator it = res.constBegin();
    QHash< KUrl, Soprano::Node >::const_iterator constEnd = res.constEnd();
    for( ; it != constEnd; it++ ) {
        const QUrl & prop = it.key();

        // Special handling for rdf:type
        if( prop == RDF::type() ) {
            query += QString::fromLatin1(" ?r a %1 . ").arg( it.value().toN3() );
            continue;
        }

        if( !q->isIdentifyingProperty( prop ) ) {
            continue;
        }

        identifyingProperties << Soprano::Node::resourceToN3( prop );

        Soprano::Node object = it.value();
        if( object.isBlank()
            || ( object.isResource() && object.uri().scheme() == QLatin1String("nepomuk") ) ) {

            QUrl objectUri = object.isResource() ? object.uri() : QString( "_:" + object.identifier() );
            if( !q->identify( objectUri ) ) {
                //kDebug() << "Identification of object " << objectUri << " failed";
                continue;
            }

            object = q->mappedUri( objectUri );
        }

        // FIXME: What about optional properties?
        query += QString::fromLatin1(" optional { ?r %1 ?o%3 . } . filter(!bound(?o%3) || ?o%3=%2). ")
                 .arg( Soprano::Node::resourceToN3( prop ),
                       object.toN3(),
                       QString::number( numIdentifyingProperties++ ) );
    }

    if( identifyingProperties.isEmpty() || numIdentifyingProperties == 0 ) {
        //kDebug() << "No identification properties found!";
        return KUrl();
    }

    // Make sure atleast one of the identification properties has been matched
    // by adding filter( bound(?o1) || bound(?o2) ... )
    query += QString::fromLatin1("filter( ");
    for( int i=0; i<numIdentifyingProperties-1; i++ ) {
        query += QString::fromLatin1(" bound(?o%1) || ").arg( QString::number( i ) );
    }
    query += QString::fromLatin1(" bound(?o%1) ) . }").arg( QString::number( numIdentifyingProperties - 1 ) );

    // Construct the entire query
    QString queryBegin = QString::fromLatin1("select distinct ?r count(?p) as ?cnt "
                                             "where { ?r ?p ?o. filter( ?p in (%1) ).")
                         .arg( identifyingProperties.join(",") );

    query = queryBegin + query + QString::fromLatin1(" order by desc(?cnt)");

    kDebug() << query;

    //
    // Only store the results which have the maximum score
    //
    QSet<KUrl> results;
    int score = -1;
    Soprano::QueryResultIterator qit = m_model->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    while( qit.next() ) {
        //kDebug() << "RESULT: " << qit["r"] << " " << qit["cnt"];

        int count = qit["cnt"].literal().toInt();
        if( score == -1 ) {
            score = count;
        }
        else if( count < score )
            break;

        results << qit["r"].uri();
    }

    //kDebug() << "Got " << results.size() << " results";
    if( results.empty() )
        return KUrl();

    KUrl newUri;
    if( results.size() == 1 )
        newUri = *results.begin();
    else {
        kDebug() << "DUPLICATE RESULTS!";
        newUri = q->duplicateMatch( res.uri(), results, 1.0 );
    }

    if( !newUri.isEmpty() ) {
        kDebug() << res.uri() << " --> " << newUri;
        return newUri;
    }

    return KUrl();
}

