/* This file is part of the Nepomuk Project
   Copyright (c) 2009 Sebastian Trueg <trueg@kde.org>

   Based on CrappyInferencingConnection.java and CrappyInferencingSail.java
   Copyright (c) 2006-2009, NEPOMUK Consortium

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "crappyinferencer.h"

#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Statement>
#include <Soprano/StatementIterator>
#include <Soprano/Node>
#include <Soprano/Graph>
#include <Soprano/QueryResultIterator>


//
// Hint: Inverse property handling has been disabled but the code kept for possible future use or educational value.
//       Handling inverse properties here does not make sense as the crappy inference is only applied to the ontologies,
//       not the data itself.
//

class Nepomuk::CrappyInferencer::Private
{
public:
    QUrl infoContext() const;

//    QHash<QUrl, QUrl> m_inverseProperties;
    QMultiHash<QUrl, QUrl> m_subClasses;
    QMultiHash<QUrl, QUrl> m_superClasses;
    QMultiHash<QUrl, QUrl> m_subProperties;
    QMultiHash<QUrl, QUrl> m_superProperties;

    QUrl m_infoContext;
};


QUrl Nepomuk::CrappyInferencer::Private::infoContext() const
{
    return m_infoContext;
}


Nepomuk::CrappyInferencer::CrappyInferencer( Soprano::Model* parentModel )
    : FilterModel( parentModel ),
      d( new Private() )
{
    d->m_infoContext = QUrl::fromEncoded("urn:crappyinference:inferredtriples");
    if ( parentModel )
        createInferenceIndex();
}


Nepomuk::CrappyInferencer::~CrappyInferencer()
{
    delete d;
}


void Nepomuk::CrappyInferencer::setParentModel( Soprano::Model* model )
{
    FilterModel::setParentModel( model );
    createInferenceIndex();
}


QUrl Nepomuk::CrappyInferencer::crappyInferenceContext() const
{
    return d->infoContext();
}


Soprano::Error::ErrorCode Nepomuk::CrappyInferencer::addStatement( const Soprano::Statement& statement )
{
    //
    // Handle the inference
    //
    if ( statement.subject().isResource() &&
         statement.object().isResource() ) {
#if 0
        //
        // handle inverse properties
        //
        if ( statement.predicate() == Soprano::Vocabulary::NRL::inverseProperty() ) {
            d->m_inverseProperties.insert( statement.subject().uri(), statement.object().uri() );
            d->m_inverseProperties.insert( statement.object().uri(), statement.subject().uri() );
            parentModel()->addStatement( statement.object(), statement.predicate(), statement.subject(), d->infoContext() );
        }
        else if ( d->m_inverseProperties.contains( statement.predicate().uri() ) ) {
            parentModel()->addStatement( statement.object(), d->m_inverseProperties[statement.predicate().uri()], statement.subject(), d->infoContext() );
        }

        //
        // infer sub classing
        //
        else
#endif
            if ( statement.predicate() == Soprano::Vocabulary::RDFS::subClassOf() ) {
            addInference( statement, d->m_subClasses, d->m_superClasses );
        }

        //
        // infer sub properties
        //
        else if ( statement.predicate() == Soprano::Vocabulary::RDFS::subPropertyOf() ) {
            addInference( statement, d->m_subProperties, d->m_superProperties );
        }

        //
        // RDFS entailment 6 and 10 (self-sub-relations)
        //
        else if ( statement.predicate() == Soprano::Vocabulary::RDF::type() ) {
            if ( statement.object() == Soprano::Vocabulary::RDF::Property() ) {
                parentModel()->addStatement( statement.subject(), Soprano::Vocabulary::RDFS::subPropertyOf(), statement.subject(), d->infoContext() );
            }
            else if ( statement.object() == Soprano::Vocabulary::RDFS::Class() ) {
                parentModel()->addStatement( statement.subject(), Soprano::Vocabulary::RDFS::subClassOf(), statement.subject(), d->infoContext() );
            }
        }
    }

    return parentModel()->addStatement( statement );
}


Soprano::Error::ErrorCode Nepomuk::CrappyInferencer::removeStatement( const Soprano::Statement& statement )
{
    if ( statement.subject().isResource() &&
         statement.object().isResource() ) {
        Soprano::Graph statementsToRemove;
        removeStatementInference( statement, statementsToRemove );
        parentModel()->removeStatements( statementsToRemove.toList() );
    }
    return parentModel()->removeStatement( statement );
}


Soprano::Error::ErrorCode Nepomuk::CrappyInferencer::removeAllStatements( const Soprano::Statement& statement )
{
    if ( ( !statement.subject().isValid() || statement.subject().isResource() ) &&
         ( !statement.object().isValid() || statement.object().isResource() ) ) {
        Soprano::Graph statementsToRemove;
        Soprano::StatementIterator it = FilterModel::listStatements( statement );
        while ( it.next() ) {
            removeStatementInference( *it, statementsToRemove );
        }

        parentModel()->removeStatements( statementsToRemove.toList() );
    }

    return FilterModel::removeAllStatements( statement );
}


void Nepomuk::CrappyInferencer::removeStatementInference( const Soprano::Statement& s, Soprano::Graph& statementsToRemove )
{
#if 0
    //
    // handle inverse properties
    //
    if ( s.predicate() == Soprano::Vocabulary::NRL::inverseProperty() ) {
        d->m_inverseProperties.remove( s.subject().uri() );
        d->m_inverseProperties.remove( s.object().uri() );
        statementsToRemove.addStatement( s.object(), s.predicate(), s.subject(), d->infoContext() );
    }
    else if ( d->m_inverseProperties.contains( s.predicate().uri() ) ) {
        // remove both - to ensure at least something vaguely like
        // consistency.
        statementsToRemove.addStatement( s.object(), d->m_inverseProperties[s.predicate().uri()], s.subject(), d->infoContext() );
    }

    //
    // remove subclass inference
    //
    else
#endif
        if ( s.predicate() == Soprano::Vocabulary::RDFS::subClassOf() ) {
        removeInference( s, d->m_subClasses, d->m_superClasses, statementsToRemove );
    }

    //
    // remove subproperty inference
    //
    else if ( s.predicate() == Soprano::Vocabulary::RDFS::subPropertyOf() ) {
        removeInference( s, d->m_subProperties, d->m_superProperties, statementsToRemove );
    }

    //
    // RDFS entailment 6 and 10 (self-sub-relations)
    //
    else if ( s.predicate() == Soprano::Vocabulary::RDF::type() ) {
        if ( s.object() == Soprano::Vocabulary::RDF::Property() ) {
            statementsToRemove.addStatement( s.subject(), Soprano::Vocabulary::RDFS::subPropertyOf(), s.subject(), d->infoContext() );
        }
        else if ( s.object() == Soprano::Vocabulary::RDFS::Class() ) {
            statementsToRemove.addStatement( s.subject(), Soprano::Vocabulary::RDFS::subClassOf(), s.subject(), d->infoContext() );
        }
    }
}


void Nepomuk::CrappyInferencer::removeInference( const Soprano::Statement& statement,
                                                 QMultiHash<QUrl, QUrl>& subRelationOf,
                                                 QMultiHash<QUrl, QUrl>& superRelationOf,
                                                 Soprano::Graph& statementsToRemove )
{
    subRelationOf.remove( statement.subject().uri(), statement.object().uri() );
    subRelationOf.remove( statement.object().uri(), statement.subject().uri() );

    Soprano::Graph removedStatements;
    removeInference( statement, subRelationOf, superRelationOf, statementsToRemove, removedStatements );
}


void Nepomuk::CrappyInferencer::removeInference( const Soprano::Statement& statement,
                                                 QMultiHash<QUrl, QUrl>& subRelationOf,
                                                 QMultiHash<QUrl, QUrl>& superRelationOf,
                                                 Soprano::Graph& statementsToRemove,
                                                 Soprano::Graph& removedStatements )
{
    Soprano::Statement s( statement );
    s.setContext( d->infoContext() );

    if ( !removedStatements.containsStatement( s ) ) {
        // remove the statement
        statementsToRemove << s;
        removedStatements << s;

        // infer sub relations
        foreach( const QUrl& uri, subRelationOf.values( statement.object().uri() ) ) {
            s.setObject( uri );
            removeInference( s, subRelationOf, superRelationOf, statementsToRemove, removedStatements );
        }

        // infer super relations
        s.setObject( statement.object() );
        foreach( const QUrl& uri, superRelationOf.values( statement.subject().uri() ) ) {
            s.setSubject( uri );
            removeInference( s, subRelationOf, superRelationOf, statementsToRemove, removedStatements );
        }
    }
}


void Nepomuk::CrappyInferencer::addInference( const Soprano::Statement& statement,
                                              QMultiHash<QUrl, QUrl>& subRelationOf,
                                              QMultiHash<QUrl, QUrl>& superRelationOf )
{
    // update status vars
    subRelationOf.insert( statement.subject().uri(), statement.object().uri() );
    superRelationOf.insert( statement.object().uri(), statement.subject().uri() );

    // do the actual inference (this will add the statement again, but we do not care)
    Soprano::Graph graph;
    addInference( statement, subRelationOf, superRelationOf, graph );
}


void Nepomuk::CrappyInferencer::addInference( const Soprano::Statement& statement,
                                              QMultiHash<QUrl, QUrl>& subRelationOf,
                                              QMultiHash<QUrl, QUrl>& superRelationOf,
                                              Soprano::Graph& addedStatements )
{
    Soprano::Statement s( statement );
    s.setContext( d->infoContext() );

    if ( !addedStatements.containsStatement( s ) ) {

        // add the statement
        parentModel()->addStatement( s );
        addedStatements.addStatement( s );

        // infer sub relations
        foreach( const QUrl& uri, subRelationOf.values( statement.object().uri() ) ) {
            s.setObject( uri );
            addInference( s, subRelationOf, superRelationOf, addedStatements );
        }

        // infer super relations
        s.setObject( statement.object() );
        foreach( const QUrl& uri, superRelationOf.values( statement.subject().uri() ) ) {
            s.setSubject( uri );
            addInference( s, subRelationOf, superRelationOf, addedStatements );
        }
    }
}


void Nepomuk::CrappyInferencer::createInferenceIndex()
{
    // clear cache
//    d->m_inverseProperties.clear();
    d->m_subClasses.clear();
    d->m_superClasses.clear();
    d->m_subProperties.clear();
    d->m_superProperties.clear();

#if 0
    // cache inverse properties
    Soprano::QueryResultIterator it = executeQuery( QString( "select ?x ?y where { ?x %1 ?y . }" )
                                                    .arg( Soprano::Node::resourceToN3( Soprano::Vocabulary::NRL::inverseProperty() ) ),
                                                    Soprano::Query::QueryLanguageSparql );
    while ( it.next() ) {
        d->m_inverseProperties.insert( it["x"].uri(), it["y"].uri() );
        d->m_inverseProperties.insert( it["y"].uri(), it["x"].uri() );
    }
#endif

    // cache subClassOf relations (only the original ones)
    Soprano::QueryResultIterator it = executeQuery( QString( "select ?x ?y where { graph ?g { ?x %1 ?y . } . FILTER(?g != %2) . }" )
                                                    .arg( Soprano::Node::resourceToN3( Soprano::Vocabulary::RDFS::subClassOf() ) )
                                                    .arg( Soprano::Node::resourceToN3( d->infoContext() ) ),
                                                    Soprano::Query::QueryLanguageSparql );
    while ( it.next() ) {
        d->m_subClasses.insert( it["x"].uri(), it["y"].uri() );
        d->m_superClasses.insert( it["y"].uri(), it["x"].uri() );
    }


    // cache subPropertyOf relations (only the original ones)
    it = executeQuery( QString( "select ?x ?y where { graph ?g { ?x %1 ?y . } . FILTER(?g != %2) . }" )
                       .arg( Soprano::Node::resourceToN3( Soprano::Vocabulary::RDFS::subPropertyOf() ) )
                       .arg( Soprano::Node::resourceToN3( d->infoContext() ) ),
                       Soprano::Query::QueryLanguageSparql );
    while ( it.next() ) {
        d->m_subProperties.insert( it["x"].uri(), it["y"].uri() );
        d->m_superProperties.insert( it["y"].uri(), it["x"].uri() );
    }
}

#include "crappyinferencer.moc"
