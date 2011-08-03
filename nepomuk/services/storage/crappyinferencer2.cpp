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

#include "crappyinferencer2.h"
#include "classandpropertytree.h"

#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Statement>
#include <Soprano/StatementIterator>
#include <Soprano/Node>
#include <Soprano/NodeIterator>
#include <Soprano/Graph>
#include <Soprano/QueryResultIterator>

#include <QtCore/QTime>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QThread>

#include <kdebug.h>


class CrappyInferencer2::Private
{
public:
    Private()
        : m_updateAllResourcesThread(0) {
    }

    /// get the list of super types for \p type
    QSet<QUrl> superClasses( const QUrl& type ) const;

    /// Check if a resource with the specified types is visible.
    bool isVisibleFromTypes( const QSet<QUrl>& types ) const;

    /// Performs a query to retrieve all types that were not added by us
    QSet<QUrl> queryResourceTypes( const QUrl& resource ) const;

    /// used to build m_superClasses (and nothing else!)
    QSet<QUrl> buildSuperClassesHash( const QUrl& type, QSet<QUrl>& visitedTypes, const QMultiHash<QUrl, QUrl>& rdfsSubClassRelations );

    QHash<QUrl, QSet<QUrl> > m_superClasses;
    Nepomuk::ClassAndPropertyTree* m_typeVisibilityTree;
    QMutex m_mutex;

    UpdateAllResourcesThread* m_updateAllResourcesThread;

    QUrl m_inferenceContext;

    CrappyInferencer2* q;
};


/**
 * Simple helper thread which updates the subclass and visibility inference on all resources
 * in the database. This is done in a separate thread since it takes up to half an hour the first time.
 */
class CrappyInferencer2::UpdateAllResourcesThread : public QThread
{
public:
    UpdateAllResourcesThread( CrappyInferencer2* parent )
        : QThread(parent),
          m_canceled(false),
          m_model(parent) {
    }

    void cancel() {
        kDebug();
        m_canceled = true;
    }

    void run() {
        kDebug();
        m_canceled = false;
#ifndef NDEBUG
        QTime timer;
        timer.start();
#endif

        // Update visibility for all visible resources the fast way
        // resources that do not have visibility set are treated as invisible, thus there is no need
        // to write their visibility value
        Q_FOREACH(const QUrl& type, m_model->d->m_typeVisibilityTree->visibleTypes()) {
            if(m_canceled)
                break;
            if(m_model->d->m_typeVisibilityTree->isChildOf(type, Soprano::Vocabulary::NRL::Graph()))
                continue;
            const QString query = QString::fromLatin1("insert into graph %1 { ?r %2 1 . } where { "
                                                      "graph ?g { ?r a %3 . } . "
                                                      "FILTER(?g!=%1) . "
                                                      "FILTER(!bif:exists((select (1) where { ?r %2 ?v . }))) . } ")
                    .arg( Soprano::Node::resourceToN3( m_model->crappyInferenceContext() ),
                          Soprano::Node::resourceToN3( Soprano::Vocabulary::NAO::userVisible() ),
                          Soprano::Node::resourceToN3( type ) );
            m_model->executeQuery( query, Soprano::Query::QueryLanguageSparql );
        }

#ifndef NDEBUG
        kDebug() << "All resources with visible type updated. Elapsed:" << timer.elapsed()/1000.0 << "sec";
#endif

        // update the resource type inference on all resources that were created before KDE 4.6.1
        // we ignore graphs entirely to save space and allow the DMS to use SPARUL to remove graphs
        Soprano::QueryResultIterator it = m_model->executeQuery(QString::fromLatin1("select distinct ?t where { ?t a %1 . "
                                                                                    "FILTER(bif:exists((select (1) where { graph ?g { ?r a ?t . } . FILTER(?g!=%2) . }))) . "
                                                                                    "FILTER(!bif:exists((select (1) where { ?t %3 %4 . }))) . }")
                                                                .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::RDFS::Class()),
                                                                     Soprano::Node::resourceToN3(m_model->crappyInferenceContext()),
                                                                     Soprano::Node::resourceToN3(Soprano::Vocabulary::RDFS::subClassOf()),
                                                                     Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::Graph())),
                                                                Soprano::Query::QueryLanguageSparql);
        while(!m_canceled && it.next()) {
            const QUrl type = it["t"].uri();
            const QSet<QUrl> superClasses = m_model->d->m_superClasses[type];
            QString superTypeTerms;
            Q_FOREACH(const QUrl& superType, superClasses) {
                superTypeTerms += QString::fromLatin1("?r a %1 . ").arg(Soprano::Node::resourceToN3(superType));
            }

            // we cannot restrict to resources that already have types in the crappy inferencer 2 graph
            // since one resource could have more than one type and thus, needs to be updated with more
            // than one query here.
            const QString query = QString::fromLatin1("insert into graph %1 { %2 } where { "
                                                      "graph ?g { ?r a %3 . } . FILTER(?g!=%1) . }")
                    .arg(Soprano::Node::resourceToN3( m_model->crappyInferenceContext()),
                         superTypeTerms,
                         Soprano::Node::resourceToN3(type));
            m_model->executeQuery( query, Soprano::Query::QueryLanguageSparql );
        }

