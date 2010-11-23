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

#include "typevisibilitytree.h"

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Node>
#include <Soprano/LiteralValue>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/RDFS>

#include <QtCore/QMutexLocker>

#include <KDebug>


class TypeVisibilityTree::TypeVisibilityNode
{
public:
    TypeVisibilityNode( const QUrl& r )
        : uri( r ),
          userVisible( 0 ) {
    }

    QUrl uri;
    int userVisible;
    QSet<TypeVisibilityNode*> parents;

    /**
     * Set the value of nao:userVisible.
     * A class is visible if it has at least one visible direct parent class.
     */
    int updateUserVisibility( QSet<TypeVisibilityNode*>& visitedNodes ) {
        if ( userVisible != 0 ) {
            return userVisible;
        }
        else {
            if ( !parents.isEmpty() ) {
                for ( QSet<TypeVisibilityNode*>::iterator it = parents.begin();
                     it != parents.end(); ++it ) {
                    // avoid endless loops
                    if( visitedNodes.contains(*it) )
                        continue;
                    visitedNodes.insert( *it );
                    if ( (*it)->updateUserVisibility( visitedNodes ) == 1 ) {
                        userVisible = 1;
                        break;
                    }
                }
            }
            if ( userVisible == 0 ) {
                // default to invisible
                userVisible = -1;
            }
            kDebug() << "Setting nao:userVisible of" << uri.toString() << ( userVisible == 1 );
            return userVisible;
        }
    }
};

TypeVisibilityTree::TypeVisibilityTree( Soprano::Model* model )
    : m_model(model)
{
}

TypeVisibilityTree::~TypeVisibilityTree()
{
    QMutexLocker lock( &m_mutex );
}

void TypeVisibilityTree::rebuildTree()
{
    QMutexLocker lock( &m_mutex );

    // cleanup
    m_visibilityHash.clear();

    // we build a temporary helper tree
    QHash<QUrl, TypeVisibilityNode*> tree;

    const QString query
        = QString::fromLatin1( "select distinct ?r ?p ?v where { "
                               "{ ?r a rdfs:Class . "
                               "OPTIONAL { ?r rdfs:subClassOf ?p . ?p a rdfs:Class . } . } "
                               "UNION "
                               "{ ?r a rdf:Property . "
                               "OPTIONAL { ?r rdfs:subPropertyOf ?p . ?p a rdf:Property . } . } "
                               "OPTIONAL { ?r %1 ?v . } . "
                               "FILTER(?r!=rdfs:Resource) . "
                               "}" )
            .arg( Soprano::Node::resourceToN3( Soprano::Vocabulary::NAO::userVisible() ) );
    kDebug() << query;
    Soprano::QueryResultIterator it
            = m_model->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    while( it.next() ) {
        const QUrl r = it["r"].uri();
        const Soprano::Node p = it["p"];
        const Soprano::Node v = it["v"];

        if ( !tree.contains( r ) ) {
            TypeVisibilityNode* r_uvn = new TypeVisibilityNode( r );
            tree.insert( r, r_uvn );
        }
        if( v.isLiteral() )
            tree[r]->userVisible = (v.literal().toBool() ? 1 : -1);
        if ( p.isResource() &&
                p.uri() != r &&
                p.uri() != Soprano::Vocabulary::RDFS::Resource() ) {
            if ( !tree.contains( p.uri() ) ) {
                TypeVisibilityNode* p_uvn = new TypeVisibilityNode( p.uri() );
                tree.insert( p.uri(), p_uvn );
                tree[r]->parents.insert( p_uvn );
            }
            else {
                tree[r]->parents.insert( tree[p.uri()] );
            }
        }
    }

    // make sure rdfs:Resource is visible by default
    TypeVisibilityNode* rdfsResourceNode = 0;
    QHash<QUrl, TypeVisibilityNode*>::iterator rdfsResourceIt = tree.find(Soprano::Vocabulary::RDFS::Resource());
    if( rdfsResourceIt == tree.end() ) {
        rdfsResourceNode = new TypeVisibilityNode(Soprano::Vocabulary::RDFS::Resource());
        tree.insert( Soprano::Vocabulary::RDFS::Resource(), rdfsResourceNode );
    }
    else {
        rdfsResourceNode = rdfsResourceIt.value();
    }
    if( rdfsResourceNode->userVisible == 0 ) {
        rdfsResourceNode->userVisible = 1;
    }
    // add rdfs:Resource as parent for all top-level classes
    for ( QHash<QUrl, TypeVisibilityNode*>::iterator it = tree.begin();
          it != tree.end(); ++it ) {
        if( it.value() != rdfsResourceNode && it.value()->parents.isEmpty() ) {
            it.value()->parents.insert( rdfsResourceNode );
        }
    }

    // finally determine visibility of all nodes
    for ( QHash<QUrl, TypeVisibilityNode*>::iterator it = tree.begin();
          it != tree.end(); ++it ) {
        QSet<TypeVisibilityNode*> visitedNodes;
        m_visibilityHash.insert(it.key(), it.value()->updateUserVisibility( visitedNodes ) == 1 );
    }

    // get rid of the temp tree
    qDeleteAll(tree);
}

bool TypeVisibilityTree::isVisible(const QUrl &type) const
{
    QMutexLocker lock( &m_mutex );

    QHash<QUrl, bool>::const_iterator it = m_visibilityHash.constFind(type);
    if( it != m_visibilityHash.constEnd() ) {
        return *it;
    }
    else {
        kDebug() << "Could not find type" << type << "in tree. Defaulting to visible.";
        return true;
    }
}
