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
#include "nepomukfilewatch.h"
#include "datamanagement.h"

#include <QtCore/QTimer>

#include <Soprano/Model>
#include <Soprano/Node>
#include <Soprano/NodeIterator>
#include <Soprano/QueryResultIterator>
#include <Soprano/LiteralValue>

#include <Nepomuk/Vocabulary/NIE>

#include <KDebug>


Nepomuk::MetadataMover::MetadataMover( Soprano::Model* model, QObject* parent )
    : QThread( parent ),
      m_stopped(false),
      m_model( model )
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
    QMutexLocker lock( &m_queueMutex );
    m_stopped = true;
    m_queueWaiter.wakeAll();
}


void Nepomuk::MetadataMover::moveFileMetadata( const KUrl& from, const KUrl& to )
{
    kDebug() << from << to;
    Q_ASSERT( !from.path().isEmpty() && from.path() != "/" );
    Q_ASSERT( !to.path().isEmpty() && to.path() != "/" );
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
    Q_ASSERT( !file.path().isEmpty() && file.path() != "/" );
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
        const bool isFolder = url.url().endsWith('/');
        kDebug() << "removing metadata for file" << url;
        Nepomuk::removeResources(QList<QUrl>() << url);

        if( isFolder ) {
            //
            // Recursively remove children
            // We cannot use the nie:isPartOf relation since only children could have metadata. Thus, we do a regex
            // match on all files and folders below the URL we got.
            //
            // CAUTION: The trailing slash on the from URL is essential! Otherwise we might match the newly added
            //          URLs, too (in case a rename only added chars to the name)
            //
            const QString query = QString::fromLatin1( "select distinct ?r where { "
                                                    "?r %1 ?url . "
                                                    "FILTER(REGEX(STR(?url),'^%2')) . "
                                                    "}" )
                                .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ),
                                        url.url(KUrl::AddTrailingSlash) );
            kDebug() << query;

            //
            // We cannot use one big loop since our updateMetadata calls below can change the iterator
            // which could have bad effects like row skipping. Thus, we handle the urls in chunks of
            // cached items.
            //
            while ( 1 ) {
                QList<QUrl> urls;
                Soprano::QueryResultIterator it = m_model->executeQuery( query + QLatin1String( " LIMIT 20" ),
                                                                         Soprano::Query::QueryLanguageSparql );
                while(it.next()) {
                    urls << it[0].uri();
                }
                if ( !urls.isEmpty() ) {
                    Nepomuk::removeResources(urls);
                }
                else {
                    break;
                }
            }
        }
    }
}


void Nepomuk::MetadataMover::updateMetadata( const KUrl& from, const KUrl& to )
{
    kDebug() << from << "->" << to;

    if ( m_model->executeQuery(QString::fromLatin1("ask where { { %1 ?p ?o . } UNION { ?r nie:url %1 . } . }")
                               .arg(Soprano::Node::resourceToN3(from)),
                               Soprano::Query::QueryLanguageSparql).boolValue() ) {
        Nepomuk::setProperty(QList<QUrl>() << from, Nepomuk::Vocabulary::NIE::url(), QVariantList() << to);
    }
    else {
        //
        // If we have no metadata yet we need to tell the file indexer (if running) so it can
        // create the metadata in case the target folder is configured to be indexed.
        //
        emit movedWithoutData( to.directory( KUrl::IgnoreTrailingSlash ) );
    }
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
