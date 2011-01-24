/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-2011 Sebastian Trueg <trueg@kde.org>

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

#include "classandpropertytree.h"

#include <QtCore/QSet>

#include <Soprano/Node>
#include <Soprano/LiteralValue>
#include <Soprano/QueryResultIterator>
#include <Soprano/Model>
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/NRL>

#include <KDebug>


class Nepomuk::ClassAndPropertyTree::ClassOrProperty
{
public:
    ClassOrProperty()
        : maxCardinality(0),
          userVisible(0) {
    }

    /// the uri of the class or property
    QUrl uri;

    /// the direct parents, ie. those for which a rdfs relations exists
    QSet<QUrl> directParents;

    /// includes all parents, even grand-parents and further up
    QSet<QUrl> allParents;

    /// the max cardinality if this is a property with a max cardinality set, 0 otherwise
    int maxCardinality;

    /// 0 - undecided, 1 - visible, -1 - non-visible
    int userVisible;
};

Nepomuk::ClassAndPropertyTree::ClassAndPropertyTree(QObject *parent)
    : QObject(parent)
{
}

Nepomuk::ClassAndPropertyTree::~ClassAndPropertyTree()
{
    qDeleteAll(m_tree);
}

bool Nepomuk::ClassAndPropertyTree::isSubClassOf(const QUrl &type, const QUrl &superClass) const
{
    if(const ClassOrProperty* cop = findClassOrProperty(type))
        return cop->directParents.contains(superClass);
    else
        return 0;
}

int Nepomuk::ClassAndPropertyTree::maxCardinality(const QUrl &type) const
{
    if(const ClassOrProperty* cop = findClassOrProperty(type))
        return cop->maxCardinality;
    else
        return 0;
}

bool Nepomuk::ClassAndPropertyTree::isUserVisible(const QUrl &type) const
{
    if(const ClassOrProperty* cop = findClassOrProperty(type))
        return cop->userVisible == 1;
    else
        return true;
}

void Nepomuk::ClassAndPropertyTree::rebuildTree(Soprano::Model* model)
{
    qDeleteAll(m_tree);
    m_tree.clear();

    const QString query
            = QString::fromLatin1("select distinct ?r ?p ?v ?mc ?c "
                                  "where { "
                                  "{ ?r a rdfs:Class . "
                                  "OPTIONAL { ?r rdfs:subClassOf ?p . ?p a rdfs:Class . } . } "
                                  "UNION "
                                  "{ ?r a rdf:Property . "
                                  "OPTIONAL { ?r rdfs:subPropertyOf ?p . ?p a rdf:Property . } . } "
                                  "OPTIONAL { ?r %1 ?mc . } . "
                                  "OPTIONAL { ?r %2 ?c . } . "
                                  "OPTIONAL { ?r %3 ?v . } . "
                                  "FILTER(?r!=%4) . "
                                  "}" )
            .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::maxCardinality()),
                 Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::cardinality()),
                 Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::userVisible()),
                 Soprano::Node::resourceToN3(Soprano::Vocabulary::RDFS::Resource()));
    kDebug() << query;
    Soprano::QueryResultIterator it
            = model->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    while( it.next() ) {
        const QUrl r = it["r"].uri();
        const Soprano::Node p = it["p"];
        const Soprano::Node v = it["v"];
        int mc = it["mc"].literal().toInt();
        int c = it["c"].literal().toInt();

        ClassOrProperty* r_cop = 0;
        QHash<QUrl, ClassOrProperty*>::iterator it = m_tree.find(r);
        if(it != m_tree.end()) {
            r_cop = it.value();
        }
        else {
            r_cop = new ClassOrProperty;
            r_cop->uri = r;
            m_tree.insert( r, r_cop );
        }

        if( v.isLiteral() ) {
            r_cop->userVisible = (v.literal().toBool() ? 1 : -1);
        }

        if(mc > 0 || c > 0) {
            r_cop->maxCardinality = qMax(mc, c);
        }

        if ( p.isResource() &&
                p.uri() != r &&
                p.uri() != Soprano::Vocabulary::RDFS::Resource() ) {
            ClassOrProperty* p_cop = 0;
            if ( !m_tree.contains( p.uri() ) ) {
                p_cop = new ClassOrProperty;
                p_cop->uri = p.uri();
                m_tree.insert( p.uri(), p_cop );
            }
            r_cop->directParents.insert(p.uri());
        }
    }

    // make sure rdfs:Resource is visible by default
    ClassOrProperty* rdfsResourceNode = 0;
    QHash<QUrl, ClassOrProperty*>::iterator rdfsResourceIt = m_tree.find(Soprano::Vocabulary::RDFS::Resource());
    if( rdfsResourceIt == m_tree.end() ) {
        rdfsResourceNode = new ClassOrProperty;
        rdfsResourceNode->uri = Soprano::Vocabulary::RDFS::Resource();
        m_tree.insert( Soprano::Vocabulary::RDFS::Resource(), rdfsResourceNode );
    }
    else {
        rdfsResourceNode = rdfsResourceIt.value();
    }
    if( rdfsResourceNode->userVisible == 0 ) {
        rdfsResourceNode->userVisible = 1;
    }
    // add rdfs:Resource as parent for all top-level classes
    for ( QHash<QUrl, ClassOrProperty*>::iterator it = m_tree.begin();
          it != m_tree.end(); ++it ) {
        if( it.value() != rdfsResourceNode && it.value()->directParents.isEmpty() ) {
            it.value()->directParents.insert( Soprano::Vocabulary::RDFS::Resource() );
        }
    }

    // update all visibility
    for ( QHash<QUrl, ClassOrProperty*>::iterator it = m_tree.begin();
          it != m_tree.end(); ++it ) {
        QSet<QUrl> visitedNodes;
        updateUserVisibility( it.value(), visitedNodes );
    }

    // complete the allParents lists
    for ( QHash<QUrl, ClassOrProperty*>::iterator it = m_tree.begin();
          it != m_tree.end(); ++it ) {
        QSet<QUrl> visitedNodes;
        getAllParents( it.value(), visitedNodes );
    }
}

