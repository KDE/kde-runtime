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
#include "simpleresource.h"

#include <QtCore/QSet>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QMutexLocker>

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
        : isProperty(false),
          maxCardinality(0),
          userVisible(0) {
    }

    /// true if this is a property, for classes this is false
    bool isProperty;

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

    /// only valid for properties
    QUrl domain;
    QUrl range;
};

Nepomuk::ClassAndPropertyTree::ClassAndPropertyTree(QObject *parent)
    : QObject(parent)
{
}

Nepomuk::ClassAndPropertyTree::~ClassAndPropertyTree()
{
    qDeleteAll(m_tree);
}

QSet<QUrl> Nepomuk::ClassAndPropertyTree::allParents(const QUrl &uri) const
{
    QMutexLocker lock(&m_mutex);
    if(const ClassOrProperty* cop = findClassOrProperty(uri))
        return cop->allParents;
    else
        return QSet<QUrl>();
}

bool Nepomuk::ClassAndPropertyTree::isChildOf(const QUrl &type, const QUrl &superClass) const
{
    QMutexLocker lock(&m_mutex);
    if(const ClassOrProperty* cop = findClassOrProperty(type))
        return cop->allParents.contains(superClass);
    else
        return 0;
}

bool Nepomuk::ClassAndPropertyTree::isChildOf(const QList< QUrl >& types, const QUrl& superClass) const
{
    if(superClass == Soprano::Vocabulary::RDFS::Resource()) {
        return true;
    }

    foreach( const QUrl & type, types ) {
        if( isChildOf( type, superClass ) )
            return true;
    }
    return false;
}

int Nepomuk::ClassAndPropertyTree::maxCardinality(const QUrl &type) const
{
    QMutexLocker lock(&m_mutex);
    if(const ClassOrProperty* cop = findClassOrProperty(type))
        return cop->maxCardinality;
    else
        return 0;
}

bool Nepomuk::ClassAndPropertyTree::isUserVisible(const QUrl &type) const
{
    QMutexLocker lock(&m_mutex);
    if(const ClassOrProperty* cop = findClassOrProperty(type))
        return cop->userVisible == 1;
    else
        return true;
}

QUrl Nepomuk::ClassAndPropertyTree::propertyDomain(const QUrl &uri) const
{
    QMutexLocker lock(&m_mutex);
    if(const ClassOrProperty* cop = findClassOrProperty(uri))
        return cop->domain;
    else
        return QUrl();
}

QUrl Nepomuk::ClassAndPropertyTree::propertyRange(const QUrl &uri) const
{
    QMutexLocker lock(&m_mutex);
    if(const ClassOrProperty* cop = findClassOrProperty(uri))
        return cop->range;
    else
        return QUrl();
}

Soprano::Node Nepomuk::ClassAndPropertyTree::variantToNode(const QVariant &value, const QUrl &property) const
{
    QSet<Soprano::Node> nodes = variantListToNodeSet(QVariantList() << value, property);
    if(nodes.isEmpty())
        return Soprano::Node();
    else
        return *nodes.begin();
}

QSet<Soprano::Node> Nepomuk::ClassAndPropertyTree::variantListToNodeSet(const QVariantList &vl, const QUrl &property) const
{
    QSet<Soprano::Node> nodes;
    const QUrl range = propertyRange(property);
    // special case: rdfs:Literal
    if(range == Soprano::Vocabulary::RDFS::Literal()) {
        Q_FOREACH(const QVariant& value, vl) {
            nodes.insert(Soprano::LiteralValue::createPlainLiteral(value.toString()));
        }
    }
    else {
        const QVariant::Type literalType = Soprano::LiteralValue::typeFromDataTypeUri(range);
        if(literalType == QVariant::Invalid) {
            Q_FOREACH(const QVariant& value, vl) {
                // treat as a resource range for now
                if(value.type() == QVariant::Url) {
                    nodes.insert(value.toUrl());
                }
                else if(value.type() == QVariant::String) {
                    QString s = value.toString();
                    if(!s.isEmpty()) {
                        // for convinience we support local file paths
                        if(s[0] == QDir::separator() && QFile::exists(s)) {
                            nodes.insert(QUrl::fromLocalFile(s));
                        }
                        else {
                            // treat it as a URI
                            nodes.insert(QUrl(s));
                        }
                    }
                    else {
                        // empty string
                        return QSet<Soprano::Node>();
                    }
                }
                else {
                    // invalid type
                    return QSet<Soprano::Node>();
                }
            }
        }
        else {
            Q_FOREACH(const QVariant& value, vl) {
                if(value.type() == literalType) {
                    nodes.insert(Soprano::LiteralValue(value));
                }
                else {
                    Soprano::LiteralValue v = Soprano::LiteralValue::fromString(value.toString(), range);
                    if(v.isValid()) {
                        nodes.insert(v);
                    }
                    else {
                        // failed literal conversion
                        return QSet<Soprano::Node>();
                    }
                }
            }
        }
    }

    return nodes;
}

