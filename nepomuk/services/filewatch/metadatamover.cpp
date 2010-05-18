/* This file is part of the KDE Project
   Copyright (c) 2009-2010 Sebastian Trueg <trueg@kde.org>

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

#include "metadatamover.h"
#include "nie.h"

#include <QtCore/QDir>
#include <QtCore/QRegExp>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#include <Soprano/Model>
#include <Soprano/StatementIterator>
#include <Soprano/Statement>
#include <Soprano/Node>
#include <Soprano/NodeIterator>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/Xesam>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>
#include <Nepomuk/Types/Property>
#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/ComparisonTerm>
#include <Nepomuk/Query/ResourceTerm>
#include <Nepomuk/Query/LiteralTerm>

#include <KDebug>


Nepomuk::MetadataMover::MetadataMover( Soprano::Model* model, QObject* parent )
    : QThread( parent ),
      m_stopped(false),
      m_model( model ),
      m_strigiParentUrlUri( "http://strigi.sf.net/ontologies/0.9#parentUrl" )
{
    QTimer* timer = new QTimer( this );
    connect( timer, SIGNAL( timeout() ), this, SLOT( slotClearRecentlyFinishedRequests() ) );
    timer->setInterval( 30000 );
    timer->start();
}


Nepomuk::MetadataMover::~MetadataMover()
{
}


void Nepomuk::MetadataMover::stop()
{
    m_stopped = true;
    m_queueWaiter.wakeAll();
}


void Nepomuk::MetadataMover::moveFileMetadata( const KUrl& from, const KUrl& to )
{
    kDebug() << from << to;
    m_queueMutex.lock();
    UpdateRequest req( from, to );
    if ( !m_updateQueue.contains( req ) &&
         !m_recentlyFinishedRequests.contains( req ) )
        m_updateQueue.enqueue( req );
    m_queueMutex.unlock();
    m_queueWaiter.wakeAll();
}


void Nepomuk::MetadataMover::removeFileMetadata( const KUrl& file )
{
    removeFileMetadata( KUrl::List() << file );
}


void Nepomuk::MetadataMover::removeFileMetadata( const KUrl::List& files )
{
    kDebug() << files;
    m_queueMutex.lock();
    foreach( const KUrl& file, files ) {
        UpdateRequest req( file );
        if ( !m_updateQueue.contains( req ) &&
             !m_recentlyFinishedRequests.contains( req ) )
            m_updateQueue.enqueue( req );
    }
    m_queueMutex.unlock();
    m_queueWaiter.wakeAll();
}


// FIXME: somehow detect when the nepomuk storage service is down and wait for it to come up again (how about having methods for that in Nepomuk::Service?)
void Nepomuk::MetadataMover::run()
{
    m_stopped = false;
    while( !m_stopped ) {

        // lock for initial iteration
        m_queueMutex.lock();

        // work the queue
        while( !m_updateQueue.isEmpty() ) {
            UpdateRequest updateRequest = m_updateQueue.dequeue();
            m_recentlyFinishedRequests.insert( updateRequest );

            // unlock after queue utilization
            m_queueMutex.unlock();

            kDebug() << "========================= handling" << updateRequest.source() << updateRequest.target();

            //
            // IMPORTANT: clear the ResourceManager cache since otherwise the following will happen
            // (see also ResourceData::invalidateCache):
            // We update the nie:url for resource X. Afterwards we get a fileDeleted event for the
            // old URL. That one is still in the ResourceManager cache and will bring us directly
            // to resource X. As a result we will delete all its metadata.
            // This is a bug in ResourceManager for which I (trueg) have not found a good solution yet.
            //
            ResourceManager::instance()->clearCache();

            // an empty second url means deletion
            if( updateRequest.target().isEmpty() ) {
                KUrl url = updateRequest.source();
                removeMetadata( url );
            }
            else {
                KUrl from = updateRequest.source();
                KUrl to = updateRequest.target();

                // We do NOT get deleted messages for overwritten files! Thus, we
                // have to remove all metadata for overwritten files first.
                removeMetadata( to );

                // and finally update the old statements
                updateMetadata( from, to );
            }

            kDebug() << "========================= done with" << updateRequest.source() << updateRequest.target();

            // lock for next iteration
            m_queueMutex.lock();
        }

        // wait for more input
        kDebug() << "Waiting...";
        m_queueWaiter.wait( &m_queueMutex );
        m_queueMutex.unlock();
        kDebug() << "Woke up.";
    }
}


void Nepomuk::MetadataMover::removeMetadata( const KUrl& url )
{
    kDebug() << url;

    if ( url.isEmpty() ) {
        kDebug() << "empty path. Looks like a bug somewhere...";
    }
    else {
        Resource res( url );
        if ( res.exists() ) {
            kDebug() << "removing metadata for file" << url << "with resource URI" << res.resourceUri();
            Query::Query query( Query::ComparisonTerm( Nepomuk::Vocabulary::NIE::isPartOf(), Query::ResourceTerm( res ) ) );
            Soprano::QueryResultIterator it = m_model->executeQuery( query.toSparqlQuery(), Soprano::Query::QueryLanguageSparql );
            while ( it.next() ) {
                removeMetadata( it[0].uri() );
            }
            res.remove();
        }
    }
}


void Nepomuk::MetadataMover::updateMetadata( const KUrl& from, const KUrl& to, bool includeChildren )
{
    kDebug() << from << "->" << to;

    //
    // Since KDE 4.4 we use nepomuk:/res/<UUID> Uris for all resources including files. Thus, moving a file
    // means updating two things:
    // 1. the nie:url property
    // 2. the nie:isPartOf relation (only necessary if the file and not the whole folder was moved)
    //
    // However, since we will still have file:/ resource URIs from old data we will also handle that
    // and convert them to new nepomuk:/res/<UUID> style URIs.
    //

    Resource oldResource( from );

    if ( oldResource.exists() ) {
        QUrl oldResourceUri = oldResource.resourceUri();
        QUrl newResourceUri = oldResourceUri;

        //
        // Handle legacy file:/ resource URIs
        //
        if ( from.equals( oldResourceUri, KUrl::CompareWithoutTrailingSlash ) ) {
            newResourceUri = updateLegacyMetadata( oldResourceUri );
        }


        //
        // Now update the nie:url and nie:isPartOf relations
        //
        Resource newResource( newResourceUri );
        newResource.setProperty( Nepomuk::Vocabulary::NIE::url(), to );
        Resource newParent( to.directory( KUrl::IgnoreTrailingSlash ) );
        if ( newParent.exists() ) {
            newResource.setProperty( Nepomuk::Vocabulary::NIE::isPartOf(), newParent );
        }
    }

    if ( includeChildren && QFileInfo( to.toLocalFile() ).isDir() ) {
        //
        // Recursively update children
        // We cannot use the nie:isPartOf relation since only children could have metadata. Thus, we do a regex
        // match on all files and folders below the URL we got.
        //
        // CAUTION: The trailing slash on the from URL is essential! Otherwise we might match the newly added
        //          URLs, too (in case a rename only added chars to the name)
        //
        QString query = QString::fromLatin1( "select distinct ?r ?url where { "
                                             "?r %1 ?url . "
                                             "FILTER(REGEX(STR(?url),'^%2')) . "
                                             "}" )
                        .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                              from.url(KUrl::AddTrailingSlash) );
        kDebug() << query;
        
        const QString oldBasePath = from.path( KUrl::AddTrailingSlash );
        const QString newBasePath = to.path( KUrl::AddTrailingSlash );
        
        Soprano::QueryResultIterator it = m_model->executeQuery( query, Soprano::Query::QueryLanguageSparql );
        while ( it.next() ) {

            // the resource URI of the resource to update
            const QUrl uri = it[0].uri();

            // the old URL of the resource to update
            const KUrl url = it[1].uri();

            // now construct the new URL
            QString oldRelativePath = url.path().mid( oldBasePath.length() );
            KUrl newUrl( newBasePath + oldRelativePath );

            // finally update the metadata (excluding children since we already handle them all here)
            updateMetadata( url, newUrl, false );
        }
    }
}


QUrl Nepomuk::MetadataMover::updateLegacyMetadata( const QUrl& oldResourceUri )
{
    kDebug() << oldResourceUri;

    //
    // Get a new resource URI
    //
    QUrl newResourceUri = ResourceManager::instance()->generateUniqueUri( QString() );

    //
    // update the resource properties
    //
    QList<Soprano::Statement> sl = m_model->listStatements( Soprano::Statement( oldResourceUri,
                                                                                Soprano::Node(),
                                                                                Soprano::Node() ) ).allStatements();
    Q_FOREACH( const Soprano::Statement& s, sl ) {
        //
        // Skip all the stuff we will update later on
        //
        if ( s.predicate() == Soprano::Vocabulary::Xesam::url() ||
             s.predicate() == Nepomuk::Vocabulary::NIE::url() ||
             s.predicate() == m_strigiParentUrlUri ||
             s.predicate() == Nepomuk::Vocabulary::NIE::isPartOf() ) {
            continue;
        }

        m_model->addStatement( Soprano::Statement( newResourceUri,
                                                   s.predicate(),
                                                   s.object(),
                                                   s.context() ) );
    }

    m_model->removeStatements( sl );


    //
    // update resources relating to it
    //
    sl = m_model->listStatements( Soprano::Node(),
                                  Soprano::Node(),
                                  oldResourceUri ).allStatements();
    Q_FOREACH( const Soprano::Statement& s, sl ) {
        m_model->addStatement( s.subject(),
                               s.predicate(),
                               newResourceUri,
                               s.context() );
    }
    m_model->removeStatements( sl );

    kDebug() << "->" << newResourceUri;

    return newResourceUri;
}


// removes all finished requests older than 1 minute
void Nepomuk::MetadataMover::slotClearRecentlyFinishedRequests()
{
    QMutexLocker lock( &m_queueMutex );
    QSet<UpdateRequest>::iterator it = m_recentlyFinishedRequests.begin();
    while ( it != m_recentlyFinishedRequests.end() ) {
        const UpdateRequest& req = *it;
        if ( req.timestamp().secsTo( QDateTime::currentDateTime() ) > 60 ) {
            it = m_recentlyFinishedRequests.erase( it );
        }
        else {
            ++it;
        }
    }
}

#include "metadatamover.moc"