const Nepomuk::ClassAndPropertyTree::ClassOrProperty * Nepomuk::ClassAndPropertyTree::findClassOrProperty(const QUrl &uri) const
{
    QHash<QUrl, ClassOrProperty*>::const_iterator it = m_tree.constFind(uri);
    if(it == m_tree.constEnd())
        return 0;
    else
        return it.value();
}


/**
 * Set the value of nao:userVisible.
 * A class is visible if it has at least one visible direct parent class.
 */
int Nepomuk::ClassAndPropertyTree::updateUserVisibility( ClassOrProperty* cop, QSet<QUrl>& visitedNodes )
{
    if ( cop->userVisible != 0 ) {
        return cop->userVisible;
    }
    else {
        for ( QSet<QUrl>::iterator it = cop->directParents.begin();
             it != cop->directParents.end(); ++it ) {
            // avoid endless loops
            if( visitedNodes.contains(*it) )
                continue;
            visitedNodes.insert(*it);
            if ( updateUserVisibility( m_tree[*it], visitedNodes ) == 1 ) {
                cop->userVisible = 1;
                break;
            }
        }
        if ( cop->userVisible == 0 ) {
            // default to invisible
            cop->userVisible = -1;
        }
        kDebug() << "Setting nao:userVisible of" << cop->uri.toString() << ( cop->userVisible == 1 );
        return cop->userVisible;
    }
}

QSet<QUrl> Nepomuk::ClassAndPropertyTree::getAllParents(ClassOrProperty* cop, QSet<QUrl>& visitedNodes)
{
    if(cop->allParents.isEmpty()) {
        for ( QSet<QUrl>::iterator it = cop->directParents.begin();
             it != cop->directParents.end(); ++it ) {
            // avoid endless loops
            if( visitedNodes.contains(*it) )
                continue;
            visitedNodes.insert( *it );
            cop->allParents += getAllParents(m_tree[*it], visitedNodes);
        }
    }
    return cop->allParents;
}

#include "classandpropertytree.moc"