#ifndef NDEBUG
        kDebug() << "All resources' subClassOf inference updated. Elapsed:" << timer.elapsed()/1000.0 << "sec";
#endif
    }

private:
    bool m_canceled;
    CrappyInferencer2* m_model;
};



QSet<QUrl> CrappyInferencer2::Private::superClasses( const QUrl& type ) const
{
    QHash<QUrl, QSet<QUrl> >::const_iterator it = m_superClasses.constFind(type);
    if( it != m_superClasses.constEnd() ) {
        return it.value();
    }
    else {
        return QSet<QUrl>() << Soprano::Vocabulary::RDFS::Resource();
    }
}

bool CrappyInferencer2::Private::isVisibleFromTypes(const QSet<QUrl> &types) const
{
    // We need to extract the leaf types in the subgraph that is types
    // as those are the types that define if the resource will be visible

    // TODO: do the following in an efficient way
    // we remove all supertypes from the set of types and are left with the leaf types
    QSet<QUrl> leafTypes(types);
    Q_FOREACH(const QUrl& type, types) {
        leafTypes -= superClasses(type);
        if(leafTypes.isEmpty())
            break;
    }

    // if there is one leaf type left that is visible the resource is visible, too
    Q_FOREACH( const QUrl& type, leafTypes ) {
        if( m_typeVisibilityTree->isUserVisible(type) ) {
            return true;
        }
    }

    // no visible type found
    return false;
}

QSet<QUrl> CrappyInferencer2::Private::queryResourceTypes(const QUrl &resource) const
{
    QSet<QUrl> resourceTypes;
    Soprano::QueryResultIterator it
            = q->parentModel()->executeQuery( QString::fromLatin1("select ?t where { graph ?g { %1 a ?t . } . FILTER(?g!=%2) . }")
                                             .arg(Soprano::Node::resourceToN3(resource),
                                                  Soprano::Node::resourceToN3(m_inferenceContext)),
                                             Soprano::Query::QueryLanguageSparql);
    while(it.next()) {
        resourceTypes << it[0].uri();
    }

    return resourceTypes;
}

QSet<QUrl> CrappyInferencer2::Private::buildSuperClassesHash( const QUrl& type, QSet<QUrl>& visitedTypes, const QMultiHash<QUrl, QUrl>& rdfsSubClassRelations = QMultiHash<QUrl, QUrl>() )
{
    QHash<QUrl, QSet<QUrl> >::const_iterator it = m_superClasses.constFind(type);
    if( it == m_superClasses.constEnd() ) {
        QSet<QUrl> superTypes;
        for(QMultiHash<QUrl, QUrl>::const_iterator it = rdfsSubClassRelations.constFind(type);
            it != rdfsSubClassRelations.constEnd() && it.key() == type; ++it) {
            // avoid endless loops in case we have circular hierarchies
            if( !visitedTypes.contains( it.value() ) ) {
                visitedTypes << it.value();
                superTypes << it.value();
                superTypes += buildSuperClassesHash( it.value(), visitedTypes, rdfsSubClassRelations );
            }
        }
        m_superClasses.insert( type, superTypes );
        return superTypes;
    }
    else {
        return it.value();
    }
}

