/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2011 Vishesh Handa <handa.vish@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "resourceidentifier.h"
#include "syncresource.h"
#include "classandpropertytree.h"

#include <QtCore/QDateTime>
#include <QtCore/QSet>

#include <Soprano/Node>
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/StatementIterator>
#include <Soprano/NodeIterator>
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Vocabulary/RDF>
#include <Nepomuk/Vocabulary/NIE>

#include <KDebug>

using namespace Soprano::Vocabulary;
using namespace Nepomuk::Vocabulary;

namespace {
    /// used to handle sets and lists of QUrls
    template<typename T> QStringList resourcesToN3(const T& urls) {
        QStringList n3;
        Q_FOREACH(const QUrl& url, urls) {
            n3 << Soprano::Node::resourceToN3(url);
        }
        return n3;
    }
}

Nepomuk::ResourceIdentifier::ResourceIdentifier( Nepomuk::StoreIdentificationMode mode,
                                                 Soprano::Model *model)
    : Nepomuk::Sync::ResourceIdentifier( model ),
      m_mode( mode )
{
    // Resource Metadata
    addOptionalProperty( NAO::created() );
    addOptionalProperty( NAO::lastModified() );
    addOptionalProperty( NAO::creator() );
    addOptionalProperty( NAO::userVisible() );
}


bool Nepomuk::ResourceIdentifier::exists(const KUrl& uri)
{
    QString query = QString::fromLatin1("ask { %1 ?p ?o . } ").arg( Soprano::Node::resourceToN3(uri) );
    return model()->executeQuery( query, Soprano::Query::QueryLanguageSparql ).boolValue();
}

KUrl Nepomuk::ResourceIdentifier::duplicateMatch(const KUrl& origUri,
                                                 const QSet<KUrl>& matchedUris )
{
    Q_UNUSED( origUri );
    //
    // We return the uri that has the oldest nao:created
    // For backwards compatibility we keep in mind that three are resources which do not have nao:created defined.
    //
    Soprano::QueryResultIterator it
            = model()->executeQuery(QString::fromLatin1("select ?r where { ?r %1 ?date . FILTER(?r in (%2)) . } ORDER BY ASC(?date) LIMIT 1")
                                    .arg(Soprano::Node::resourceToN3(NAO::created()),
                                         resourcesToN3(matchedUris).join(QLatin1String(","))),
                                    Soprano::Query::QueryLanguageSparql);
    if(it.next()) {
        return it[0].uri();
    }
    else {
        // FIXME: fallback to what? a random one from the set?
        return KUrl();
    }
}

bool Nepomuk::ResourceIdentifier::isIdentifyingProperty(const QUrl& uri)
{
    if( uri == NAO::created()
            || uri == NAO::creator()
            || uri == NAO::lastModified()
            || uri == NAO::userVisible() ) {
        return false;
    }
    else {
        return ClassAndPropertyTree::self()->isIdentifyingProperty(uri);
    }
}


bool Nepomuk::ResourceIdentifier::runIdentification(const KUrl& uri)
{
    if( m_mode == IdentifyNone )
        return false;

    if( m_mode == IdentifyNew ) {
        if( exists( uri ) ) {
            manualIdentification( uri, uri );
            return true;
        }
    }

    //kDebug() << "Identifying : " << uri;
    //
    // Check if a uri with the same name exists
    //
    if( exists( uri ) ) {
        manualIdentification( uri, uri );
        return true;
    }

    const Sync::SyncResource & res = simpleResource( uri );
    //kDebug() << res;

    //
    // Check if a uri with the same nie:url exists
    //
    QUrl nieUrl = res.nieUrl();
    if( !nieUrl.isEmpty() ) {
        QString query = QString::fromLatin1("select ?r where { ?r %1 %2 . }")
                        .arg( Soprano::Node::resourceToN3( NIE::url() ),
                              Soprano::Node::resourceToN3( nieUrl ) );
        Soprano::QueryResultIterator it = model()->executeQuery( query, Soprano::Query::QueryLanguageSparql );
        if( it.next() ) {
            const QUrl newUri = it["r"].uri();
            kDebug() << uri << " --> " << newUri;
            manualIdentification( uri, newUri );
            return true;
        }

        return false;
    }

    // Run the normal identification procedure
    return Sync::ResourceIdentifier::runIdentification( uri );
}
