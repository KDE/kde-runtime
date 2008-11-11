/*
  This file is part of the Nepomuk KDE project.
  Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>

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

#include "searchthread.h"
#include "term.h"
#include "qurlhash.h"

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Resource>
#include <Nepomuk/Types/Property>
#include <Nepomuk/Types/Class>
#include <Nepomuk/Types/Literal>

#include <Soprano/Version>
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Node>
#include <Soprano/Statement>
#include <Soprano/LiteralValue>
#include <Soprano/StatementIterator>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/XMLSchema>
#include <Soprano/Vocabulary/OWL>

#include <KDebug>

#include <QtCore/QTime>



// FIXME: With our cutoff score we might miss results that are hit multiple times and thus, would get their
//        score increased

#warning Make query optimization methods return an invalid term if the query cannot be resolved and handle this as no results

using namespace Soprano;

namespace {
    /**
     * The maximum number of resources that are matched in resolveValues when converting
     * an equals or contains term.
     */
    const int MAX_RESOURCES = 4;


    void mergeInResult( QHash<QUrl, Nepomuk::Search::Result>& results, const Nepomuk::Search::Result& resource ) {
        QHash<QUrl, Nepomuk::Search::Result>::iterator old = results.find( resource.resourceUri() );
        if ( old == results.end() ) {
            results.insert( resource.resourceUri(), resource );
        }
        else {
            // FIXME: how do we join the scores properly? Is adding a good idea? It can certainly not be multiplication!
            Nepomuk::Search::Result& result = *old;
            result.setScore( result.score() + resource.score() );
        }
    }

    void mergeInResults( QHash<QUrl, Nepomuk::Search::Result>& results, const QHash<QUrl, Nepomuk::Search::Result>& otherResults ) {
        for ( QHash<QUrl, Nepomuk::Search::Result>::const_iterator it = otherResults.constBegin();
              it != otherResults.constEnd(); ++it ) {
            mergeInResult( results, it.value() );
        }
    }

    // This is a copy of Soprano::Index::IndexFilterModel::encodeStringForLuceneQuery
    // which we do not use to prevent linking to sopranoindex
    QString luceneQueryEscape( const QString& s ) {
        /* Chars to escape: + - && || ! ( ) { } [ ] ^ " ~  : \ */

        static QRegExp rx( "([\\-" + QRegExp::escape( "+&|!(){}[]^\"~:\\" ) + "])" );
        QString es( s );
        es.replace( rx, "\\\\1" );
        return es;
    }

    QString luceneQueryEscape( const QUrl& s ) {
        return luceneQueryEscape( QString::fromAscii( s.toEncoded() ) );
    }

    QString createLuceneLiteralQuery( const QString& escaped ) {
        if ( escaped.contains( QRegExp( "\\s" ) ) ) {
            return "\"" + escaped + "\"";
        }
        else {
            return escaped;
        }
    }

    QString createLuceneQuery( const Nepomuk::Search::SearchNode& node ) {
        if ( node.term.type() == Nepomuk::Search::Term::LiteralTerm ) {
            return createLuceneLiteralQuery( luceneQueryEscape( node.term.value().toString() ) );
        }
        else if ( node.term.type() == Nepomuk::Search::Term::ComparisonTerm ) {
            return luceneQueryEscape( node.term.property() ) + ':' + createLuceneLiteralQuery( luceneQueryEscape( node.term.subTerms().first().value().toString() ) );
        }
        else {
            Q_ASSERT( node.term.type() == Nepomuk::Search::Term::AndTerm ||
                      node.term.type() == Nepomuk::Search::Term::OrTerm );

            QStringList sq;
            foreach( const Nepomuk::Search::SearchNode& n, node.subNodes ) {
                sq += createLuceneQuery( n );
            }
            if ( node.term.type() == Nepomuk::Search::Term::AndTerm ) {
                return " ( " + sq.join( " AND " ) + " ) ";
            }
            else {
                return " ( " + sq.join( " OR " ) + " ) ";
            }
        }
    }

    QString comparatorString( Nepomuk::Search::Term::Comparator c ) {
        switch( c ) {
        case Nepomuk::Search::Term::Contains:
            return ":";
        case Nepomuk::Search::Term::Equal:
            return "=";
        case Nepomuk::Search::Term::Greater:
            return ">";
        case Nepomuk::Search::Term::Smaller:
            return "<";
        case Nepomuk::Search::Term::GreaterOrEqual:
            return ">=";
        case Nepomuk::Search::Term::SmallerOrEqual:
            return "<=";
        }
        // make gcc happy
        return QString();
    }


    bool isNumberLiteralValue( const Soprano::LiteralValue& value ) {
        return value.isInt() || value.isInt64() || value.isUnsignedInt() || value.isUnsignedInt64() || value.isDouble();
    }


    QString createGraphPattern( const Nepomuk::Search::SearchNode& node, int& varCnt, const QString& varName = QString( "?r" ) )
    {
        switch( node.term.type() ) {
        case Nepomuk::Search::Term::ComparisonTerm: {

            Nepomuk::Search::Term subTerm( node.term.subTerms().first() );

            //
            // is the subterm (we only support one ATM) a final term (no further subterms)
            // -> actually match the literal or resource
            //
            if ( subTerm.type() == Nepomuk::Search::Term::ResourceTerm ||
                 subTerm.type() == Nepomuk::Search::Term::LiteralTerm ) {
                if( node.term.comparator() != Nepomuk::Search::Term::Equal ) {
                    // For numbers there is no need for quotes + this way we can handle all the xsd decimal types
                    // FIXME: it may be necessary to escape stuff
                    QString filter = QString( "?var%1 %2 " )
                                     .arg( ++varCnt )
                                     .arg( comparatorString( node.term.comparator() ) );
                    if ( isNumberLiteralValue( subTerm.value() ) ) {
                        filter += subTerm.value().toString();
                    }
                    else {
                        Nepomuk::Types::Property prop( node.term.property() );
                        filter += QString( "\"%1\"" ).arg( subTerm.value().toString() );
                        if ( prop.literalRangeType().dataTypeUri().isValid() )
                            filter += QString( "^^<%1>" ).arg( prop.literalRangeType().dataTypeUri().toString() );
                    }

                    return QString( "%1 <%2> ?var%3 . FILTER(%4) . " )
                        .arg( varName )
                        .arg( QString::fromAscii( node.term.property().toEncoded() ) )
                        .arg( varCnt )
                        .arg( filter );
                }
                else {
                    if ( subTerm.type() == Nepomuk::Search::Term::ResourceTerm ) {
                        return QString( "%1 <%2> <%3> . " )
                            .arg( varName )
                            .arg( QString::fromAscii( node.term.property().toEncoded() ) )
                            .arg( QString::fromAscii( subTerm.resource().toEncoded() ) );
                    }
                    else if ( Nepomuk::Types::Property( node.term.property() ).range().isValid() ) {
                        return QString( "%7 <%1> ?x . { ?x <%2> \"%3\"^^<%4> . } UNION { ?x <%5> \"%3\"^^<%4>.  } UNION { ?x <%6> \"%3\"^^<%4> . }" )
                            .arg( QString::fromAscii( node.term.property().toEncoded() ) )
                            .arg( Soprano::Vocabulary::RDFS::label().toString() )
                            .arg( subTerm.value().toString() )
                            .arg( Soprano::Vocabulary::XMLSchema::string().toString() )
                            .arg( Soprano::Vocabulary::NAO::prefLabel().toString() )
                            .arg( Soprano::Vocabulary::NAO::identifier().toString() )
                            .arg( varName );
                    }
                    else {
                        return QString( "%1 <%2> \"%3\"^^<%4> . " )
                            .arg( varName )
                            .arg( QString::fromAscii( node.term.property().toEncoded() ) )
                            .arg( subTerm.value().toString() )
                            .arg( Nepomuk::Types::Property( node.term.property() ).literalRangeType().dataTypeUri().toString() );
                    }
                }
            }

            //
            // Is the subterm not final, i.e. has further subterms
            // -> combine graph pattern with subterm graph pattern
            //
            else {
                QString bridgeVarName = QString( "?var%1" ).arg( ++varCnt );
                return QString( "%1 <%2> %3 . " )
                    .arg( varName )
                    .arg( QString::fromAscii( node.term.property().toEncoded() ) )
                    .arg( bridgeVarName )
                    + createGraphPattern( node.subNodes.first(), varCnt, bridgeVarName );
            }
        }

        case Nepomuk::Search::Term::AndTerm: {
            QString s( "{ " );
            foreach( const Nepomuk::Search::SearchNode& n, node.subNodes ) {
                s += createGraphPattern( n, varCnt );
            }
            s += "} ";
            return s;
        }

        case Nepomuk::Search::Term::OrTerm: {
            QStringList s;
            foreach( const Nepomuk::Search::SearchNode& n, node.subNodes ) {
                s += createGraphPattern( n, varCnt );
            }
            Q_ASSERT( !s.isEmpty() );
            return "{ " + s.join( " } UNION { " ) + " } ";
        }

        default:
            Q_ASSERT_X( 0, "createGraphPattern", "unsupported Term type" );
        }

        return QString();
    }
}


