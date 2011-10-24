/* This file is part of the KDE Project
   Copyright (c) 2009-2011 Sebastian Trueg <trueg@kde.org>

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
    : QObject( parent ),
      m_queueMutex(QMutex::Recursive),
      m_model( model )
{
    // setup the main update queue timer
    m_queueTimer = new QTimer(this);
    connect(m_queueTimer, SIGNAL(timeout()),
            this, SLOT(slotWorkUpdateQueue()),
            Qt::DirectConnection);

    // setup the cleanup timer which removes requests that are done
    m_recentlyFinishedRequestsTimer = new QTimer(this);
    connect( m_recentlyFinishedRequestsTimer, SIGNAL( timeout() ),
             this, SLOT( slotClearRecentlyFinishedRequests() ),
             Qt::DirectConnection );
    m_recentlyFinishedRequestsTimer->setInterval( 30000 );
}


Nepomuk::MetadataMover::~MetadataMover()
{
}


void Nepomuk::MetadataMover::moveFileMetadata( const KUrl& from, const KUrl& to )
{
//    kDebug() << from << to;
    Q_ASSERT( !from.path().isEmpty() && from.path() != "/" );
    Q_ASSERT( !to.path().isEmpty() && to.path() != "/" );

    QMutexLocker lock(&m_queueMutex);

    UpdateRequest req( from, to );
    if ( !m_updateQueue.contains( req ) &&
         !m_recentlyFinishedRequests.contains( req ) )
        m_updateQueue.enqueue( req );

    QTimer::singleShot(0, this, SLOT(slotStartUpdateTimer()));
}


void Nepomuk::MetadataMover::removeFileMetadata( const KUrl& file )
{
    Q_ASSERT( !file.path().isEmpty() && file.path() != "/" );
    removeFileMetadata( KUrl::List() << file );
}


void Nepomuk::MetadataMover::removeFileMetadata( const KUrl::List& files )
{
    kDebug() << files;
    QMutexLocker lock(&m_queueMutex);

    foreach( const KUrl& file, files ) {
        UpdateRequest req( file );
        if ( !m_updateQueue.contains( req ) &&
             !m_recentlyFinishedRequests.contains( req ) )
            m_updateQueue.enqueue( req );
    }

    QTimer::singleShot(0, this, SLOT(slotStartUpdateTimer()));
}


void Nepomuk::MetadataMover::slotWorkUpdateQueue()
{
    // lock for initial iteration
    QMutexLocker lock(&m_queueMutex);

    // work the queue
    if( !m_updateQueue.isEmpty() ) {
        UpdateRequest updateRequest = m_updateQueue.dequeue();
        m_recentlyFinishedRequests.insert( updateRequest );

        // unlock after queue utilization
        lock.unlock();

//        kDebug() << "========================= handling" << updateRequest.source() << updateRequest.target();

        // an empty second url means deletion
        if( updateRequest.target().isEmpty() ) {
            removeMetadata( updateRequest.source() );
        }
        else {
            const KUrl from = updateRequest.source();
            const KUrl to = updateRequest.target();

            // We do NOT get deleted messages for overwritten files! Thus, we
            // have to remove all metadata for overwritten files first.
            removeMetadata( to );

            // and finally update the old statements
            updateMetadata( from, to );
        }

//        kDebug() << "========================= done with" << updateRequest.source() << updateRequest.target();
    }
    else {
        kDebug() << "All update requests handled. Stopping timer.";
        m_queueTimer->stop();
    }
}


void Nepomuk::MetadataMover::removeMetadata( const KUrl& url )
{
//    kDebug() << url;

    if ( url.isEmpty() ) {
        kDebug() << "empty path. Looks like a bug somewhere...";
    }
    else {
        const bool isFolder = url.url().endsWith('/');
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
        // If we have no metadata yet we need to tell strigi (if running) so it can
        // create the metadata in case the target folder is configured to be indexed.
        //
        emit movedWithoutData( to.path() );
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

    if(m_recentlyFinishedRequests.isEmpty()) {
        kDebug() << "No more old requests. Stopping timer.";
        m_recentlyFinishedRequestsTimer->stop();
    }
}


// start the timer in the update thread
void Nepomuk::MetadataMover::slotStartUpdateTimer()
{
    if(!m_queueTimer->isActive()) {
        m_queueTimer->start();
    }
    if(!m_recentlyFinishedRequestsTimer->isActive()) {
        m_recentlyFinishedRequestsTimer->start();
    }
}

#include "metadatamover.moc"
