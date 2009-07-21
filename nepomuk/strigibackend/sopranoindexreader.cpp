/*
   Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

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
#include "nie.h"

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
#include <QtCore/QFile>


using namespace Soprano;
using namespace std;


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

#if 0
static QString luceneQueryEscape( const QString& s )
{
    /* Chars to escape: + - && || ! ( ) { } [ ] ^ " ~  : \ */

    static QRegExp rx( "([\\-" + QRegExp::escape( "+&|!(){}[]^\"~:\\" ) + "])" );
    QString es( s );
    es.replace( rx, "\\\\1" );
    return es;
}
#endif

static lucene::index::Term* createWildCardTerm( const TString& name,
                                                const string& value )
{
    TString v = TString::fromUtf8( value.c_str() );
    return _CLNEW lucene::index::Term( name.data(), v.data() );
}

static lucene::index::Term* createTerm( const TString& name,
                                        const string& value )
{
    qDebug() << "createTerm" << name << value.c_str();

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
    qDebug() << "Creating single field query: " << field.c_str();
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
                    qDebug() << "Setting IndexedDocument uri=" << value.c_str();
                    doc.uri = value;
                }
                else if (fieldName == FieldRegister::mimetypeFieldName) {
                    doc.mimetype = value;
                }
                else if (fieldName == FieldRegister::mtimeFieldName) {
                    // Sadly in Xesam sourceModified is not typed as DateTime but defaults to an int :( We try to be compatible
                    if ( s.object().literal().isDateTime() ) {
                        doc.mtime = s.object().literal().toDateTime().toTime_t();
                    }
                    else {
                        doc.mtime = s.object().literal().toUnsignedInt();
                    }
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

    ::Soprano::Model* repository;
};


Strigi::Soprano::IndexReader::IndexReader( ::Soprano::Model* model )
    : Strigi::IndexReader()
{
    qDebug() << "IndexReader::IndexReader in thread" << QThread::currentThread();
    d = new Private;
    d->repository = model;
}


Strigi::Soprano::IndexReader::~IndexReader()
{
    qDebug() << "IndexReader::~IndexReader in thread" << QThread::currentThread();
    delete d;
}


int32_t Strigi::Soprano::IndexReader::countHits( const Query& query )
{
    qDebug() << "IndexReader::countHits in thread" << QThread::currentThread();

    lucene::search::Query* q = createQuery( query );
    ::Soprano::QueryResultIterator hits = d->repository->executeQuery( TString( q->toString(), true ),
                                                                       ::Soprano::Query::QueryLanguageUser,
                                                                       QLatin1String( "lucene" ) );
    int s = 0;
    while ( hits.next() ) {
        qDebug() << "Query hit:" << hits.binding( 0 );
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
    qDebug() << "IndexReader::getHits in thread" << QThread::currentThread();
    lucene::search::Query* bq = createQuery( query );
    ::Soprano::QueryResultIterator hits = d->repository->executeQuery( TString( bq->toString(), true ),
                                                                       ::Soprano::Query::QueryLanguageUser,
                                                                       QLatin1String( "lucene" ) );

    int i = -1;
    while ( hits.next() ) {
        ++i;
        if ( i < off ) {
            continue;
        }
        if ( i > max ) {
            break;
        }

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
    qDebug() << "IndexReader::query in thread" << QThread::currentThread();
    vector<IndexedDocument> results;
    lucene::search::Query* bq = createQuery( query );
    ::Soprano::QueryResultIterator hits = d->repository->executeQuery( TString( bq->toString(), true ),
                                                                       ::Soprano::Query::QueryLanguageUser,
                                                                       QLatin1String( "lucene" ) );

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
        result.score = hits.binding( 1 ).literal().toDouble();
        if ( d->createDocument( hits.binding( 0 ), result ) ) {
            results.push_back( result );
        }
        else {
            qDebug() << "Failed to create indexed document for resource " << hits.binding( 0 ) << ": " << d->repository->lastError();
        }
    }
    _CLDELETE(bq);
    return results;
}


// an empty parent url is perfectly valid as strigi stores a parent url for everything
void Strigi::Soprano::IndexReader::getChildren( const std::string& parent,
                                                std::map<std::string, time_t>& children )
{
    //
    // We are compatible with old Xesam data, thus the weird query
    //
    QString query = QString( "select distinct ?path ?mtime where { "
                             "{ ?r <http://strigi.sf.net/ontologies/0.9#parentUrl> %1 . } "
                             "UNION "
                             "{ ?r <http://strigi.sf.net/ontologies/0.9#parentUrl> %2 . } "
                             "UNION "
                             "{ ?r %3 %2 . } . "
                             "{ ?r %4 ?mtime . } UNION { ?r %5 ?mtime . } "
                             "{ ?r %6 ?path . } UNION { ?r %7 ?path . } "
                             "}")
                    .arg( Node::literalToN3( QString::fromUtf8( parent.c_str() ) ),
                          Node::resourceToN3( QUrl::fromLocalFile( QFile::decodeName( parent.c_str() ) ) ),
                          Node::resourceToN3( Nepomuk::Vocabulary::NIE::isPartOf() ),
                          Node::resourceToN3( Vocabulary::Xesam::sourceModified() ),
                          Node::resourceToN3( Nepomuk::Vocabulary::NIE::lastModified() ),
                          Node::resourceToN3( Vocabulary::Xesam::url() ),
                          Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ) );

    qDebug() << "running getChildren query:" << query;

    QueryResultIterator result = d->repository->executeQuery( query, ::Soprano::Query::QueryLanguageSparql );

    while ( result.next() ) {
        Node pathNode = result.binding( "path" );
        Node mTimeNode = result.binding( "mtime" );
        qDebug() << "file in index: " << pathNode.toString() << "mtime:" << mTimeNode.literal().toDateTime() << "(" << mTimeNode.literal().toDateTime().toTime_t() << ")";

        // be backwards compatible in case there are paths left encoded as literals
        std::string path;
        if ( pathNode.isLiteral() )
            path = pathNode.toString().toUtf8().data();
        else
            path = QFile::encodeName( pathNode.uri().toLocalFile() ).data();

        // Sadly in Xesam sourceModified is not typed as DateTime but defaults to an int :( We try to be compatible
        if ( mTimeNode.literal().isDateTime() ) {
            children[path] = mTimeNode.literal().toDateTime().toTime_t();
        }
        else {
            children[path] = mTimeNode.literal().toUnsignedInt();
        }
    }
}