Nepomuk::Search::SearchThread::SearchThread( QObject* parent )
    : QThread( parent )
{
}


Nepomuk::Search::SearchThread::~SearchThread()
{
}


void Nepomuk::Search::SearchThread::query( const Query& term, double cutOffScore )
{
    if( isRunning() ) {
        cancel();
    }

    kDebug() << term << cutOffScore;

    m_canceled = false;
    m_searchTerm = term;
    m_cutOffScore = cutOffScore;
    m_numResults = 0;

    start();
}


void Nepomuk::Search::SearchThread::cancel()
{
    m_canceled = true;
    wait();
}


void Nepomuk::Search::SearchThread::run()
{
    QTime time;
    time.start();

    if ( m_searchTerm.type() == Query::PlainQuery ) {
        kDebug() << "Plain Query:    " << m_searchTerm;
        Term t = resolveFields( m_searchTerm.term() );
        kDebug() << "Fields resolved:" << t;
        t = resolveValues( t );
        kDebug() << "Values resolved:" << t;
        t = optimize( t );
        kDebug() << "Optimized query:" << t;

        search( splitLuceneSparql( t ) /*optimize( resolveValues( resolveFields( m_searchTerm ) ) )*/, 1.0, true );
    }
    else {
        // FIXME: once we have the Soprano query API it should be simple to add the requestProperties here
        // for now we do it the hacky way
        QString query = m_searchTerm.sparqlQuery();
        int pos = query.indexOf( QLatin1String( "where" ) );
        if ( pos > 0 ) {
            query.insert( pos, buildRequestPropertyVariableList() + ' ' );
            pos = query.lastIndexOf( '}' );
            if ( pos > 0 ) {
                query.insert( pos, ' ' + buildRequestPropertyPatterns() + ' ' );
            }
        }

        sparqlQuery( query, 1.0, true );
    }

    kDebug() << time.elapsed();
}


