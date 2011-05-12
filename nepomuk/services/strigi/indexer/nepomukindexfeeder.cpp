/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-11 Vishesh Handa <handa.vish@gmail.com>
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

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


#include "nepomukindexfeeder.h"
#include "../util.h"
#include "datamanagement.h"

#include <QtCore/QDateTime>

#include <Soprano/Statement>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/NAO>
#include <Nepomuk/Vocabulary/NFO>

#include <KDebug>
#include <KJob>

using namespace Soprano::Vocabulary;
using namespace Nepomuk::Vocabulary;

Nepomuk::IndexFeeder::IndexFeeder( QObject* parent )
    : QObject( parent )
{
}


Nepomuk::IndexFeeder::~IndexFeeder()
{
}


void Nepomuk::IndexFeeder::begin( const QUrl & url )
{
    //kDebug() << "BEGINNING";
    Request req;
    req.uri = url;
    req.durationPassed = false;
    
    m_stack.push( req );
}


void Nepomuk::IndexFeeder::addStatement(const Soprano::Statement& st)
{
    Q_ASSERT( !m_stack.isEmpty() );
    Request & req = m_stack.top();
    
    if( st.subject().isResource() )
        req.uri = st.subject().uri();

    // Make sure nfo:duration has a max cardinality of 1
    if( req.uri == st.subject().uri() && st.predicate().uri() == NFO::duration() ) {
        if( req.durationPassed )
            return;
        req.durationPassed = true;
    }
    
    req.graph.addStatement( st );

    // Debug info
    qDebug() << st.subject().toN3() << " " << st.predicate().toN3() << " " << st.object().toN3();
}


void Nepomuk::IndexFeeder::addStatement(const Soprano::Node& subject, const Soprano::Node& predicate, const Soprano::Node& object)
{
    addStatement( Soprano::Statement( subject, predicate, object, Soprano::Node() ) );
}


void Nepomuk::IndexFeeder::end()
{
    if( m_stack.isEmpty() )
        return;
    //kDebug() << "ENDING";

    Request req = m_stack.pop();
    handleRequest( req );
}

void Nepomuk::IndexFeeder::handleRequest( Request& request )
{   
    QHash<QUrl, QVariant> additionalMetadata;
    additionalMetadata.insert( RDF::type(), NRL::DiscardableInstanceBase() );

    //
    // Add all the resources as sub-resources of the main resource
    //
    foreach( const SimpleResource & res, request.graph.toList() ) {
        if( res.uri() != request.uri ) {
            request.graph.addStatement( request.uri, NAO::hasSubResource(), res.uri() );
        }
    }

    
    KJob* job = Nepomuk::storeResources( request.graph, additionalMetadata );
    job->exec();
    if( job->error() ) {
        kDebug() << job->errorString();
    }

    m_lastRequestUri = request.uri;
}

QUrl Nepomuk::IndexFeeder::lastRequestUri() const
{
    return m_lastRequestUri;
}
