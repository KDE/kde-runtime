/*
  Copyright (C) 2007-2009 Sebastian Trueg <trueg@kde.org>

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

#include "util.h"
#include "datamanagement.h"

#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QUuid>
#include <QtCore/QDebug>

#include <Soprano/Model>
#include <Soprano/Statement>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/XMLSchema>

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Vocabulary/NIE>

#include <KJob>
#include <KDebug>
#include <KGlobal>
#include <KComponentData>

#define STRIGI_NS "http://www.strigi.org/data#"

QUrl Strigi::Util::fieldUri( const std::string& s )
{
    QString qKey = QString::fromUtf8( s.c_str() );
    QUrl url;

    // very naive test for proper URI
    if ( qKey.contains( ":/" ) ) {
        url = qKey;
    }
    else {
        url = STRIGI_NS + qKey;
    }

    // just to be sure
    if ( url.isRelative() ) {
        url.setScheme( "http" );
    }

    return url;
}


QUrl Strigi::Util::fileUrl( const std::string& filename )
{
    QUrl url = QUrl::fromLocalFile( QFileInfo( QString::fromUtf8( filename.c_str() ) ).absoluteFilePath() );
    url.setScheme( "file" );
    return url;
}


std::string Strigi::Util::fieldName( const QUrl& uri )
{
    QString s = uri.toString();
    if ( s.startsWith( STRIGI_NS ) ) {
        s = s.mid( strlen( STRIGI_NS ) );
    }
    return s.toUtf8().data();
}


QUrl Strigi::Util::uniqueUri( const QString& ns, Soprano::Model* model )
{
    QUrl uri;
    do {
        QString uid = QUuid::createUuid().toString();
        uri = ( ns + uid.mid( 1, uid.length()-2 ) );
    } while ( model->containsAnyStatement( Soprano::Statement( uri, Soprano::Node(), Soprano::Node() ) ) );
    return uri;
}

QUrl Strigi::Ontology::indexGraphFor()
{
    return QUrl::fromEncoded( "http://www.strigi.org/fields#indexGraphFor", QUrl::StrictMode );
}

bool Nepomuk::clearIndexedDataForUrl( const KUrl& url )
{
    if ( url.isEmpty() )
        return false;

    //
    // New way of storing Strigi Data
    // The Datamanagement API will automatically find the resource corresponding to that url
    //
    KComponentData component = KGlobal::mainComponent();
    if( component.componentName() != QLatin1String("nepomukindexer") ) {
        component = KComponentData( QByteArray("nepomukindexer"),
                                    QByteArray(), KComponentData::SkipMainComponentRegistration );
    }
    KJob* job = Nepomuk::removeDataByApplication( QList<QUrl>() << url, RemoveSubResoures, component );
    job->exec();
    
    if( job->error() ) {
        kError() << job->errorString();
    }

    //
    // Old way
    //
    QString query = QString::fromLatin1( "select ?g where { "
                                         "{ "
                                         "?r %2 %1 . "
                                         "?g %3 ?r . } "
                                         "UNION "
                                         "{ ?g %3 %1 . }"
                                         "}")
                    .arg( Soprano::Node::resourceToN3( url ),
                          Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                          Soprano::Node::resourceToN3( Strigi::Ontology::indexGraphFor() ) );

    Soprano::QueryResultIterator result = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    while ( result.next() ) {
        // delete the indexed data (The Soprano::NRLModel in the storage service will take care of
        // the metadata graph)
        Nepomuk::ResourceManager::instance()->mainModel()->removeContext( result.binding( "g" ) );
    }

    return true;
}


// static
bool Nepomuk::clearIndexedDataForResourceUri( const KUrl& res )
{
    if ( res.isEmpty() )
        return false;
    
    KComponentData component = KGlobal::mainComponent();
    if( component.componentName() != QLatin1String("nepomukindexer") ) {
        component = KComponentData( QByteArray("nepomukindexer"),
                                    QByteArray(), KComponentData::SkipMainComponentRegistration );
    }
    kDebug() << component.componentName();
    KJob* job = Nepomuk::removeDataByApplication( QList<QUrl>() << res, RemoveSubResoures, component );
    job->exec();

    if( job->error() ) {
        kError() << job->errorString();
    }
    
    QString query = QString::fromLatin1( "select ?g where { ?g %1 %2 . }" )
                    .arg( Soprano::Node::resourceToN3( Strigi::Ontology::indexGraphFor() ),
                          Soprano::Node::resourceToN3( res ) );

    Soprano::QueryResultIterator result = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    while ( result.next() ) {
        // delete the indexed data (The Soprano::NRLModel in the storage service will take care of
        // the metadata graph)
        Nepomuk::ResourceManager::instance()->mainModel()->removeContext( result.binding( "g" ) );
    }

    return true;
}