Nepomuk::Search::Term Nepomuk::Search::SearchThread::resolveFields( const Term& term )
{
    switch( term.type() ) {
    case Term::AndTerm:
    case Term::OrTerm: {
        Term newTerm;
        newTerm.setType( term.type() );
        QList<Term> terms = term.subTerms();
        foreach( const Term& t, terms ) {
            if ( m_canceled ) break;
            newTerm.addSubTerm( resolveFields( t ) );
        }
        return newTerm;
    }


    case Term::ComparisonTerm: {
        Term newTerm( term );
        Term subTerm = term.subTerms().first();
        if ( subTerm.type() != Term::LiteralTerm &&
             subTerm.type() != Term::ResourceTerm ) {
            newTerm.setSubTerms( QList<Term>() << resolveFields( subTerm ) );
        }

        if ( !newTerm.property().isValid() ) {
            // FIXME: use the score of the field search as boost factors
            QList<QUrl> properties = matchFieldName( term.field() );
            if ( properties.count() > 0 ) {
                if ( properties.count() == 1 ) {
                    newTerm.setProperty( properties.first() );
                    return newTerm;
                }
                else {
                    Term orTerm;
                    orTerm.setType( Term::OrTerm );
                    foreach( const QUrl& property, properties ) {
                        Term t( newTerm );
                        t.setProperty( property );
                        orTerm.addSubTerm( t );
                    }
                    return orTerm;
                }
            }
            else {
                kDebug() << "Failed to resolve field" << term.field() << "to any property!";
                return Term();
            }
        }
    }

    default:
        return term;
    }
}