void Nepomuk::ClassAndPropertyTree::rebuildTree(Soprano::Model* model)
{
    QMutexLocker lock(&m_mutex);

    qDeleteAll(m_tree);
    m_tree.clear();

    const QString query
            = QString::fromLatin1("select distinct ?r ?p ?v ?mc ?c ?domain ?range ?ct ?pt "
                                  "where { "
                                  "{ ?r a ?ct . FILTER(?ct=rdfs:Class) . "
                                  "OPTIONAL { ?r rdfs:subClassOf ?p . ?p a rdfs:Class . } . } "
                                  "UNION "
                                  "{ ?r a ?pt . FILTER(?pt=rdf:Property) . "
                                  "OPTIONAL { ?r rdfs:subPropertyOf ?p . ?p a rdf:Property . } . } "
                                  "OPTIONAL { ?r %1 ?mc . } . "
                                  "OPTIONAL { ?r %2 ?c . } . "
                                  "OPTIONAL { ?r %3 ?v . } . "
                                  "OPTIONAL { ?r %4 ?domain . } . "
                                  "OPTIONAL { ?r %5 ?range . } . "
                                  "FILTER(?r!=%6) . "
                                  "}" )
            .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::maxCardinality()),
                 Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::cardinality()),
                 Soprano::Node::resourceToN3(Soprano::Vocabulary::NAO::userVisible()),
                 Soprano::Node::resourceToN3(Soprano::Vocabulary::RDFS::domain()),
                 Soprano::Node::resourceToN3(Soprano::Vocabulary::RDFS::range()),
                 Soprano::Node::resourceToN3(Soprano::Vocabulary::RDFS::Resource()));
//    kDebug() << query;
    Soprano::QueryResultIterator it
            = model->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    while( it.next() ) {
        const QUrl r = it["r"].uri();
        const Soprano::Node p = it["p"];
        const Soprano::Node v = it["v"];
        int mc = it["mc"].literal().toInt();
        int c = it["c"].literal().toInt();
        const QUrl domain = it["domain"].uri();
        const QUrl range = it["range"].uri();

        ClassOrProperty* r_cop = 0;
        QHash<QUrl, ClassOrProperty*>::iterator copIt = m_tree.find(r);
        if(copIt != m_tree.end()) {
            r_cop = copIt.value();
        }
        else {
            r_cop = new ClassOrProperty;
            r_cop->uri = r;
            m_tree.insert( r, r_cop );
        }

        r_cop->isProperty = it["pt"].isValid();

        if( v.isLiteral() ) {
            r_cop->userVisible = (v.literal().toBool() ? 1 : -1);
        }

        if(mc > 0 || c > 0) {
            r_cop->maxCardinality = qMax(mc, c);
        }

        if(!domain.isEmpty()) {
            r_cop->domain = domain;
        }

        if(!range.isEmpty()) {
            r_cop->range = range;
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
        //kDebug() << "Setting nao:userVisible of" << cop->uri.toString() << ( cop->userVisible == 1 );
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
        cop->allParents += cop->directParents;

        // some cleanup to fix inheritance loops
        cop->allParents << Soprano::Vocabulary::RDFS::Resource();
        cop->allParents.remove(cop->uri);
    }
    return cop->allParents;
}


namespace {
    Soprano::Node convertIfBlankNode( const Soprano::Node & n ) {
        if( n.isResource() ) {
            const QString uriString = n.uri().toString();
            if( uriString.startsWith("_:") ) {
                return Soprano::Node( uriString.mid(2) ); // "_:" take 2 characters
            }
        }
        return n;
    }
}

QList<Soprano::Statement> Nepomuk::ClassAndPropertyTree::simpleResourceToStatementList(const Nepomuk::SimpleResource &res) const
{
    const Soprano::Node subject = convertIfBlankNode(res.uri());
    QList<Soprano::Statement> list;
    PropertyHash properties = res.properties();
    QHashIterator<QUrl, QVariant> it( properties );
    while( it.hasNext() ) {
        it.next();
        const Soprano::Node object = variantToNode(it.value(), it.key());
        list << Soprano::Statement(subject,
                                   it.key(),
                                   convertIfBlankNode(object));
    }
    return list;
}

QList<QUrl> Nepomuk::ClassAndPropertyTree::visibleTypes() const
{
    QList<QUrl> types;
    QHash<QUrl, ClassOrProperty*>::const_iterator end = m_tree.constEnd();
    for(QHash<QUrl, ClassOrProperty*>::const_iterator it = m_tree.constBegin(); it != end; ++it) {
        const ClassOrProperty* cop = *it;
        if(!cop->isProperty && cop->userVisible == 1) {
            types << cop->uri;
        }
    }
    return types;
}

#include "classandpropertytree.moc"
