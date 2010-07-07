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

#ifndef _NEPOMUK_CRAPPY_INFERENCER_H_
#define _NEPOMUK_CRAPPY_INFERENCER_H_

#include <nepomuk/nepomuk_export.h>

#include <Soprano/FilterModel>
#include <Soprano/Node> // for qHash(QUrl)

#include <QtCore/QMultiHash>

namespace Soprano {
    class Graph;
}

namespace Nepomuk {
    /**
     * The CrappyInferencer is based on work by Gunnar Grimnes and Leo Sauermann
     * for the Nepomuk project.
     *
     * As they state, the CrappyInferencer(TM), it is unsound, incomplete and at times
     * completely crazy. It only covers 3 cases:
     *
     * \li Inverse Properties: (x, p1, y) => (y, p2, x) - where ( p1, nrl:inverseProperty, p2 )
     * \li ABox Subclass Inference: (x, rdfs:subClass, y) => (x, rdfs:subClass, z)
     * for all z that is super-classes of y
     * \li ABox SubProperty Inference: (x, rdfs:subProperty, y) => (x, rdfs:subProperty, z)
     * for all z that is super-properties of y
     *
     * The full RDFS entailments are described in
     * <a href="http://www.w3.org/TR/rdf-mt/#RDFSRules"> RDFS entailment rules</a>.
     *
     * For subclass and subproperty relations, the self-referencing relation is
     * added when the UU type rdfs:Class is added.
     *
     * Note that for inverseProperty the order of insertion is important, i.e. when
     * adding a new (a, inverseProperty, b), existing triples using a or b will not
     * be updated. Yes, this makes it wrong and inconsistent. I might fix this
     * later.
     *
     * \section crappy_queries Queries
     *
     * No type inference is performed for queries, so to get "inference" you have to
     * query for rdfs:subClassOf relationship. For example, imagine you want all
     * instance of MyClass (and all it's subclasses), you need this query:
     *
     * \code
     * SELECT ?x WHERE { ?x a ?t . ?t rdfs:subClassOf prefix:MyClass }
     * \endcode
     *
     * If you really need to know the direct subClass of something, you need to rely
     * on the context information using a query like the following (see also crappyInferenceContext()):
     *
     * \code
     * SELECT ?x WHERE { GRAPH ?g { ?x rdfs:subClassOf prefix:MyClass . } .
     *                   FILTER(?g != <urn:crappyinference:inferredtriples>) . }
     * \endcode
     *
     * \section crappy_deleting Deleting Triples
     *
     * \li Deleting a triple with a predicate that has an inverse will also delete
     * the inverse triple.
     * \li Deleting subClassOf, subPropertyOf or nrl:inverseProperty triples will
     * <strong>NOT</strong> attempt to "fix" the triples already inferred from
     * this. Only future insertions will be correct. Yes this means it will be
     * inconsistent and wrong. I said it was crappy. I will not fix this. For
     * instance, if (A subclass B), (B subclass C) is in the store and (B subclass
     * C) is deleted, I will still think A is a subClass of C.
     *
     * \author Sebastian Trueg <trueg@kde.org>
     */
    class CrappyInferencer : public Soprano::FilterModel
    {
        Q_OBJECT

    public:
        CrappyInferencer( Soprano::Model* parentModel = 0 );
        ~CrappyInferencer();

        /**
         * \reimpl to update internal structures
         */
        void setParentModel( Soprano::Model* model );

        Soprano::Error::ErrorCode addStatement( const Soprano::Statement& statement );
        Soprano::Error::ErrorCode removeStatement( const Soprano::Statement& statement );
        Soprano::Error::ErrorCode removeAllStatements( const Soprano::Statement& statement );

        using Soprano::FilterModel::addStatement;
        using Soprano::FilterModel::removeStatement;
        using Soprano::FilterModel::removeAllStatements;

        /**
         * All inferred triples are stored in the one crappy inference graph.
         * Thus, removing inference is as simple as removing this one graph.
         */
        QUrl crappyInferenceContext() const;

    private:
        /**
         * Called by addStatement.
         *
         * \param statement The statement to infer
         * \param subRelationOf A map of sub relations to use (will be either subClasses or subProperties)
         * \param superRelationOf A map of super relations to use (will be either superClasses or superProperties)
         */
        void addInference( const Soprano::Statement& statement,
                           QMultiHash<QUrl, QUrl>& subRelationOf,
                           QMultiHash<QUrl, QUrl>& superRelationOf );

        /**
         * Called by addInference
         */
        void addInference( const Soprano::Statement& statement,
                           QMultiHash<QUrl, QUrl>& subRelationOf,
                           QMultiHash<QUrl, QUrl>& superRelationOf,
                           Soprano::Graph& addedStatements );

        /**
         * Called in removeStatement and removeAllStatements.
         * \param s Statement to remove, this will not actually be removed
         * \param statementsToRemove The statements that need to be removed for the inference to be cleaned up
         *
         * This method will not actually remove any statements, only add them to statementsToRemove. So it can
         * be used in a Soprano iterator loop.
         */
        void removeStatementInference( const Soprano::Statement& s, Soprano::Graph& statementsToRemove );

        void removeInference( const Soprano::Statement& statement,
                              QMultiHash<QUrl, QUrl>& subRelationOf,
                              QMultiHash<QUrl, QUrl>& superRelationOf,
                              Soprano::Graph& statementsToRemove );

        void removeInference( const Soprano::Statement& statement,
                              QMultiHash<QUrl, QUrl>& subRelationOf,
                              QMultiHash<QUrl, QUrl>& superRelationOf,
                              Soprano::Graph& statementsToRemove,
                              Soprano::Graph& removedStatements );

        void createInferenceIndex();

        class Private;
        Private* const d;
    };
}

#endif