// precondition: resolveFields needs to be run before this one as it only touches properties
Nepomuk::Search::Term Nepomuk::Search::SearchThread::resolveValues( const Term& term )
{
    switch( term.type() ) {
    case Term::AndTerm:
    case Term::OrTerm: {
        Term newTerm;
        newTerm.setType( term.type() );
        QList<Term> terms = term.subTerms();
        foreach( const Term& t, terms ) {
            if ( m_canceled ) break;
            newTerm.addSubTerm( resolveValues( t ) );
        }
        return newTerm;
    }


    case Term::ComparisonTerm: {
        // FIXME: we could also handle this via lucene for literals but what is better?
        // with lucene we have the additional work of getting the requestProperties

        // FIXME: handle subqueries

        //
        // ComparisonTerm Terms can contain subterms that again. We do not support
        // arbitrary subterms but only comparator terms. Here we will only resolve the
        // last one since all others will be handled in a single SPARQL query.
        //
        // Also, non-comtains comparators are handled in the SPARQL query as well.
        //
        // Thus, in the end we only resolve literal contains terms.
        //
        if ( term.comparator() == Term::Contains &&
             term.subTerms().first().type() == Term::LiteralTerm ) {

            Q_ASSERT ( term.property().isValid() );

            // we only need to augment terms that have a property with
            // a non-literal range. These will never hit in a lucene query
            // anyway
            Nepomuk::Types::Property prop( term.property() );
            if ( prop.range().isValid() ) {

                Term orTerm;
                orTerm.setType( Term::OrTerm );

                // FIXME: cache the results as it is very well possible that we search the same multiple times
                // if resolveFields did create an OR term

                // rdfs:label has a higher priority than any other property
                // TODO: without being able to query the resource type simple searching for term.value() is waaaaay to slow
                //QString query = QString( "%1:\"%2\"^4 \"%2\"" )
                QString query = QString( "%1:\"%2\" OR %3:\"%2\" OR %4:\"%2\"" )
                                .arg( luceneQueryEscape( Soprano::Vocabulary::RDFS::label() ) )
                                .arg( term.subTerms().first().value().toString() )
                                .arg( luceneQueryEscape( Soprano::Vocabulary::NAO::prefLabel() ) )
                                .arg( luceneQueryEscape( Soprano::Vocabulary::NAO::identifier() ) );
                Soprano::QueryResultIterator hits = ResourceManager::instance()->mainModel()->executeQuery( query,
                                                                                                            Soprano::Query::QueryLanguageUser,
                                                                                                            "lucene" );

                while ( hits.next() ) {
                    if ( m_canceled ) break;

                    // FIXME: use the lucene score as boost factor
                    QUrl hit = hits.binding( 0 ).uri();
                    if ( prop.range().uri() == Soprano::Vocabulary::RDFS::Resource() ||
                         Nepomuk::Resource( hit ).hasType( prop.range().uri() ) ) {
                        orTerm.addSubTerm( Term( term.property(), hit ) );
                        if ( orTerm.subTerms().count() == MAX_RESOURCES ) {
                            break;
                        }
                    }
                }

                if ( orTerm.subTerms().count() == 1 ) {
                    return orTerm.subTerms().first();
                }
                else if ( orTerm.subTerms().count() ) {
                    return orTerm;
                }
                else {
                    kDebug() << "Failed to match value" << term.subTerms().first().value() << "to any possible resource.";
                    return term;
                }
            }
            else {
                // nothing to do here
                return term;
            }
        }

        // non-literal term or non-contains term -> handled in SPARQL query
        else {
            Term newTerm( term );
            newTerm.setSubTerms( QList<Term>() << resolveValues( term.subTerms().first() ) );
            return newTerm;
        }
    }

    default:
        return term;
    }
}