CrappyInferencer2::CrappyInferencer2(Nepomuk::ClassAndPropertyTree* tree, Soprano::Model* parent)
    : Soprano::FilterModel(parent),
      d(new Private())
{
    d->q = this;
    d->m_inferenceContext = QUrl::fromEncoded("urn:crappyinference2:inferredtriples");
    d->m_typeVisibilityTree = tree;
    if ( parent )
        updateInferenceIndex();
}

CrappyInferencer2::~CrappyInferencer2()
{
    if( d->m_updateAllResourcesThread ) {
        d->m_updateAllResourcesThread->cancel();
        d->m_updateAllResourcesThread->wait();
    }
    delete d;
}

void CrappyInferencer2::setParentModel(Soprano::Model *model)
{
    FilterModel::setParentModel( model );
    updateInferenceIndex();
}

Soprano::Error::ErrorCode CrappyInferencer2::addStatement(const Soprano::Statement &statement)
{
    if( statement.context() == d->m_inferenceContext ) {
#ifdef NDEBUG
        kDebug() << "No actions are allowed on the crappy inferencer context in release mode!" << statement;
        return Soprano::Error::ErrorInvalidArgument;
#else
        kWarning() << "WARNING!!!!!!!!!!! Adding statement in crappy inferencer context!!!!!!!!!!!!" << statement;
#endif
    }

    QMutexLocker lock( &d->m_mutex );

    Soprano::Error::ErrorCode r = parentModel()->addStatement( statement );
    if( r == Soprano::Error::ErrorNone ) {
        //
        // Handle the inference. However, we ignore inference for all graphs to save space and allow the DMS to use SPARUL there.
        //
        if ( statement.subject().isResource() &&
             statement.object().isResource() &&
             statement.predicate() == Soprano::Vocabulary::RDF::type() &&
             !d->m_typeVisibilityTree->isChildOf(statement.object().uri(), Soprano::Vocabulary::NRL::Graph())) {
            addInferenceStatements( statement.subject().uri(), statement.object().uri() );
        }
    }

    return r;
}

Soprano::Error::ErrorCode CrappyInferencer2::removeStatement(const Soprano::Statement &statement)
{
    if( statement.context() == d->m_inferenceContext ) {
#ifdef NDEBUG
        kDebug() << "No actions are allowed on the crappy inferencer context in release mode!" << statement;
        return Soprano::Error::ErrorInvalidArgument;
#else
        kWarning() << "WARNING!!!!!!!!!!! Removing statement from crappy inferencer context!!!!!!!!!!!!" << statement;
#endif
    }

    QMutexLocker lock( &d->m_mutex );

    Soprano::Error::ErrorCode error = parentModel()->removeStatement(statement);
    if( error == Soprano::Error::ErrorNone ) {
        if ( statement.isValid() &&
                statement.subject().isResource() &&
                statement.object().isResource() &&
                statement.predicate() == Soprano::Vocabulary::RDF::type() ) {
            removeInferenceStatements( statement.subject().uri(), statement.object().uri() );
        }
        return Soprano::Error::ErrorNone;
    }
    else {
        return error;
    }
}