int32_t Strigi::Soprano::IndexReader::countDocuments()
{
    qDebug() << "IndexReader::countDocuments in thread" << QThread::currentThread();
    // FIXME: the only solution I see ATM is: select distinct ?r where { ?r ?p ?o }
    return 0;
}


int32_t Strigi::Soprano::IndexReader::countWords()
{
    qDebug() << "IndexReader::countWords in thread" << QThread::currentThread();
    // FIXME: what to do here? use the index? Count the predicates?
    return -1;
}


int64_t Strigi::Soprano::IndexReader::indexSize()
{
    qDebug() << "IndexReader::indexSize in thread" << QThread::currentThread();
    return d->repository->statementCount();
}


time_t Strigi::Soprano::IndexReader::mTime( const std::string& uri )
{
    //
    // We are compatible with old Xesam data, thus the weird query
    //
    QString query = QString( "select ?mtime where { "
                             "{ ?r %1 %2 . } UNION { ?r %3 %2 . } "
                             "{ ?r %4 ?mtime . } UNION { ?r %5 ?mtime . } }" )
                    .arg( Node::resourceToN3( Vocabulary::Xesam::url() ),
                          Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                          Node::literalToN3( QString::fromUtf8( uri.c_str() ) ),
                          Node::resourceToN3( Vocabulary::Xesam::sourceModified() ),
                          Node::resourceToN3( Nepomuk::Vocabulary::NIE::lastModified() ) );

    qDebug() << "mTime( " << uri.c_str() << ") query:" << query;

    QueryResultIterator it = d->repository->executeQuery( query, ::Soprano::Query::QueryLanguageSparql );

    time_t mtime = 0;
    if ( it.next() ) {
        ::Soprano::LiteralValue val = it.binding( "mtime" ).literal();

        // Sadly in Xesam sourceModified is not typed as DateTime but defaults to an int :( We try to be compatible
        if ( val.isDateTime() ) {
            mtime = val.toDateTime().toTime_t();
        }
        else {
            mtime = val.toUnsignedInt();
        }
    }
    return mtime;
}


std::vector<std::string> Strigi::Soprano::IndexReader::fieldNames()
{
    qDebug() << "IndexReader::fieldNames in thread" << QThread::currentThread();
    // This is a weird method
    // Our list of field names (the predicates) is probably awefully long.

    std::vector<std::string> fields;
    QueryResultIterator it = d->repository->executeQuery( "select distinct ?p where { ?r ?p ?o . }", ::Soprano::Query::QueryLanguageSparql );
    while ( it.next() ) {
        fields.push_back( Util::fieldName( it.binding("p").uri() ) );
    }
    return fields;
}


std::vector<std::pair<std::string,uint32_t> > Strigi::Soprano::IndexReader::histogram( const std::string& query,
                                                                                       const std::string& fieldname,
                                                                                       const std::string& labeltype )
{
    Q_UNUSED(query);
    Q_UNUSED(fieldname);
    Q_UNUSED(labeltype);

    // FIXME: what is meant by fieldname and labeltype?
    qDebug() << "IndexReader::histogram in thread" << QThread::currentThread();
    // IMPLEMENTME? Seems not like a very important method though.
    return std::vector<std::pair<std::string,uint32_t> >();
}


int32_t Strigi::Soprano::IndexReader::countKeywords( const std::string& keywordprefix,
                                                     const std::vector<std::string>& fieldnames)
{
    Q_UNUSED(keywordprefix);
    Q_UNUSED(fieldnames);

    qDebug() << "IndexReader::countKeywords in thread" << QThread::currentThread();
    // the clucene indexer also returns 2. I suspect this means: "not implemented" ;)
    return 2;
}


std::vector<std::string> Strigi::Soprano::IndexReader::keywords( const std::string& keywordmatch,
                                                                 const std::vector<std::string>& fieldnames,
                                                                 uint32_t max, uint32_t offset )
{
    Q_UNUSED(keywordmatch);
    Q_UNUSED(fieldnames);
    Q_UNUSED(max);
    Q_UNUSED(offset);

    qDebug() << "IndexReader::keywords in thread" << QThread::currentThread();
    // IMPLEMENTME? Seems like a rarely used method...
    return std::vector<std::string>();
}