Nepomuk::Search::Term Nepomuk::Search::SearchThread::optimize( const Term& term )
{
    switch( term.type() ) {
    case Term::AndTerm:
    case Term::OrTerm: {
        QList<Term> subTerms = term.subTerms();
        QList<Term> newSubTerms;
        QList<Term>::const_iterator end( subTerms.constEnd() );
        for ( QList<Term>::const_iterator it = subTerms.constBegin();
              it != end; ++it ) {
            const Term& t = *it;
            Term ot = optimize( t );
            if ( ot.type() == term.type() ) {
                newSubTerms += ot.subTerms();
            }
            else {
                newSubTerms += ot;
            }
        }
        Term newTerm;
        newTerm.setType( term.type() );
        newTerm.setSubTerms( newSubTerms );
        return newTerm;
    }

    default:
        return term;
    }
}


Nepomuk::Search::SearchNode Nepomuk::Search::SearchThread::splitLuceneSparql( const Term& term )
{
    // Goal: separate the terms into 2 groups: literal and resource which are
    // merged with only one AND or OR action. Is that possible?

    // For now we will do this (our query lang does not handle nested queries anyway)
    // LiteralTerm    -> one lucene, no sparql
    // ComparisonTerm -> one lucene, no sparql (resource contains will be resolved to equality above)
    // AndTerm        -> divide all subterms and create two "small" AND terms
    // OrTerm         -> divide all subterms and create two "small" OR terms

    switch( term.type() ) {
    case Term::LiteralTerm:
        return SearchNode( term, SearchNode::Lucene );

    case Term::ComparisonTerm:
        if ( term.comparator() == Term::Contains &&
             term.subTerms().first().type() == Term::LiteralTerm ) {
            // no need for subnides here - we only use the subterm's value
            return SearchNode( term, SearchNode::Lucene );
        }
        else {
            // all subnodes are resolved and can be handled in a SPARQL query
            SearchNode node( term, SearchNode::Sparql );
            node.subNodes += splitLuceneSparql( term.subTerms().first() );
            return node;
        }

    case Term::AndTerm:
    case Term::OrTerm: {
        QList<Term> subTerms = term.subTerms();
        QList<SearchNode> luceneNodes, sparqlNodes, unknownNodes;

        QList<Term>::const_iterator end( subTerms.constEnd() );
        for ( QList<Term>::const_iterator it = subTerms.constBegin();
              it != end; ++it ) {
            SearchNode node = splitLuceneSparql( *it );
            if ( node.type == SearchNode::Lucene ) {
                luceneNodes += node;
            }
            else if ( node.type == SearchNode::Sparql ) {
                sparqlNodes += node;
            }
            else {
                unknownNodes += node;
            }
        }

        if ( luceneNodes.count() && !sparqlNodes.count() && !unknownNodes.count() ) {
            return SearchNode( term, SearchNode::Lucene, luceneNodes );
        }
        else if ( !luceneNodes.count() && sparqlNodes.count() && !unknownNodes.count() ) {
            return SearchNode( term, SearchNode::Sparql, sparqlNodes );
        }
        else if ( !luceneNodes.count() && !sparqlNodes.count() && unknownNodes.count() ) {
            return SearchNode( term, SearchNode::Unknown, unknownNodes );
        }
        else {
            Term newTerm;
            newTerm.setType( term.type() );
            SearchNode andNode( newTerm );
            if ( luceneNodes.count() )
                andNode.subNodes += SearchNode( term, SearchNode::Lucene, luceneNodes );
            if ( sparqlNodes.count() )
                andNode.subNodes += SearchNode( term, SearchNode::Sparql, sparqlNodes );
            if ( unknownNodes.count() )
                andNode.subNodes += SearchNode( term, SearchNode::Unknown, unknownNodes );
            return andNode;
        }
    }

    default:
//        Q_ASSERT_X( 0, "splitLuceneSparql", "invalid term" );
        return SearchNode( Term() );
    }
}


QHash<QUrl, Nepomuk::Search::Result> Nepomuk::Search::SearchThread::search( const SearchNode& node, double baseScore, bool reportResults )
{
    if ( node.type == SearchNode::Lucene ) {
        return luceneQuery( createLuceneQuery( node ), baseScore, reportResults );
    }
    else if ( node.type == SearchNode::Sparql ) {
        return sparqlQuery( createSparqlQuery( node ), baseScore, reportResults );
    }
    else if ( node.term.type() == Term::AndTerm ) {
        return andSearch( node.subNodes, baseScore, reportResults );
    }
    else {
        return orSearch( node.subNodes, baseScore, reportResults );
    }
}