Soprano::Error::ErrorCode CrappyInferencer2::removeAllStatements(const Soprano::Statement &statement)
{
    if( statement.context() == d->m_inferenceContext ) {
#ifdef NDEBUG
        kDebug() << "No actions are allowed on the crappy inferencer context in release mode!" << statement;
        return Soprano::Error::ErrorInvalidArgument;
#else
        kWarning() << "WARNING!!!!!!!!!!! Removing statements from crappy inferencer context!!!!!!!!!!!!" << statement;
#endif
    }

    QMutexLocker lock( &d->m_mutex );

    //
    // The following conditions need to be met in order for us to need to check for types
    // 1. Object or context need to be valid. Otherwise our types are removed anyway
    // 2. Object needs to be empty or a resource. Literals are never types
    // 3. Predicate needs to be empty or rdf:type. Otherwise the call is not related to types.
    //
    if ( statement.context() != d->m_inferenceContext &&
            ( ( statement.predicate().isEmpty() ||
               statement.predicate() == Soprano::Vocabulary::RDF::type() ) &&
             ( statement.object().isEmpty() ||
              statement.object().isResource() ) &&
             ( statement.object().isValid() ||
              statement.context().isValid() ) ) ) {
        //
        // If both resource and type are defined we can simply call removeInferenceStatements() on them
        //
        if( statement.subject().isResource() &&
                statement.object().isResource() ) {
            const Soprano::Error::ErrorCode error = parentModel()->removeAllStatements(statement);
            if( error == Soprano::Error::ErrorNone ) {
                removeInferenceStatements( statement.subject().uri(), statement.object().uri() );
            }
            return error;
        }

        //
        // if only the type is undefined we get the types and then call removeInferenceStatements(). This avoids
        // performing the query in the latter method over and over for each type
        //
        else if( statement.subject().isValid() ) {
            //
            // we need to first get the types to remove, then remove the statements, and only then call
            // removeInferenceStatements(). This is since the latter makes use of the types in the model
            //
            QString query;
            if( statement.context().isValid() ) {
                query = QString::fromLatin1("select ?t where { graph %2 { %1 a ?t . } . }")
                        .arg(statement.subject().toN3(),
                             statement.context().toN3());
            }
            else {
                query = QString::fromLatin1("select ?t where { graph ?g { %1 a ?t . } . FILTER(?g!=%2) . }")
                        .arg(statement.subject().toN3(),
                             Soprano::Node::resourceToN3(crappyInferenceContext()));
            }
            const QList<Soprano::Node> types = executeQuery( query, Soprano::Query::QueryLanguageSparql ).iterateBindings(0).allNodes();
            const Soprano::Error::ErrorCode error = parentModel()->removeAllStatements(statement);
            if( error == Soprano::Error::ErrorNone ) {
                removeInferenceStatements( statement.subject().uri(), types );
            }
            return error;
        }

        //
        // If the subject is not defined but the type is we need to update several resources. There is no way around
        // calling removeInferenceStatements() for each resource.
        //
        else if( statement.object().isValid() ) {
            //
            // We need to get the resources before actually removing the statements since otherwise we might miss them
            //
            QString query;
            if( statement.context().isValid() ) {
                query = QString::fromLatin1("select ?r where { graph %2 { ?r a %1 . } . }")
                        .arg(statement.object().toN3(),
                             statement.context().toN3());
            }
            else {
                query = QString::fromLatin1("select ?r where { graph ?g { ?r a %1 . } . FILTER(?g!=%2) . }")
                        .arg(statement.object().toN3(),
                             Soprano::Node::resourceToN3(crappyInferenceContext()));
            }
            const QList<Soprano::Node> resources = executeQuery( query, Soprano::Query::QueryLanguageSparql ).iterateBindings(0).allNodes();
            const Soprano::Error::ErrorCode error = parentModel()->removeAllStatements(statement);
            if( error == Soprano::Error::ErrorNone ) {
                Q_FOREACH( const Soprano::Node& res, resources ) {
                    removeInferenceStatements( res.uri(), statement.object().uri() );
                }
            }
            return error;
        }

        //
        // Neither subject nor object are defined
        //
        else {
            QHash<QUrl, QList<QUrl> > resTypeHash;
            QString query;
            if( statement.context().isValid() ) {
                query = QString::fromLatin1("select distinct ?r ?t where { graph %1 { ?r a ?t . } . }")
                        .arg(statement.context().toN3());
            }
            else {
                query = QString::fromLatin1("select distinct ?r ?t where { graph ?g { ?r a ?t . } . FILTER(?g!=%1) . }")
                        .arg(Soprano::Node::resourceToN3(crappyInferenceContext()));
            }
            Soprano::QueryResultIterator it = parentModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );
            while( it.next() ) {
                resTypeHash[it["r"].uri()].append( it["t"].uri() );
            }
            const Soprano::Error::ErrorCode error = parentModel()->removeAllStatements(statement);
            if( error == Soprano::Error::ErrorNone ) {
                QHash<QUrl, QList<QUrl> >::const_iterator end = resTypeHash.constEnd();
                for( QHash<QUrl, QList<QUrl> >::const_iterator it = resTypeHash.constBegin(); it != end; ++it ) {
                    removeInferenceStatements( it.key(), it.value() );
                }
            }
            return error;
        }
    }
    else {
        return parentModel()->removeAllStatements(statement);
    }
}

