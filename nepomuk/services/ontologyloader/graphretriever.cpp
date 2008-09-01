/*  This file is part of the KDE semantic clipboard
    Copyright (C) 2008 Tobias Wolf <twolf@access.unizh.ch>
    Copyright (C) 2008 Sebastian Trueg <trueg@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "graphretriever.h"

#include <QtCore/QByteArray>
#include <QtCore/QEventLoop>
#include <QtCore/QHash>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtCore/QUrl>

#include <Soprano/Model>
#include <Soprano/Global>
#include <Soprano/Parser>
#include <Soprano/PluginManager>
#include <Soprano/StatementIterator>

#include <KDebug>
#include <KLocale>
#include <kio/job.h>


const unsigned int Nepomuk::GraphRetriever::s_defaultTimeoutThreshold = 8;

class Nepomuk::GraphRetriever::Private
{
public:
    Private(Nepomuk::GraphRetriever *qq);

    void get( const QUrl& url );

    Nepomuk::GraphRetriever*   q;

    QUrl url;
    QHash<int, QByteArray>     m_data;
    QByteArray                 m_currentData;
    KIO::TransferJob*          m_job;
    QTimer*                    m_timer;
    unsigned int               m_idleCount;
    unsigned int               m_timeoutThreshold;
};


Nepomuk::GraphRetriever::Private::Private(Nepomuk::GraphRetriever *qq)
    : q(qq),
      m_job( 0 ),
      m_timer( 0 ),
      m_idleCount( 0 ),
      m_timeoutThreshold( s_defaultTimeoutThreshold )
{
}


void Nepomuk::GraphRetriever::Private::get( const QUrl& url )
{
    m_job = KIO::get( url );
    m_job->addMetaData( "accept",
                        QString( "%1;q=0.2, %2" )
                        .arg( Soprano::serializationMimeType( Soprano::SerializationRdfXml ) )
                        .arg( Soprano::serializationMimeType( Soprano::SerializationTrig ) ) );
    m_job->addMetaData( "Charsets", "utf-8" );

    connect( m_job, SIGNAL(data(KIO::Job*,QByteArray)),
             q, SLOT(httpData(KIO::Job*,QByteArray)));
    connect( m_job, SIGNAL(finished(KJob*)),
             q, SLOT(httpRequestFinished()));
    m_currentData.clear();
}


Nepomuk::GraphRetriever::GraphRetriever( QObject* parent )
    : KJob( parent ),
      d( new Private(this) )
{
    d->m_timer = new QTimer( this );
    connect( d->m_timer, SIGNAL( timeout() ), this, SLOT( timeout() ) );
}


Nepomuk::GraphRetriever::~GraphRetriever()
{
    delete d;
}


void Nepomuk::GraphRetriever::setUrl( const QUrl& url )
{
    d->url = url;
}


QUrl Nepomuk::GraphRetriever::url() const
{
    return d->url;
}


void Nepomuk::GraphRetriever::start()
{
    d->get( d->url );
}


Soprano::Model* Nepomuk::GraphRetriever::model() const
{
    Soprano::Model* result = Soprano::createModel();
    Soprano::StatementIterator it = statements();
    while ( it.next() ) {
        result->addStatement( *it );
    }
    return result;
}


Soprano::StatementIterator Nepomuk::GraphRetriever::statements() const
{
    QByteArray data;
    Soprano::RdfSerialization serialization = Soprano::SerializationRdfXml;
    if ( d->m_data.contains( ( int )Soprano::SerializationTrig ) ) {
        serialization = Soprano::SerializationTrig;
        data = d->m_data[( int )Soprano::SerializationTrig];
    }
    else {
        serialization = Soprano::SerializationRdfXml;
        data = d->m_data[( int )Soprano::SerializationRdfXml];
    }

    QTextStream stream( data );
    if ( const Soprano::Parser* parser =
         Soprano::PluginManager::instance()->discoverParserForSerialization( serialization ) ) {
        return parser->parseStream( stream, d->url, serialization );
    }
    else {
        return Soprano::StatementIterator();
    }
}

void Nepomuk::GraphRetriever::httpData(KIO::Job *, const QByteArray &data)
{
    d->m_currentData += data;
}

void Nepomuk::GraphRetriever::httpRequestFinished()
{
    // reset idle counter every time a request is finished
    d->m_idleCount = 0;

    QString mimetype = d->m_job->mimetype();
    Soprano::RdfSerialization serialization = Soprano::mimeTypeToSerialization( mimetype );
    if ( serialization == Soprano::SerializationUser &&
         mimetype.contains( "xml", Qt::CaseInsensitive ) ) {
        serialization = Soprano::SerializationRdfXml;
    }
    if ( serialization != Soprano::SerializationUser )
        d->m_data[( int )serialization] = d->m_currentData;

    d->m_currentData.clear();
    d->m_job = 0;

    emitResult();
}


void Nepomuk::GraphRetriever::timeout()
{
    // abort if no request was finished in over 10 timeouts
    if ( ++d->m_idleCount >= d->m_timeoutThreshold ) {
        d->m_timer->stop();
    }
}


bool Nepomuk::GraphRetriever::hasTimedOut() const
{
    return ( d->m_idleCount >= d->m_timeoutThreshold );
}


void Nepomuk::GraphRetriever::setTimeoutThreshold( unsigned int timeoutThreshold )
{
    if ( timeoutThreshold ) {
        d->m_timeoutThreshold = timeoutThreshold;
    } else {
        d->m_timeoutThreshold = s_defaultTimeoutThreshold;
    }
}


Nepomuk::GraphRetriever* Nepomuk::GraphRetriever::retrieve( const QUrl& uri )
{
    GraphRetriever* gr = new GraphRetriever();
    gr->setUrl( uri );
    gr->start();
    return gr;
}

#include "graphretriever.moc"
