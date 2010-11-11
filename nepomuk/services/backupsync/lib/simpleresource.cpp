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


#include "simpleresource.h"

#include <Soprano/Node>
#include <Soprano/Graph>
#include <Soprano/Statement>
#include <Soprano/StatementIterator>

#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Vocabulary/NFO>
#include <Soprano/Vocabulary/RDF>

#include <QtCore/QSharedData>

class Nepomuk::Sync::SimpleResource::Private : public QSharedData {
public:
    KUrl uri;
};


Nepomuk::Sync::SimpleResource::SimpleResource()
    : d( new Nepomuk::Sync::SimpleResource::Private )
{
}

Nepomuk::Sync::SimpleResource::SimpleResource(const Nepomuk::Sync::SimpleResource& rhs)
    : QMultiHash< KUrl, Soprano::Node >(rhs),
      d( rhs.d )
{
}

Nepomuk::Sync::SimpleResource::~SimpleResource()
{
}

Nepomuk::Sync::SimpleResource& Nepomuk::Sync::SimpleResource::operator=(const Nepomuk::Sync::SimpleResource& rhs)
{
    d = rhs.d;
    return *this;
}

bool Nepomuk::Sync::SimpleResource::operator==(const Nepomuk::Sync::SimpleResource& res)
{
    return d->uri == res.d->uri &&
    this->QHash<KUrl, Soprano::Node>::operator==( res );
}

QList< Soprano::Statement > Nepomuk::Sync::SimpleResource::toStatementList() const
{
    QList<Soprano::Statement> list;
    const QList<KUrl> & keys = uniqueKeys();
    foreach( const KUrl & key, keys ) {
        Soprano::Statement st;
        st.setSubject( Soprano::Node( d->uri ) );
        st.setPredicate( Soprano::Node( key ) );

        const QList<Soprano::Node>& objects = values( key );
        foreach( const Soprano::Node & node, objects ) {
            st.setObject( node );
            list.append( st );
        }
    }
    return list;
}


bool Nepomuk::Sync::SimpleResource::isFolder() const
{
    return values( Soprano::Vocabulary::RDF::type() ).contains( Soprano::Node( Nepomuk::Vocabulary::NFO::Folder() ) );
}


bool Nepomuk::Sync::SimpleResource::isFileDataObject() const
{
    return values( Soprano::Vocabulary::RDF::type() ).contains( Soprano::Node( Nepomuk::Vocabulary::NFO::FileDataObject() ) );
}


KUrl Nepomuk::Sync::SimpleResource::nieUrl() const
{
    const QHash<KUrl, Soprano::Node>::const_iterator it = constFind( Nepomuk::Vocabulary::NIE::url() );
    if( it == constEnd() )
        return KUrl();
    else
        return it.value().uri();
}


void Nepomuk::Sync::SimpleResource::setUri(const KUrl& newUri)
{
    d->uri = newUri;
}

KUrl Nepomuk::Sync::SimpleResource::uri() const
{
    return d->uri;
}

QList< Soprano::Node > Nepomuk::Sync::SimpleResource::property(const KUrl& url) const
{
    return values(url);
}

void Nepomuk::Sync::SimpleResource::removeObject(const KUrl& uri)
{
    QMutableHashIterator<KUrl, Soprano::Node> iter( *this );
    while( iter.hasNext() ) {
        iter.next();
        
        if( iter.value().isResource() && iter.value().uri() == uri )
            iter.remove();
    }
}

// static
Nepomuk::Sync::SimpleResource Nepomuk::Sync::SimpleResource::fromStatementList(const QList< Soprano::Statement >& list)
{
    Q_ASSERT( !list.isEmpty() );
    
    SimpleResource res;
    res.setUri( list.first().subject().uri() );
    
    foreach( const Soprano::Statement & st, list ) {
        KUrl pred = st.predicate().uri();
        Soprano::Node obj = st.object();
        
        if( !res.contains( pred, obj ) )
            res.insert( pred, obj );
    }
    
    return res;
}

//
// ResourceHash
//

// static
Nepomuk::Sync::ResourceHash Nepomuk::Sync::ResourceHash::fromGraph(const Soprano::Graph& graph)
{
    return fromStatementList( graph.listStatements().allStatements() );
}

// static
Nepomuk::Sync::ResourceHash Nepomuk::Sync::ResourceHash::fromStatementList(const QList< Soprano::Statement >& allStatements)
{
    //
    // Convert into multi hash for easier look up
    //
    QMultiHash<KUrl, Soprano::Statement> stHash;
    stHash.reserve( allStatements.size() );
    foreach( const Soprano::Statement & st, allStatements ) {
        KUrl uri = st.subject().uri();
        stHash.insert( uri, st );
    }
    
    //
    // Convert them into a better format --> SimpleResource
    //
    const QList<KUrl> & uniqueUris = stHash.uniqueKeys();
    
    ResourceHash resources;
    resources.reserve( uniqueUris.size() );
    
    foreach( const KUrl & resUri, uniqueUris ) {
        SimpleResource res = SimpleResource::fromStatementList( stHash.values( resUri ) );
        resources.insert( res.uri(), res );
    }
    
    return resources;
}


QList< Soprano::Statement > Nepomuk::Sync::ResourceHash::toStatementList() const
{
    QList<Soprano::Statement> stList;
    Q_FOREACH( const KUrl& uri, uniqueKeys() ) {
        const SimpleResource & res = value( uri );
        stList += res.toStatementList();
    }

    return stList;
}