QUrl CrappyInferencer2::crappyInferenceContext() const
{
    return d->m_inferenceContext;
}

void CrappyInferencer2::updateInferenceIndex()
{
    kDebug();

    QMutexLocker lock( &d->m_mutex );

    // we only create ontology statements - we need to mark the graph as such
    parentModel()->addStatement(d->m_inferenceContext, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::Ontology(), d->m_inferenceContext);


    // Build superclasses hash
    // ==============================================

    // clear cache
    d->m_superClasses.clear();

    // cache subClassOf relations (only the original ones)
    QMultiHash<QUrl, QUrl> rdfsSubClassRelations;
    Soprano::QueryResultIterator it = executeQuery( QString::fromLatin1("select ?x ?y where { graph ?g { ?x %1 ?y . } . "
                                                                        "FILTER(?g != <urn:crappyinference:inferredtriples>) . "
                                                                        "FILTER(?x!=?y) . }" )
                                                    .arg( Soprano::Node::resourceToN3( Soprano::Vocabulary::RDFS::subClassOf() ) ),
                                                    Soprano::Query::QueryLanguageSparql );
    while ( it.next() ) {
        rdfsSubClassRelations.insert( it["x"].uri(), it["y"].uri() );
    }

    // complete the map of super classes
    Q_FOREACH( const QUrl& type, rdfsSubClassRelations.keys() ) {
        QSet<QUrl> visitedClasses;
        d->buildSuperClassesHash( type, visitedClasses, rdfsSubClassRelations );
    }

    // get the classes that do not have any parent class
    it = executeQuery( QString::fromLatin1("select ?c where { ?c a %1 . OPTIONAL { ?c %2 ?p . } . FILTER(!BOUND(?p)) . }" )
                      .arg( Soprano::Node::resourceToN3( Soprano::Vocabulary::RDFS::Class() ) )
                      .arg( Soprano::Node::resourceToN3( Soprano::Vocabulary::RDFS::subClassOf() ) ),
                      Soprano::Query::QueryLanguageSparql );
    while ( it.next() ) {
        d->m_superClasses.insert( it["c"].uri(), QSet<QUrl>() );
    }

    // add rdfs:Resource as parent for all classes
    QMutableHashIterator<QUrl, QSet<QUrl> > rdfsResIt( d->m_superClasses );
    while( rdfsResIt.hasNext() ) {
        rdfsResIt.next();
        rdfsResIt.value().insert(Soprano::Vocabulary::RDFS::Resource());
    }

#ifndef NDEBUG
    // count for debugging
    int cnt = 0;
    for( QHash<QUrl, QSet<QUrl> >::const_iterator it = d->m_superClasses.constBegin();
         it != d->m_superClasses.constEnd(); ++it ) {
        //kDebug() << it.key() << "->" << it.value();
        cnt += it.value().count();
    }
    kDebug() << "Number of classes:           " << d->m_superClasses.count();
    kDebug() << "Number of subclass relations:" << cnt;
#endif
}

void CrappyInferencer2::updateAllResources()
{
    kDebug();
    if( !d->m_updateAllResourcesThread ) {
        d->m_updateAllResourcesThread = new UpdateAllResourcesThread(this);
        connect(d->m_updateAllResourcesThread, SIGNAL(finished()), this, SIGNAL(allResourcesUpdated()));
    }
    d->m_updateAllResourcesThread->start();
}

