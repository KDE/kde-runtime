/*
  This file is part of the Nepomuk KDE project.
  Copyright (C) 2007-2010 Sebastian Trueg <trueg@kde.org>

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
#include "nfo.h"

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
#include <Soprano/Vocabulary/Xesam>

#include <KDebug>
#include <KDateTime>
#include <KRandom>

#include <QtCore/QTime>
#include <QtCore/QRegExp>
#include <QtCore/QLatin1String>
#include <QtCore/QStringList>



using namespace Soprano;

Nepomuk::Query::SearchThread::SearchThread( QObject* parent )
    : QThread( parent ),
      m_resultCnt( 0 )
{
}


Nepomuk::Query::SearchThread::~SearchThread()
{
}


void Nepomuk::Query::SearchThread::query( const QString& query, const RequestPropertyMap& requestProps, double cutOffScore )
{
    kDebug() << query << cutOffScore;

    if( isRunning() ) {
        cancel();
    }

    m_canceled = false;
    m_sparqlQuery = query;
    m_requestProperties = requestProps;
    m_cutOffScore = qMin( 1.0, qMax( cutOffScore, 0.0 ) );

    start();
}


void Nepomuk::Query::SearchThread::cancel()
{
    m_canceled = true;
    wait();
}


void Nepomuk::Query::SearchThread::run()
{
    QTime time;
    time.start();

    m_resultCnt = 0;

    //
    // To speed up the user experience and since in most cases users would only
    // look at the first few results anyway, we run the query twice: once with a
    // limit of 10 and once without a limit. To check if the query already has a
    // limit or an offset we do not do any fancy things. We simply check if the
    // query ends with a closing bracket which always suceeds for queries
    // created via the Nepomuk query API.
    //
    if ( m_sparqlQuery.endsWith( QLatin1String( "}" ) ) ) {
        sparqlQuery( m_sparqlQuery + QLatin1String( " LIMIT 10" ), 1.0 );
        if ( !m_canceled && m_resultCnt >= 10 )
            sparqlQuery( m_sparqlQuery + QLatin1String( " OFFSET 10" ), 1.0 );
    }
    else {
        sparqlQuery( m_sparqlQuery, 1.0 );
    }

    kDebug() << time.elapsed();
}


void Nepomuk::Query::SearchThread::sparqlQuery( const QString& query, double baseScore )
{
    kDebug() << query;

    Soprano::QueryResultIterator hits = ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    while ( hits.next() ) {
        if ( m_canceled ) break;

        ++m_resultCnt;

        Resource res( hits[0].uri() );
        if ( !res.hasType( Soprano::Vocabulary::RDFS::Class() ) &&
             !res.hasType( Soprano::Vocabulary::RDF::Property() ) &&
             !res.hasType( Soprano::Vocabulary::NRL::Graph() ) ) {
            Result result = extractResult( hits );
            result.setScore( result.score() * baseScore );

            kDebug() << "Found result:" << result.resource().resourceUri() << result.score();

            emit newResult( result );
        }
    }
}


Nepomuk::Query::Result Nepomuk::Query::SearchThread::extractResult( const Soprano::QueryResultIterator& it ) const
{
    kDebug() << it.binding( 0 ).uri();
    Result result( it.binding( 0 ).uri() );

    // make sure we do not store values twice
    QStringList names = it.bindingNames();
    names.removeAll( QLatin1String( "r" ) );

    for ( RequestPropertyMap::const_iterator rpIt = m_requestProperties.constBegin();
          rpIt != m_requestProperties.constEnd(); ++rpIt ) {
        result.addRequestProperty( rpIt.value(), it.binding( rpIt.key() ) );
        names.removeAll( rpIt.key() );
    }

    static const char* s_scoreVarName = "_n_f_t_m_s_";

    Soprano::BindingSet set;
    int score = 0;
    Q_FOREACH( const QString& var, names ) {
        if ( var == QLatin1String( s_scoreVarName ) )
            score = it[var].literal().toInt();
        else
            set.insert( var, it[var] );
    }

    result.setAdditionalBindings( set );
    result.setScore( ( double )score );

    // score will be set above
    return result;
}

#include "searchthread.moc"
