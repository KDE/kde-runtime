/*
   $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $

   This file is part of the Strigi project.
   Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "sopranoindexreader.h"
#include "tstring.h"
#include <strigi/query.h>
#include <strigi/queryparser.h>
#include <strigi/fieldtypes.h>
#include "util.h"

#include <Soprano/Soprano>
#include <Soprano/Index/IndexFilterModel>
#include <Soprano/Index/CLuceneIndex>
#include <Soprano/Vocabulary/XMLSchema>

#include <map>
#include <utility>
#include <sstream>

#include <CLucene.h>

#include <QtCore/QThread>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtCore/QLatin1String>

#include <KDebug>


using namespace Soprano;


static lucene::index::Term* createWildCardTerm( const TString& name,
                                                const string& value );
static lucene::index::Term* createTerm( const TString& name,
                                        const string& value );
static lucene::index::Term* createKeywordTerm( const TString& name,
                                               const string& value );
static lucene::search::BooleanQuery* createBooleanQuery( const Strigi::Query& query );
static lucene::search::Query* createQuery( const Strigi::Query& query );
static lucene::search::Query* createSimpleQuery( const Strigi::Query& query );
static lucene::search::Query* createSingleFieldQuery( const string& field,
                                                      const Strigi::Query& query );
static lucene::search::Query* createMultiFieldQuery( const Strigi::Query& query );


static lucene::index::Term* createWildCardTerm( const TString& name,
                                                const string& value )
{
    TString v = TString::fromUtf8( value.c_str() );
    return _CLNEW lucene::index::Term( name.data(), v.data() );
}

static lucene::index::Term* createTerm( const TString& name,
                                        const string& value )
{
    kDebug(300002) << "createTerm" << name << value.c_str();

    TString v = TString::fromUtf8( value.c_str() );

    lucene::util::StringReader sr( v.data() );
    lucene::analysis::standard::StandardAnalyzer a;
    lucene::analysis::TokenStream* ts = a.tokenStream(name.data(), &sr);
    lucene::analysis::Token* to = ts->next();
    const wchar_t *tv;
    if (to) {
        tv = to->termText();
    } else {
        tv = v.data();
    }
    lucene::index::Term* t = _CLNEW lucene::index::Term(name.data(), tv);
    if (to) {
        _CLDELETE(to);
    }
    _CLDELETE(ts);
    return t;
}

static lucene::index::Term* createKeywordTerm( const TString& name,
                                               const string& value )
{
    TString v = TString::fromUtf8( value.c_str() );
    lucene::index::Term* t = _CLNEW lucene::index::Term( name.data(), v.data() );
    return t;
}

static lucene::search::BooleanQuery* createBooleanQuery( const Strigi::Query& query )
{
    lucene::search::BooleanQuery* bq = _CLNEW lucene::search::BooleanQuery();
    bool isAnd = query.type() == Strigi::Query::And;
    const vector<Strigi::Query>& sub = query.subQueries();
    for (vector<Strigi::Query>::const_iterator i = sub.begin(); i != sub.end(); ++i) {
        lucene::search::Query* q = createQuery(*i);
        bq->add(q, true, isAnd, i->negate());
    }
    return bq;
}

static lucene::search::Query* createQuery( const Strigi::Query& query )
{
    return query.subQueries().size()
        ? createBooleanQuery(query)
        : createSimpleQuery(query);
}

static lucene::search::Query* createSimpleQuery( const Strigi::Query& query )
{
    switch (query.fields().size()) {
    case 0:  return createSingleFieldQuery("text", query);
    case 1:  return createSingleFieldQuery(query.fields()[0], query);
    default: return createMultiFieldQuery(query);
    }
}

static lucene::search::Query* createSingleFieldQuery( const string& field,
                                                      const Strigi::Query& query ) {
    kDebug(300002) << "Creating single field query: " << field.c_str();
    TString fieldname = Strigi::Soprano::Util::convertSearchField( field );
    lucene::search::Query* q;
    lucene::index::Term* t;
    const string& val = query.term().string();
    switch (query.type()) {
    case Strigi::Query::LessThan:
          t = createTerm(fieldname, val.c_str());
          q = _CLNEW lucene::search::RangeQuery(0, t, false);
          break;
    case Strigi::Query::LessThanEquals:
          t = createTerm(fieldname, query.term().string());
          q = _CLNEW lucene::search::RangeQuery(0, t, true);
          break;
    case Strigi::Query::GreaterThan:
          t = createTerm(fieldname, query.term().string());
          q = _CLNEW lucene::search::RangeQuery(t, 0, false);
          break;
    case Strigi::Query::GreaterThanEquals:
          t = createTerm(fieldname, query.term().string());
          q = _CLNEW lucene::search::RangeQuery(t, 0, true);
          break;
    case Strigi::Query::Keyword:
          t = createKeywordTerm(fieldname, query.term().string());
          q = _CLNEW lucene::search::TermQuery(t);
          break;
    default:
          if (strpbrk(val.c_str(), "*?")) {
               t = createWildCardTerm(fieldname, val);
               q = _CLNEW lucene::search::WildcardQuery(t);
          } else {
               t = createTerm(fieldname, val);
               q = _CLNEW lucene::search::TermQuery(t);
          }
    }
    _CLDECDELETE(t);
    return q;
}

static lucene::search::Query* createMultiFieldQuery( const Strigi::Query& query )
{
    lucene::search::BooleanQuery* bq = _CLNEW lucene::search::BooleanQuery();
    for (vector<string>::const_iterator i = query.fields().begin();
            i != query.fields().end(); ++i) {
        lucene::search::Query* q = createSingleFieldQuery(*i, query);
        bq->add(q, true, false, false);
    }
    return bq;
}


class Strigi::Soprano::IndexReader::Private
{
public:
    bool createDocument( const Node& res, IndexedDocument& doc ) {
        StatementIterator it = repository->listStatements( Statement( res, Node(), Node() ) );
        if ( it.lastError() ) {
            return false;
        }

        // use the resource URI as fallback file URI
        doc.uri = res.uri().toLocalFile().toUtf8().data();

        while ( it.next() ) {
            Statement s = *it;
            if ( s.object().isLiteral() ) {
                std::string fieldName = Util::fieldName( s.predicate().uri() );
                std::string value = s.object().toString().toUtf8().data();

                if (fieldName == "text") {
                    doc.fragment = value;
                }
                else if (fieldName == FieldRegister::pathFieldName) {
                    kDebug(300002) << "Setting IndexedDocument uri=" << value.c_str();
                    doc.uri = value;
                }
                else if (fieldName == FieldRegister::mimetypeFieldName) {
                    doc.mimetype = value;
                }
                else if (fieldName == FieldRegister::mtimeFieldName) {
                    doc.mtime = s.object().literal().toDateTime().toTime_t();
                }
                else if (fieldName == FieldRegister::sizeFieldName) {
                    doc.size = s.object().literal().toInt64();
                }
                else {
                    doc.properties.insert( make_pair<const string, string>( fieldName, value ) );
                }
            }
            else {
                // FIXME: For "Strigi++" we should at least go one level deeper, i.e. make an RDF query on those results that are
                // not literal statements
            }
        }

        return true;
    }

//    ::Soprano::Index::IndexFilterModel* repository;
    ::Soprano::Model* repository;
};


Strigi::Soprano::IndexReader::IndexReader( ::Soprano::Model* model )
    : Strigi::IndexReader()
{
    kDebug(300002) << "IndexReader::IndexReader in thread" << QThread::currentThread();
    d = new Private;
    d->repository = model;
}


Strigi::Soprano::IndexReader::~IndexReader()
{
    kDebug(300002) << "IndexReader::~IndexReader in thread" << QThread::currentThread();
    delete d;
}


int32_t Strigi::Soprano::IndexReader::countHits( const Query& query )
{
    kDebug(300002) << "IndexReader::countHits in thread" << QThread::currentThread();

    lucene::search::Query* q = createQuery( query );
    ::Soprano::QueryResultIterator hits = d->repository->executeQuery( TString( q->toString(), true ),
                                                                       ::Soprano::Query::QUERY_LANGUAGE_USER,
                                                                       QLatin1String( "lucene" ) );
//    Iterator< ::Soprano::Index::QueryHit> hits = d->repository->index()->search( q );
    int s = 0;
    while ( hits.next() ) {
        kDebug(300002) << "Query hit:" << hits.binding( 0 );
        ++s;
    }
    _CLDELETE(q);
    return s;
}


void Strigi::Soprano::IndexReader::getHits( const Strigi::Query& query,
                                            const std::vector<std::string>& fields,
                                            const std::vector<Strigi::Variant::Type>& types,
                                            std::vector<std::vector<Strigi::Variant> >& result,
                                            int off, int max )
{
    kDebug(300002) << "IndexReader::getHits in thread" << QThread::currentThread();
    lucene::search::Query* bq = createQuery( query );
    ::Soprano::QueryResultIterator hits = d->repository->executeQuery( TString( bq->toString(), true ),
                                                                       ::Soprano::Query::QUERY_LANGUAGE_USER,
                                                                       QLatin1String( "lucene" ) );
//    Iterator< ::Soprano::Index::QueryHit> hits = d->repository->index()->search( bq );

    int i = -1;
    while ( hits.next() ) {
        ++i;
        if ( i < off ) {
            continue;
        }
        if ( i > max ) {
            break;
        }

//        ::Soprano::Index::QueryHit hit = *hits;
        std::vector<Strigi::Variant> resultRow;
        std::vector<std::string>::const_iterator fieldIt = fields.begin();
        std::vector<Strigi::Variant::Type>::const_iterator typesIt = types.begin();
        while ( fieldIt != fields.end() ) {
            if ( typesIt == types.end() ) {
                qFatal( "(Soprano::IndexReader) Invalid types list in getHits!" );
                return;
            }

            StatementIterator it = d->repository->listStatements( Statement( hits.binding( "resource" ),
                                                                             Util::fieldUri( *fieldIt ),
                                                                             Node() ) );
            // FIXME: what if we have a field with a cardinality > 1?
            if ( it.next() ) {
                resultRow.push_back( Util::nodeToVariant( it.current().object() ) );
            }
            else {
                resultRow.push_back( Strigi::Variant() );
            }

            ++fieldIt;
            ++typesIt;
        }

        result.push_back( resultRow );
    }
    _CLDELETE(bq);
}


std::vector<Strigi::IndexedDocument> Strigi::Soprano::IndexReader::query( const Query& query, int off, int max )
{
    kDebug(300002) << "IndexReader::query in thread" << QThread::currentThread();
    vector<IndexedDocument> results;
    lucene::search::Query* bq = createQuery( query );
    ::Soprano::QueryResultIterator hits = d->repository->executeQuery( TString( bq->toString(), true ),
                                                                       ::Soprano::Query::QUERY_LANGUAGE_USER,
                                                                       "lucene" );
//    Iterator< ::Soprano::Index::QueryHit> hits = d->repository->index()->search( bq );

    int i = -1;
    while ( hits.next() ) {
        ++i;
        if ( i < off ) {
            continue;
        }
        if ( i > max ) {
            break;
        }

        IndexedDocument result;
//        ::Soprano::Index::QueryHit hit = *hits;
        result.score = hits.binding( 1 ).literal().toDouble();
        if ( d->createDocument( hits.binding( 0 ), result ) ) {
            results.push_back( result );
        }
        else {
            kDebug(300002) << "Failed to create indexed document for resource " << hits.binding( 0 ) << ": " << d->repository->lastError();
        }
    }
    _CLDELETE(bq);
    return results;
}


// an empty parent url is perfectly valid as strigi stores a parent url for everything
void Strigi::Soprano::IndexReader::getChildren( const std::string& parent,
                                                std::map<std::string, time_t>& children )
{
    kDebug(300002) << "IndexReader::getChildren in thread" << QThread::currentThread();
    QString query = QString( "select distinct ?path ?mtime where { ?r <%1> \"%2\"^^<%3> . ?r <%4> ?mtime . ?r <%5> ?path . }")
                    .arg( Util::fieldUri( FieldRegister::parentLocationFieldName ).toString() )
                    .arg( QString::fromUtf8( parent.c_str() ) )
                    .arg( Vocabulary::XMLSchema::string().toString() )
                    .arg( Util::fieldUri( FieldRegister::mtimeFieldName ).toString() )
                    .arg( Util::fieldUri( FieldRegister::pathFieldName ).toString() );

    kDebug(300002) << "running getChildren query:" << query;

    QueryResultIterator result = d->repository->executeQuery( query, ::Soprano::Query::QUERY_LANGUAGE_SPARQL );

    while ( result.next() ) {
        Node pathNode = result.binding( "path" );
        Node mTimeNode = result.binding( "mtime" );
        kDebug(300002) << "file in index: " << pathNode.toString() << "mtime:" << mTimeNode.literal().toDateTime() << "(" << mTimeNode.literal().toDateTime().toTime_t() << ")";
        children[std::string( pathNode.toString().toUtf8().data() )] = mTimeNode.literal().toDateTime().toTime_t();
    }
}


int32_t Strigi::Soprano::IndexReader::countDocuments()
{
    kDebug(300002) << "IndexReader::countDocuments in thread" << QThread::currentThread();
    // FIXME: the only solution I see ATM is: select distinct ?r where { ?r ?p ?o }
    return 0;
}


int32_t Strigi::Soprano::IndexReader::countWords()
{
    kDebug(300002) << "IndexReader::countWords in thread" << QThread::currentThread();
    // FIXME: what to do here? use the index? Count the predicates?
    return -1;
}


int64_t Strigi::Soprano::IndexReader::indexSize()
{
    kDebug(300002) << "IndexReader::indexSize in thread" << QThread::currentThread();
    return d->repository->statementCount();
}


time_t Strigi::Soprano::IndexReader::mTime( const std::string& uri )
{
    kDebug(300002) << "IndexReader::mTime in thread" << QThread::currentThread();
    QString query = QString( "select ?mtime where { ?r <%2> \"%3\"^^<%4> . ?r <%1> ?mtime . }" )
                    .arg( Util::fieldUri( FieldRegister::mtimeFieldName ).toString() )
                    .arg( Util::fieldUri( FieldRegister::pathFieldName ).toString() )
                    .arg( QString::fromUtf8( uri.c_str() ) )
                    .arg( Vocabulary::XMLSchema::string().toString() );

    kDebug(300002) << "mTime( " << uri.c_str() << ") query:" << query;

    QueryResultIterator it = d->repository->executeQuery( query, ::Soprano::Query::QUERY_LANGUAGE_SPARQL );

    time_t mtime = 0;
    if ( it.next() ) {
        mtime = it.binding( "mtime" ).literal().toDateTime().toTime_t();
    }
    return mtime;
}


std::vector<std::string> Strigi::Soprano::IndexReader::fieldNames()
{
    kDebug(300002) << "IndexReader::fieldNames in thread" << QThread::currentThread();
    // This is a weird method
    // Our list of field names (the predicates) is probably awefully long.

    std::vector<std::string> fields;
    QueryResultIterator it = d->repository->executeQuery( "select distinct ?p where { ?r ?p ?o . }", ::Soprano::Query::QUERY_LANGUAGE_SPARQL );
    while ( it.next() ) {
        fields.push_back( Util::fieldName( it.binding("p").uri() ) );
    }
    return fields;
}


std::vector<std::pair<std::string,uint32_t> > Strigi::Soprano::IndexReader::histogram( const std::string& query,
                                                                                       const std::string& fieldname,
                                                                                       const std::string& labeltype )
{
    // FIXME: what is meant by fieldname and labeltype?
    kDebug(300002) << "IndexReader::histogram in thread" << QThread::currentThread();
    // IMPLEMENTME? Seems not like a very important method though.
    return std::vector<std::pair<std::string,uint32_t> >();
}


int32_t Strigi::Soprano::IndexReader::countKeywords( const std::string& keywordprefix,
                                                     const std::vector<std::string>& fieldnames)
{
    kDebug(300002) << "IndexReader::countKeywords in thread" << QThread::currentThread();
    // the clucene indexer also returns 2. I suspect this means: "not implemented" ;)
    return 2;
}


std::vector<std::string> Strigi::Soprano::IndexReader::keywords( const std::string& keywordmatch,
                                                                 const std::vector<std::string>& fieldnames,
                                                                 uint32_t max, uint32_t offset )
{
    kDebug(300002) << "IndexReader::keywords in thread" << QThread::currentThread();
    // IMPLEMENTME? Seems like a rarely used method...
    return std::vector<std::string>();
}