void CrappyInferencer2::addInferenceStatements( const QUrl& resource, const QUrl& type )
{
    return addInferenceStatements( resource, QSet<QUrl>() << type );
}

void CrappyInferencer2::addInferenceStatements( const QUrl& resource, const QSet<QUrl>& types )
{
    // add missing types
    bool haveVisibleType = false;
    QSet<QUrl> superTypes;
    Q_FOREACH( const QUrl& type, types ) {
        superTypes += d->superClasses(type);
        if( !haveVisibleType && d->m_typeVisibilityTree->isUserVisible(type) )
            haveVisibleType = true;
    }
    QSet<QUrl>::const_iterator end = superTypes.constEnd();
    for( QSet<QUrl>::const_iterator typeIt = superTypes.constBegin(); typeIt != end; ++typeIt ) {
        parentModel()->addStatement(resource,
                                    Soprano::Vocabulary::RDF::type(),
                                    *typeIt,
                                    crappyInferenceContext());
    }

    // the visibility can only change if we add a visible type
    if( haveVisibleType ) {
        setVisibility( resource, true );
    }
}

void CrappyInferencer2::removeInferenceStatements(const QUrl &resource, const QUrl &type)
{
    removeInferenceStatements( resource, QList<QUrl>() << type );
}

void CrappyInferencer2::removeInferenceStatements( const QUrl& resource, const QList<Soprano::Node>& types )
{
    QList<QUrl> uris;
    Q_FOREACH( const Soprano::Node& node, types )
        uris << node.uri();
    removeInferenceStatements( resource, uris );
}

// Always called *after* removing the actual statements
void CrappyInferencer2::removeInferenceStatements(const QUrl &resource, const QList<QUrl> &types)
{
    // we cannot simply remove all super types since the resource might still have types that are somewhere in that hierarchy

    // determine the types that should be in the database now
    const QSet<QUrl> correctTypes = d->queryResourceTypes(resource);
    QSet<QUrl> correctSuperTypes;
    Q_FOREACH( const QUrl& type, correctTypes ) {
        correctSuperTypes += d->superClasses(type);
    }

    // now compare that list with the ones that we could remove
    QSet<QUrl> typesToRemove;
    Q_FOREACH( const QUrl& type, QSet<QUrl>::fromList(types) )
        typesToRemove += d->superClasses(type);
    typesToRemove -= correctSuperTypes;

    bool haveVisibleType = false;
    Q_FOREACH( const QUrl& typeToRemove, typesToRemove ) {
        if( !haveVisibleType && d->m_typeVisibilityTree->isUserVisible(typeToRemove) )
            haveVisibleType = true;
        parentModel()->removeStatement(resource,
                                       Soprano::Vocabulary::RDF::type(),
                                       typeToRemove,
                                       crappyInferenceContext());
    }

    // remove the visibility if necessary
    // only be removing visible types we can change the actual visibility
    if( haveVisibleType ) {
        setVisibility( resource, !correctTypes.isEmpty() && d->isVisibleFromTypes(correctTypes) );
    }
}

void CrappyInferencer2::setVisibility(const QUrl &resource, bool visible)
{
    // we use an integer instead of a boolean for performance reasons: virtuoso does not support bool and Soprano converts them to strings
    if( visible ) {
        // we only set the visibility if it is not set yet
        executeQuery(QString::fromLatin1("insert into graph %1 { %2 %3 1 . } where { FILTER(!bif:exists((select (1) where { %2 %3 ?v . }))) }")
                     .arg( Soprano::Node::resourceToN3( crappyInferenceContext() ),
                           Soprano::Node::resourceToN3( resource ),
                           Soprano::Node::resourceToN3( Soprano::Vocabulary::NAO::userVisible() ) ),
                     Soprano::Query::QueryLanguageSparql);
    }
    else {
        // we only remove the visibility value we created ourselves
        parentModel()->removeAllStatements(resource,
                                           Soprano::Vocabulary::NAO::userVisible(),
                                           Soprano::Node(),
                                           crappyInferenceContext());
    }
}

#include "crappyinferencer2.moc"
