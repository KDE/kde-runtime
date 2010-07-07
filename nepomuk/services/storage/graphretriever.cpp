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
#include <QtCore/QUrl>

#include <Soprano/Model>
#include <Soprano/Global>
#include <Soprano/Parser>
#include <Soprano/PluginManager>
#include <Soprano/StatementIterator>

#include <KDebug>
#include <KLocale>
#include <kio/job.h>


class Nepomuk::GraphRetriever::Private
{
public:
    Private( Nepomuk::GraphRetriever* qq );

    void get( const QUrl& url );

    Nepomuk::GraphRetriever*   q;

    QUrl url;
    QHash<int, QByteArray>     m_data;
    unsigned int               m_idleCount;
    unsigned int               m_timeoutThreshold;
};


Nepomuk::GraphRetriever::Private::Private( Nepomuk::GraphRetriever* qq )
    : q(qq),
      m_idleCount( 0 )
{
}


void Nepomuk::GraphRetriever::Private::get( const QUrl& url )
{
    KIO::StoredTransferJob* job = KIO::storedGet( url, KIO::Reload, KIO::HideProgressInfo );
    job->addMetaData( "accept",
                        QString( "%1;q=0.2, %2" )
                        .arg( Soprano::serializationMimeType( Soprano::SerializationRdfXml ) )
                        .arg( Soprano::serializationMimeType( Soprano::SerializationTrig ) ) );
    job->addMetaData( "Charsets", "utf-8" );

    connect( job, SIGNAL(result(KJob*)),
             q, SLOT(httpRequestFinished(KJob*)));
}


Nepomuk::GraphRetriever::GraphRetriever( QObject* parent )
    : KJob( parent ),
      d( new Private(this) )
{
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


void Nepomuk::GraphRetriever::httpRequestFinished( KJob* job )
{
    KIO::StoredTransferJob* tj = static_cast<KIO::StoredTransferJob*>( job );

    // reset idle counter every time a request is finished
    d->m_idleCount = 0;

    QString mimetype = tj->mimetype();
    Soprano::RdfSerialization serialization = Soprano::mimeTypeToSerialization( mimetype );
    if ( serialization == Soprano::SerializationUser &&
         mimetype.contains( "xml", Qt::CaseInsensitive ) ) {
        serialization = Soprano::SerializationRdfXml;
    }
    if ( serialization != Soprano::SerializationUser )
        d->m_data[( int )serialization] = tj->data();

    emitResult();
}


Nepomuk::GraphRetriever* Nepomuk::GraphRetriever::retrieve( const QUrl& uri )
{
    GraphRetriever* gr = new GraphRetriever();
    gr->setUrl( uri );
    gr->start();
    return gr;
}

#include "graphretriever.moc"
