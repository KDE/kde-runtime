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
#include <strigi/query.h>
#include <strigi/queryparser.h>
#include <strigi/fieldtypes.h>
#include "util.h"
#include "nie.h"

#include <Soprano/Soprano>
#include <Soprano/Vocabulary/XMLSchema>

#include <map>
#include <utility>
#include <sstream>

#include <QtCore/QThread>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtCore/QLatin1String>
#include <QtCore/QFile>


using namespace Soprano;
using namespace std;


class Strigi::Soprano::IndexReader::Private
{
public:
    ::Soprano::Model* repository;
};


Strigi::Soprano::IndexReader::IndexReader( ::Soprano::Model* model )
    : Strigi::IndexReader()
{
    d = new Private;
    d->repository = model;
}


Strigi::Soprano::IndexReader::~IndexReader()
{
    delete d;
}


// an empty parent url is perfectly valid as strigi stores a parent url for everything
void Strigi::Soprano::IndexReader::getChildren( const std::string& parent,
                                                std::map<std::string, time_t>& children )
{
    //
    // We are compatible with old Xesam data where the url was encoded as a string instead of a url,
    // thus the weird query
    //
    QString query = QString( "select distinct ?path ?mtime where { "
                             "{ ?r <http://strigi.sf.net/ontologies/0.9#parentUrl> %1 . } "
                             "UNION "
                             "{ ?r <http://strigi.sf.net/ontologies/0.9#parentUrl> %2 . } "
                             "UNION "
                             "{ ?r %3 ?parent . ?parent %7 %2 . } . "
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

//    qDebug() << "running getChildren query:" << query;

    QueryResultIterator result = d->repository->executeQuery( query, ::Soprano::Query::QueryLanguageSparql );

    while ( result.next() ) {
        Node pathNode = result.binding( "path" );
        Node mTimeNode = result.binding( "mtime" );

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


int64_t Strigi::Soprano::IndexReader::indexSize()
{
    qDebug() << "IndexReader::indexSize in thread" << QThread::currentThread();
    return d->repository->statementCount();
}


time_t Strigi::Soprano::IndexReader::mTime( const std::string& path )
{
    //
    // We are compatible with old Xesam data, thus the weird query
    //
    QString query = QString( "select ?mtime where { "
                             "{ ?r %1 %2 . } UNION { ?r %3 %4 . } "
                             "{ ?r %5 ?mtime . } UNION { ?r %6 ?mtime . } }" )
                    .arg( Node::resourceToN3( Vocabulary::Xesam::url() ),
                          Node::literalToN3( QString::fromUtf8( path.c_str() ) ),
                          Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                          Node::resourceToN3( QUrl::fromLocalFile( QFile::decodeName( path.c_str() ) ) ),
                          Node::resourceToN3( Vocabulary::Xesam::sourceModified() ),
                          Node::resourceToN3( Nepomuk::Vocabulary::NIE::lastModified() ) );

//    qDebug() << "mTime( " << path.c_str() << ") query:" << query;

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



// not implemented
int32_t Strigi::Soprano::IndexReader::countDocuments()
{
    return 0;
}


// not implemented
int32_t Strigi::Soprano::IndexReader::countWords()
{
    return -1;
}


// not implemented
std::vector<std::string> Strigi::Soprano::IndexReader::fieldNames()
{
    return std::vector<std::string>();
}


// not implemented
int32_t Strigi::Soprano::IndexReader::countHits( const Query& )
{
    return -1;
}


// not implemented
void Strigi::Soprano::IndexReader::getHits( const Strigi::Query&,
                                            const std::vector<std::string>&,
                                            const std::vector<Strigi::Variant::Type>&,
                                            std::vector<std::vector<Strigi::Variant> >&,
                                            int,
                                            int )
{
}


// not implemented
std::vector<Strigi::IndexedDocument> Strigi::Soprano::IndexReader::query( const Query&, int, int )
{
    return vector<IndexedDocument>();
}


// not implemented
std::vector<std::pair<std::string,uint32_t> > Strigi::Soprano::IndexReader::histogram( const std::string&,
                                                                                       const std::string&,
                                                                                       const std::string& )
{
    return std::vector<std::pair<std::string,uint32_t> >();
}


// not implemented
int32_t Strigi::Soprano::IndexReader::countKeywords( const std::string&,
                                                     const std::vector<std::string>& )
{
    // the clucene indexer also returns 2. I suspect this means: "not implemented" ;)
    return 2;
}


// not implemented
std::vector<std::string> Strigi::Soprano::IndexReader::keywords( const std::string&,
                                                                 const std::vector<std::string>&,
                                                                 uint32_t,
                                                                 uint32_t )
{
    return std::vector<std::string>();
}
