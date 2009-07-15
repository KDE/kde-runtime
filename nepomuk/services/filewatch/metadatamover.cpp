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
#include "nfo.h"
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

#include <KDebug>


namespace {
    Soprano::QueryResultIterator queryChildren( Soprano::Model* model, const QString& path )
    {
        // escape special chars
        QString regexpPath( path );
        if ( regexpPath[regexpPath.length()-1] != '/' ) {
            regexpPath += '/';
        }
        regexpPath.replace( QRegExp( "([\\.\\?\\*\\\\+\\(\\)\\\\\\|\\[\\]{}])" ), "\\\\\\1" );

//        kDebug() << "query:" << QString( "select ?r ?p where { ?r <http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#fileUrl> ?p FILTER(REGEX(STR(?p), '^%1')) . }" ).arg( regexpPath );

        // query for all files that
        return model->executeQuery( QString( "select ?r ?p where { "
                                             "{ ?r %1 ?p . } "
                                             "UNION "
                                             "{ ?r %2 ?p . } . "
                                             "FILTER(REGEX(STR(?p), '^%3')) . "
                                             "}" )
                                    .arg( Soprano::Node::resourceToN3( Soprano::Vocabulary::Xesam::url() ) )
                                    .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NFO::fileUrl() ) )
                                    .arg( regexpPath ),
                                    Soprano::Query::QueryLanguageSparql );
    }
}


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

                // remove child annotations in case it is a local folder
                foreach( const Soprano::Node& node, queryChildren( m_model, url.path() ).iterateBindings( 0 ).allNodes() ) {
                    m_model->removeAllStatements( node, Soprano::Node(), Soprano::Node() );
                }
            }
            else {
                KUrl from = updateData.first;
                KUrl to = updateData.second;

                // We do NOT get deleted messages for overwritten files! Thus, we have to remove all metadata for overwritten files
                // first. We do that now.
                removeMetadata( to );

                // and finally update the old statements
                updateMetadata( from, to );

                // update children files in case from is a folder
                QString fromPath = from.path();
                QList<Soprano::BindingSet> children = queryChildren( m_model, fromPath ).allBindings();
                foreach( const Soprano::BindingSet& bs, children ) {
                    QString path = to.path();
                    if ( !path.endsWith( '/' ) )
                        path += '/';
                    path += bs[1].toString().mid( fromPath.endsWith( '/' ) ? fromPath.length() : fromPath.length()+1 );
                    updateMetadata( bs[1].toString(), path ); // FIXME: reuse the URI we already have
                }

                // TODO: optionally create a xesam:url property in case a file was moved from a remote URL to a local one
                // still disabled since we also need a new context and that is much easier with a proper NRLModel which
                // we will hopefully have in Soprano 2.2
//         if ( to.isLocalFile() ) {
//             if ( !m_model->containsAnyStatement( to, Soprano::Vocabulary::Xesam::url(), Soprano::Node() ) ) {
//                 m_model->addStatement( to, Soprano::Vocabulary::Xesam::url(), Soprano::LiteralValue( to.path() ) );
//             }
//         }
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
        return;
    }

    m_model->removeAllStatements( url, Soprano::Node(), Soprano::Node() );

    // FIXME: what about the triples that have our uri as object?
    // FIXME: what about the graph metadata?
}


void Nepomuk::MetadataMover::updateMetadata( const KUrl& from, const KUrl& to )
{
    kDebug() << from << "->" << to;

    //
    // Nepomuk allows annotating of remote files. These files do not necessarily have a xesam:url property
    // since it would not be of much use in the classic sense: we cannot use it to locate the file on the hd
    //
    // Thus, when remote files are moved through KIO and we get the notification, we simply change all triples
    // referring to the original file to use the new URL
    //

    Soprano::Node oldResource = from;
    Soprano::Node newResource = to;

    // update the resource itself
    // -----------------------------------------------
    if ( m_model->containsAnyStatement( Soprano::Statement( oldResource, Soprano::Node(), Soprano::Node() ) ) ) {

        QList<Soprano::Statement> sl = m_model->listStatements( Soprano::Statement( oldResource,
                                                                                    Soprano::Node(),
                                                                                    Soprano::Node() ) ).allStatements();
        Q_FOREACH( const Soprano::Statement& s, sl ) {
            if ( s.predicate() == Soprano::Vocabulary::Xesam::url() ) {
                m_model->addStatement( Soprano::Statement( newResource,
                                                           s.predicate(),
                                                           Soprano::LiteralValue( to.path() ),
                                                           s.context() ) );
            }
            else if ( s.predicate() == m_strigiParentUrlUri ||
                      s.predicate() == Nepomuk::Vocabulary::NIE::isPartOf() ) {
                m_model->addStatement( Soprano::Statement( newResource,
                                                           s.predicate(),
                                                           Soprano::LiteralValue( to.directory( KUrl::IgnoreTrailingSlash ) ),
                                                           s.context() ) );
            }
            else {
                m_model->addStatement( Soprano::Statement( newResource,
                                                           s.predicate(),
                                                           s.object(),
                                                           s.context() ) );
            }
        }

        m_model->removeStatements( sl );
        // -----------------------------------------------


        // update resources relating to it
        // -----------------------------------------------
        sl = m_model->listStatements( Soprano::Node(),
                                      Soprano::Node(),
                                      oldResource ).allStatements();
        Q_FOREACH( const Soprano::Statement& s, sl ) {
            m_model->addStatement( s.subject(),
                                   s.predicate(),
                                   newResource,
                                   s.context() );
        }
        m_model->removeStatements( sl );
        // -----------------------------------------------
    }
}

#include "metadatamover.moc"