QHash<QUrl, Nepomuk::Search::Result> Nepomuk::Search::SearchThread::andSearch( const QList<SearchNode>& nodes, double baseScore, bool reportResults )
{
    QHash<QUrl, Result> results;
    bool first = true;
    foreach( const SearchNode& node, nodes ) {
        if ( m_canceled ) break;
        // FIXME: the search will restrict the number of results to maxResults although
        //        after the merge we might have less
        QHash<QUrl, Result> termResults = search( node, baseScore, false );
        if ( first ) {
            results = termResults;
            first = false;
        }
        else {
            // intersect the results
            // FIXME: sort by score
            QHash<QUrl, Result>::iterator it = results.begin();
            while ( it != results.end() ) {
                if ( m_canceled ) break;
                QHash<QUrl, Result>::const_iterator termIt = termResults.constFind( it.key() );
                if ( termIt != termResults.constEnd() ) {
                    // update score
                    it.value().setScore( it.value().score() + termIt.value().score() );
                    ++it;
                }
                else {
                    it = results.erase( it );
                }
            }
        }
    }

    if ( reportResults ) {
        for ( QHash<QUrl, Result>::const_iterator it = results.constBegin();
              it != results.constEnd(); ++it ) {
            if ( m_canceled ) break;
            if ( m_searchTerm.limit() > 0 && m_numResults >= m_searchTerm.limit() ) {
                return results;
            }
            else {
                ++m_numResults;
                emit newResult( it.value() );
            }
        }
    }

    return results;
}


QHash<QUrl, Nepomuk::Search::Result> Nepomuk::Search::SearchThread::orSearch( const QList<SearchNode>& nodes, double baseScore, bool reportResults )
{
    QHash<QUrl, Result> results;
    foreach( const SearchNode& node, nodes ) {
        if ( m_canceled ) break;
        // FIXME: sort by score, ie. use the maxResults results with the highest score
        mergeInResults( results, search( node, baseScore, reportResults ) );
    }
    if ( reportResults ) {
        for ( QHash<QUrl, Result>::const_iterator it = results.constBegin();
              it != results.constEnd(); ++it ) {
            if ( m_canceled ) break;
            if ( m_searchTerm.limit() > 0 && m_numResults >= m_searchTerm.limit() ) {
                return results;
            }
            else {
                ++m_numResults;
                emit newResult( it.value() );
            }
        }
    }
    return results;
}


