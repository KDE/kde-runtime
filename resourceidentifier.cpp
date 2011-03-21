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
