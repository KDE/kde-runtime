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

#include "searchrunnable.h"
#include "folder.h"

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Resource>

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
#include <Nepomuk/Vocabulary/NFO>

#include <KDebug>
#include <KDateTime>
#include <KRandom>

#include <QtCore/QTime>
#include <QtCore/QRegExp>
#include <QtCore/QLatin1String>
#include <QtCore/QStringList>



using namespace Soprano;

Nepomuk::Query::SearchRunnable::SearchRunnable( Folder* folder )
    : QRunnable(),
      m_folder( folder )
{
}


Nepomuk::Query::SearchRunnable::~SearchRunnable()
{
}


void Nepomuk::Query::SearchRunnable::cancel()
{
    // "detach" us from the folder which will most likely be deleted now
    QMutexLocker lock( &m_folderMutex );
    m_folder = 0;
}


void Nepomuk::Query::SearchRunnable::run()
{
    QMutexLocker lock( &m_folderMutex );
    if( !m_folder )
        return;
    kDebug() << m_folder->query() << m_folder->sparqlQuery();
    const QString sparql = m_folder->sparqlQuery();
    lock.unlock();

#ifndef NDEBUG
    QTime time;
    time.start();
#endif

    Soprano::QueryResultIterator hits = ResourceManager::instance()->mainModel()->executeQuery( sparql, Soprano::Query::QueryLanguageSparql );
    while ( m_folder &&
            hits.next() ) {
        Result result = extractResult( hits );

        kDebug() << "Found result:" << result.resource().resourceUri() << result.score();

        lock.relock();
        if( m_folder ) {
            m_folder->addResults( QList<Result>() << result );
        }
        lock.unlock();
    }

#ifndef NDEBUG
    kDebug() << time.elapsed();
#endif

    lock.relock();
    if( m_folder ) {
        m_folder->listingFinished();
    }
}


Nepomuk::Query::Result Nepomuk::Query::SearchRunnable::extractResult( const Soprano::QueryResultIterator& it ) const
{
    Result result( Resource::fromResourceUri( it[0].uri() ) );

    // make sure we do not store values twice
    QStringList names = it.bindingNames();
    names.removeAll( QLatin1String( "r" ) );

    m_folderMutex.lock();
    if( m_folder ) {
        RequestPropertyMap requestProperties = m_folder->requestPropertyMap();
        for ( RequestPropertyMap::const_iterator rpIt = requestProperties.constBegin();
             rpIt != requestProperties.constEnd(); ++rpIt ) {
            result.addRequestProperty( rpIt.value(), it.binding( rpIt.key() ) );
            names.removeAll( rpIt.key() );
        }
    }
    m_folderMutex.unlock();

    static const char* s_scoreVarName = "_n_f_t_m_s_";
    static const char* s_excerptVarName = "_n_f_t_m_ex_";

    Soprano::BindingSet set;
    int score = 0;
    Q_FOREACH( const QString& var, names ) {
        if ( var == QLatin1String( s_scoreVarName ) )
            score = it[var].literal().toInt();
        else if ( var == QLatin1String( s_excerptVarName ) )
            result.setExcerpt( it[var].toString() );
        else
            set.insert( var, it[var] );
    }

    result.setAdditionalBindings( set );
    result.setScore( ( double )score );

    // score will be set above
    return result;
}
