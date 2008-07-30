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
#include <QtNetwork/QHttp>
#include <QtNetwork/QHttpRequestHeader>

#include <Soprano/Model>
#include <Soprano/Global>
#include <Soprano/Parser>
#include <Soprano/PluginManager>
#include <Soprano/StatementIterator>

#include <KDebug>
#include <KLocale>


const unsigned int Nepomuk::GraphRetriever::s_defaultTimeoutThreshold = 8;

class Nepomuk::GraphRetriever::Private
{
public:
    Private();

    void get( const QUrl& url );

    QUrl url;
    QHttp                      m_connection;
    QHash<int, QByteArray>     m_data;
    QTimer*                    m_timer;
    unsigned int               m_idleCount;
    unsigned int               m_timeoutThreshold;
};


Nepomuk::GraphRetriever::Private::Private()
    : m_timer( 0 ),
      m_idleCount( 0 ),
      m_timeoutThreshold( s_defaultTimeoutThreshold )
{
}


void Nepomuk::GraphRetriever::Private::get( const QUrl& url )
{
    m_connection.setHost( url.host(), url.port( 80 ) );
    m_connection.setUser( url.userName(), url.password() );

    // create the header
    QHttpRequestHeader header( "GET", url.path() );
    header.setValue( "Host", url.host() );
    header.setValue( "Accept",
                     QString( "%1;q=0.2, %2" )
                     .arg( Soprano::serializationMimeType( Soprano::SerializationRdfXml ) )
                     .arg( Soprano::serializationMimeType( Soprano::SerializationTrig ) ) );
    header.setValue( "Accept-Charset", "utf-8" );

    // get a QHttp object for the request and queue it
    int requestId = m_connection.request( header );
}


Nepomuk::GraphRetriever::GraphRetriever( QObject* parent )
    : KJob( parent ),
      d( new Private() )
{
    connect( &d->m_connection, SIGNAL( requestFinished( int, bool ) ),
             this, SLOT( httpRequestFinished( int, bool ) ) );
    connect( &d->m_connection, SIGNAL( done( bool ) ),
             this, SLOT( httpDone( bool ) ) );

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


void Nepomuk::GraphRetriever::httpRequestFinished( int id, bool error )
{
    // reset idle counter every time a request is finished
    d->m_idleCount = 0;

    const QHttpResponseHeader response = d->m_connection.lastResponse();

    switch ( response.statusCode() ) {
    case 200: { // OK
        kDebug() << response.contentType();
        Soprano::RdfSerialization serialization = Soprano::mimeTypeToSerialization( response.contentType() );
        if ( serialization == Soprano::SerializationUser &&
             response.contentType().contains( "xml", Qt::CaseInsensitive ) ) {
            serialization = Soprano::SerializationRdfXml;
        }
        if ( serialization != Soprano::SerializationUser )
            d->m_data[( int )serialization] = d->m_connection.readAll();
        break;
    }

    case 301:  // Moved Permanently
    case 302:  // Found
    case 303:  // See Other
    case 307:  // Temporary Redirect
        // follow redirects
        d->get( QUrl( response.value( "Location" ) ) );
        break;

    default:
        // consider it an error
        d->m_data.clear();
    }
}


void Nepomuk::GraphRetriever::httpDone( bool error )
{
    d->m_timer->stop();

    if ( hasTimedOut() ) {
        setError( -1 );
        setErrorText( i18n( "Timeout" ) );
    }
    else if ( error ) {
        setError( ( int )d->m_connection.error() );
        setErrorText( d->m_connection.errorString() );
    }

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
