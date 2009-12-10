/* This file is part of the KDE Project
   Copyright (c) 2009 Sebastian Trueg <trueg@kde.org>

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

#include <KDebug>


Nepomuk::MetadataMover::MetadataMover( Soprano::Model* model, QObject* parent )
    : QThread( parent ),
      m_stopped(false),
      m_model( model ),
      m_strigiParentUrlUri( "http://strigi.sf.net/ontologies/0.9#parentUrl" )
{
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
    m_queueMutex.lock();
    m_updateQueue.enqueue( qMakePair( from, to ) );
    m_queueMutex.unlock();
    m_queueWaiter.wakeAll();
}


void Nepomuk::MetadataMover::removeFileMetadata( const KUrl& file )
{
    m_queueMutex.lock();
    m_updateQueue.enqueue( qMakePair( file, KUrl() ) );
    m_queueMutex.unlock();
    m_queueWaiter.wakeAll();
}


void Nepomuk::MetadataMover::removeFileMetadata( const KUrl::List& files )
{
    m_queueMutex.lock();
    foreach( const KUrl& file, files )
        m_updateQueue.enqueue( qMakePair( file, KUrl() ) );
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
            QPair<KUrl, KUrl> updateData = m_updateQueue.dequeue();

            // unlock after queue utilization
            m_queueMutex.unlock();

            // an empty second url means deletion
            if( updateData.second.isEmpty() ) {
                KUrl url = updateData.first;
                removeMetadata( url );
            }
            else {
                KUrl from = updateData.first;
                KUrl to = updateData.second;

                // We do NOT get deleted messages for overwritten files! Thus, we
                // have to remove all metadata for overwritten files first.
                removeMetadata( to );

                // and finally update the old statements
                updateMetadata( from, to );
            }

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


void Nepomuk::MetadataMover::updateMetadata( const KUrl& from, const KUrl& to )
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

        //
        // Recursively update children
        //
        Query::Query query( Query::ComparisonTerm( Nepomuk::Vocabulary::NIE::isPartOf(), Query::ResourceTerm( newResource ) ) );
        query.addRequestProperty( Query::Query::RequestProperty( Nepomuk::Vocabulary::NIE::url() ) );
        Soprano::QueryResultIterator it = m_model->executeQuery( query.toSparqlQuery(), Soprano::Query::QueryLanguageSparql );
        while ( it.next() ) {
            QUrl uri = it[0].uri();
            KUrl url = it[1].uri();
            KUrl newUrl( to );
            newUrl.addPath( url.fileName() );
            updateMetadata( url, newUrl );
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

#include "metadatamover.moc"
