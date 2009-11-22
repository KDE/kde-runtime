/*
  This file is part of the Nepomuk KDE project.
  Copyright (C) 2007-2009 Sebastian Trueg <trueg@kde.org>

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
#include <QLatin1String>
#include <QStringList>



using namespace Soprano;

Nepomuk::Query::SearchThread::SearchThread( QObject* parent )
    : QThread( parent )
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
    m_cutOffScore = cutOffScore;

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

    sparqlQuery( m_sparqlQuery, 1.0, true );

    kDebug() << time.elapsed();
}


QHash<QUrl, Nepomuk::Query::Result> Nepomuk::Query::SearchThread::sparqlQuery( const QString& query, double baseScore, bool reportResults )
{
    kDebug() << query;

    QHash<QUrl, Result> results;

    Soprano::QueryResultIterator hits = ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    while ( hits.next() ) {
        if ( m_canceled ) break;

        Result result = extractResult( hits );
        result.setScore( baseScore );

        kDebug() << "Found result:" << result.resource().resourceUri();

        // these are actual direct hits and we can report them right away
        if ( reportResults ) {
            emit newResult( result );
        }

        results.insert( result.resource().resourceUri(), result );
    }

    return results;
}


Nepomuk::Query::Result Nepomuk::Query::SearchThread::extractResult( const Soprano::QueryResultIterator& it ) const
{
    kDebug() << it.binding( 0 ).uri();
    Result result( it.binding( 0 ).uri() );

    for ( RequestPropertyMap::const_iterator rpIt = m_requestProperties.constBegin();
          rpIt != m_requestProperties.constEnd(); ++rpIt ) {
        result.addRequestProperty( rpIt.value(), it.binding( rpIt.key() ) );
    }

    // score will be set above
    return result;
}

#include "searchthread.moc"