QList<QUrl> Nepomuk::Search::SearchThread::matchFieldName( const QString& field )
{
    kDebug() << field;

    QList<QUrl> results;

    // Step 1: see if we have a direct match to a predicate label
    //         there is no need in selecting unused properties
    QString query = QString( "select distinct ?p where { "
                             "?p <%1> <%2> . "
                             "?p <%3> \"%4\"^^<%5> . "
                             "?x ?p ?y . }" )
                    .arg( Soprano::Vocabulary::RDF::type().toString() )
                    .arg( Soprano::Vocabulary::RDF::Property().toString() )
                    .arg( Soprano::Vocabulary::RDFS::label().toString() )
                    .arg( field )
                    .arg( Soprano::Vocabulary::XMLSchema::string().toString() );
    kDebug() << "Direct match query:" << query;

    Soprano::QueryResultIterator labelHits = ResourceManager::instance()->mainModel()->executeQuery( query,
                                                                                                     Soprano::Query::QueryLanguageSparql );
    if ( !m_canceled ) {
        while ( labelHits.next() ) {
            results << labelHits.binding( "p" ).uri();
            kDebug() << "Found direct match" << labelHits.binding( "p" ).uri();
        }

        if ( results.isEmpty() ) {
            // FIXME: how about we have two repositories: one for the ontologies and one for the data.
            //        I don't think there will be relations between the RDF or Xesam ontology and some
            //        metadata....
            //        Because then queries like the one we are doing here will be more performant since
            //        we do not search the data itself and do not have to filter
            // BUT: What about inference?

            query = QString( "select ?p where { "
                             "?p <%1> <%2> . "
                             "?p <%3> ?label . "
                             "FILTER(REGEX(STR(?label),'%4','i')) . }" )
                    .arg( Soprano::Vocabulary::RDF::type().toString() )
                    .arg( Soprano::Vocabulary::RDF::Property().toString() )
                    .arg( Soprano::Vocabulary::RDFS::label().toString() )
                    .arg( field );
            kDebug() << "Indirect hit query:" << query;
            labelHits = ResourceManager::instance()->mainModel()->executeQuery( query,
                                                                                Soprano::Query::QueryLanguageSparql );
            QString newQuery;
            while ( labelHits.next() ) {
                results << labelHits.binding( "p" ).uri();
                kDebug() << "Found indirect match by label" << labelHits.binding( "p" ).uri();
            }
        }


        if ( results.isEmpty() ) {
            query = QString( "select ?p where { "
                             "?p <%1> <%2> . "
                             "FILTER(REGEX(STR(?p),'%3','i')) . }" )
                    .arg( Soprano::Vocabulary::RDF::type().toString() )
                    .arg( Soprano::Vocabulary::RDF::Property().toString() )
                    .arg( field );
            kDebug() << "Indirect hit query:" << query;
            labelHits = ResourceManager::instance()->mainModel()->executeQuery( query,
                                                                                Soprano::Query::QueryLanguageSparql );
            QString newQuery;
            while ( labelHits.next() ) {
                results << labelHits.binding( "p" ).uri();
                kDebug() << "Found indirect match by name" << labelHits.binding( "p" ).uri();
            }
        }
    }

    return results;
}


QString Nepomuk::Search::SearchThread::createSparqlQuery( const Nepomuk::Search::SearchNode& node )
{
    int varCnt = 0;
    return QString( "select distinct ?r %1 where { graph ?g { ?r a ?type . } . ?g a <%2> . %3 %4 }" )
        .arg( buildRequestPropertyVariableList() )
        .arg( Soprano::Vocabulary::NRL::InstanceBase().toString() )
        .arg( createGraphPattern( node, varCnt ) )
        .arg( buildRequestPropertyPatterns() );
}


QHash<QUrl, Nepomuk::Search::Result> Nepomuk::Search::SearchThread::sparqlQuery( const QString& query, double baseScore, bool reportResults )
{
    kDebug() << query;

    QHash<QUrl, Result> results;

    Soprano::QueryResultIterator hits = ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    while ( hits.next() ) {
        if ( m_canceled ) break;

        Result result = extractResult( hits );
        result.setScore( baseScore );

        kDebug() << "Found result:" << result.resourceUri();

        // these are actual direct hits and we can report them right away
        if ( reportResults ) {
            if ( m_searchTerm.limit() > 0 && m_numResults >= m_searchTerm.limit() ) {
                return results;
            }
            else {
                ++m_numResults;
                emit newResult( result );
            }
        }

        results.insert( result.resourceUri(), result );
    }

    return results;
}


