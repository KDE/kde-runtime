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
#include "nepomuktools.h"

#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QUuid>
#include <QtCore/QScopedPointer>
#include <QtCore/QDebug>

#include <Soprano/Model>
#include <Soprano/Statement>
#include <Soprano/QueryResultIterator>
#include <Soprano/NodeIterator>
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


QUrl Strigi::Ontology::indexGraphFor()
{
    return QUrl::fromEncoded( "http://www.strigi.org/fields#indexGraphFor", QUrl::StrictMode );
}

KJob* Nepomuk::clearIndexedData( const QUrl& url )
{
    return clearIndexedData(QList<QUrl>() << url);
}

KJob* Nepomuk::clearIndexedData( const QList<QUrl>& urls )
{
    if ( urls.isEmpty() )
        return 0;

    kDebug() << urls;

    //
    // New way of storing Strigi Data
    // The Datamanagement API will automatically find the resource corresponding to that url
    //
    KComponentData component = KGlobal::mainComponent();
    if( component.componentName() != QLatin1String("nepomukindexer") ) {
        component = KComponentData( QByteArray("nepomukindexer"),
                                    QByteArray(), KComponentData::SkipMainComponentRegistration );
    }
    return Nepomuk::removeDataByApplication( urls, RemoveSubResoures, component );
}

bool Nepomuk::clearLegacyIndexedDataForUrls( const KUrl::List& urls )
{
    if ( !urls.isEmpty() )
        return true;

    if(urls.count() > Nepomuk::MAX_SPLIT_LIST_ITEMS) {
        foreach(const QList<KUrl>& u, Nepomuk::splitList(urls)) {
            if(!clearLegacyIndexedDataForUrls(u)) {
                return false;
            }
        }
    }
    else {
        kDebug() << urls;

        const QString query
                = QString::fromLatin1( "select ?g where { "
                                       "{ "
                                       "?r %2 ?u . "
                                       "FILTER(?u in (%1)) . "
                                       "?g %3 ?r . } "
                                       "UNION "
                                       "{ ?g %3 ?u . "
                                       "FILTER(?u in (%1)) . } . "
                                       "}")
                .arg( Nepomuk::resourcesToN3( urls ).join(QLatin1String(",")),
                      Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                      Soprano::Node::resourceToN3( Strigi::Ontology::indexGraphFor() ) );

        const QList<Soprano::Node> graphs =
                Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql )
                .iterateBindings(0).allNodes();

        foreach(const Soprano::Node& graph, graphs) {
            // delete the indexed data (The Soprano::NRLModel in the storage service will take care of
            // the metadata graph)
            if(Nepomuk::ResourceManager::instance()->mainModel()->removeContext( graph ) != Soprano::Error::ErrorNone) {
                return false;
            }
        }
    }

    return true;
}

bool Nepomuk::clearLegacyIndexedDataForResourceUri( const KUrl& res )
{
    if ( res.isEmpty() )
        return false;

    kDebug() << res;

    QString query = QString::fromLatin1( "select ?g where { ?g %1 %2 . }" )
                    .arg( Soprano::Node::resourceToN3( Strigi::Ontology::indexGraphFor() ),
                          Soprano::Node::resourceToN3( res ) );

    const QList<Soprano::Node> graphs =
            Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql )
            .iterateBindings(0).allNodes();

    foreach(const Soprano::Node& graph, graphs) {
        // delete the indexed data (The Soprano::NRLModel in the storage service will take care of
        // the metadata graph)
        Nepomuk::ResourceManager::instance()->mainModel()->removeContext( graph );
    }

    return true;
}