QHash<QUrl, Nepomuk::Search::Result> Nepomuk::Search::SearchThread::luceneQuery( const QString& query, double baseScore, bool reportResults )
{
    QString finalQuery( query );

    // if Soprano is 2.1.64 or newer the storage service does force the indexing or rdf:type which means that
    // we can query it via lucene queries
    // normally for completeness we would have to exclude all the owl and nrl properties but that would make
    // for way to long queries and this should cover most cases anyway
    // since we do not have inference we even need to check subclasses
#if SOPRANO_IS_VERSION(2,1,64)
    finalQuery += QString(" AND NOT %1:%2 AND NOT %1:%3 AND NOT %1:%4 AND NOT %1:%5 AND NOT %1:%6 AND NOT %1:%7")
                  .arg( luceneQueryEscape(Soprano::Vocabulary::RDF::type()) )
                  .arg( luceneQueryEscape(Soprano::Vocabulary::RDF::Property()) )
                  .arg( luceneQueryEscape(Soprano::Vocabulary::RDFS::Class()) )
                  .arg( luceneQueryEscape(Soprano::Vocabulary::OWL::Class()) )
                  .arg( luceneQueryEscape(Soprano::Vocabulary::NRL::InstanceBase()) )
                  .arg( luceneQueryEscape(Soprano::Vocabulary::NRL::Ontology()) )
                  .arg( luceneQueryEscape(Soprano::Vocabulary::NRL::KnowledgeBase()) );
#endif

    kDebug() << finalQuery;

    Soprano::QueryResultIterator hits = ResourceManager::instance()->mainModel()->executeQuery( finalQuery,
                                                                                                Soprano::Query::QueryLanguageUser,
                                                                                                "lucene" );
    QHash<QUrl, Result> results;

    while ( hits.next() ) {
        if ( m_canceled ) break;

        QUrl hitUri = hits.binding( 0 ).uri();
        double hitScore = hits.binding( 1 ).literal().toDouble() * baseScore;

        if ( hitScore >= cutOffScore() ) {
            Result result( hitUri, hitScore );

            if ( !m_searchTerm.requestProperties().isEmpty() ) {
                // FIXME: when merging with results from sparqlQuery there is no need to fetch them twice!
                fetchRequestPropertiesForResource( result );
            }

            // these are actual direct hits and we can report them right away
            if ( reportResults ) {
                if ( m_searchTerm.limit() > 0 && m_numResults >= m_searchTerm.limit() ) {
                    return results;
                }
                else {
                    ++m_numResults;
                    kDebug() << "direct hit:" << hitUri << hitScore;
                    emit newResult( result );
                }
            }

            results.insert( hitUri, result );
        }
        else {
            kDebug() << "Score too low:" << hitUri << hitScore;
        }
    }

    return results;
}


QString Nepomuk::Search::SearchThread::buildRequestPropertyVariableList() const
{
    int numRequestProperties = m_searchTerm.requestProperties().count();
    QString s;
    for ( int i = 1; i <= numRequestProperties; ++i ) {
        s += QString( "?reqProp%1 " ).arg( i );
    }
    return s;
}


QString Nepomuk::Search::SearchThread::buildRequestPropertyPatterns() const
{
    QList<Query::RequestProperty> requestProperties = m_searchTerm.requestProperties();
    QString s;
    int i = 1;
    foreach ( const Query::RequestProperty& rp, requestProperties ) {
        if ( rp.second ) {
            s += "OPTIONAL { ";
        }

        s += QString( "?r <%1> ?reqProp%2 . " ).arg( QString::fromAscii( rp.first.toEncoded() ) ).arg( i++ );

        if ( rp.second ) {
            s += "} ";
        }
    }
    return s;
}


Nepomuk::Search::Result Nepomuk::Search::SearchThread::extractResult( const Soprano::QueryResultIterator& it ) const
{
    Result result( it.binding( 0 ).uri() );

    int i = 1;
    QList<Query::RequestProperty> requestProperties = m_searchTerm.requestProperties();
    foreach ( const Query::RequestProperty& rp, requestProperties ) {
        result.addRequestProperty( rp.first, it.binding( QString("reqProp%1").arg( i++ ) ) );
    }

    // score will be set above
    return result;
}


void Nepomuk::Search::SearchThread::fetchRequestPropertiesForResource( Result& result )
{
    QString q = QString( "select distinct %1 where { %2 }" )
                .arg( buildRequestPropertyVariableList() )
                .arg( buildRequestPropertyPatterns().replace( "?r ", '<' + QString::fromAscii( result.resourceUri().toEncoded() ) + "> " ) );
    kDebug() << q;
    Soprano::QueryResultIterator reqPropHits = ResourceManager::instance()->mainModel()->executeQuery( q, Soprano::Query::QueryLanguageSparql );
    if ( reqPropHits.next() ) {
        int i = 1;
        QList<Query::RequestProperty> requestProperties = m_searchTerm.requestProperties();
        foreach ( const Query::RequestProperty& rp, requestProperties ) {
            result.addRequestProperty( rp.first, reqPropHits.binding( QString("reqProp%1").arg( i++ ) ) );
        }
    }
}

#include "searchthread.moc"
