/* This file is part of the KDE Project
   Copyright (c) 2007-2008 Sebastian Trueg <trueg@kde.org>

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

#include "nepomukfilewatch.h"

#include <QtCore/QTimer>
#include <QtCore/QDir>
#include <QtCore/QRegExp>
#include <QtCore/QFileInfo>
#include <QtDBus/QDBusConnection>

#include <kdebug.h>
#include <KUrl>
#include <kmessagebox.h>
#include <klocale.h>
#include <KPluginFactory>
#include <kio/netaccess.h>

#include <Soprano/Model>
#include <Soprano/StatementIterator>
#include <Soprano/Statement>
#include <Soprano/Node>
#include <Soprano/NodeIterator>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/Xesam>

// Restrictions and TODO:
// ----------------------
//
// * KIO slaves that do change the local file system may emit stuff like
//   file:///foo/bar -> xyz://foobar while the file actually ends up in
//   the local file system again. This is not handled here. It is maybe
//   necessary to use KFileItem::mostLocalUrl to determine local paths
//   before deciding to call updateMetaDataForResource.
//
// * Only operations done through KIO are caught
//

using namespace Soprano;


NEPOMUK_EXPORT_SERVICE( Nepomuk::FileWatch, "nepomukfilewatch")


namespace {
    Soprano::QueryResultIterator queryChildren( Model* model, const QString& path )
    {
        // escape special chars
        QString regexpPath( path );
        if ( regexpPath[regexpPath.length()-1] != '/' ) {
            regexpPath += '/';
        }
        regexpPath.replace( QRegExp( "([\\.\\?\\*\\\\+\\(\\)\\\\\\|\\[\\]{}])" ), "\\\\\\1" );

//        kDebug() << "query:" << QString( "select ?r ?p where { ?r <http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#fileUrl> ?p FILTER(REGEX(STR(?p), '^%1')) . }" ).arg( regexpPath );

        // query for all files that
        return model->executeQuery( QString( "prefix xesam: <http://freedesktop.org/standards/xesam/1.0/core#> "
                                             "select ?r ?p where { ?r xesam:url ?p . FILTER(REGEX(STR(?p), '^%1')) . }" ).arg( regexpPath ),
                                    Soprano::Query::QueryLanguageSparql );
    }
}



Nepomuk::FileWatch::FileWatch( QObject* parent, const QList<QVariant>& )
    : Service( parent )
{
    // monitor KIO for changes
    QDBusConnection::sessionBus().connect( QString(), QString(), "org.kde.KDirNotify", "FileMoved",
                                           this, SLOT( slotFileMoved( const QString&, const QString& ) ) );
    QDBusConnection::sessionBus().connect( QString(), QString(), "org.kde.KDirNotify", "FilesRemoved",
                                           this, SLOT( slotFilesDeleted( const QStringList& ) ) );
}


Nepomuk::FileWatch::~FileWatch()
{
}


void Nepomuk::FileWatch::slotFileMoved( const QString& urlFrom, const QString& urlTo )
{
    KUrl from( urlFrom );
    KUrl to( urlTo );

    kDebug() << from << to;

    if ( from.isEmpty() || to.isEmpty() ) {
        kDebug() << "empty path. Looks like a bug somewhere...";
        return;
    }

    if ( mainModel() ) {
        // We do NOT get deleted messages for overwritten files! Thus, we have to remove all metadata for overwritten files
        // first. We do that now.
        removeMetaData( to );

        // and finally update the old statements
        updateMetaData( from, to );

        // update children files in case from is a folder
        QString fromPath = from.path();
        QList<Soprano::BindingSet> children = queryChildren( mainModel(), fromPath ).allBindings();
        foreach( const Soprano::BindingSet& bs, children ) {
            QString path = to.path();
            if ( !path.endsWith( '/' ) )
                path += '/';
            path += bs[1].toString().mid( fromPath.endsWith( '/' ) ? fromPath.length() : fromPath.length()+1 );
            updateMetaData( bs[1].toString(), path ); // FIXME: reuse the URI we already have
        }

        // TODO: optionally create a xesam:url property in case a file was moved from a remote URL to a local one
        // still disabled since we also need a new context and that is much easier with a proper NRLModel which
        // we will hopefully have in Soprano 2.2
//         if ( to.isLocalFile() ) {
//             if ( !mainModel()->containsAnyStatement( to, Soprano::Vocabulary::Xesam::url(), Soprano::Node() ) ) {
//                 mainModel()->addStatement( to, Soprano::Vocabulary::Xesam::url(), Soprano::LiteralValue( to.path() ) );
//             }
//         }
    }
    else {
        kDebug() << "Could not contact Nepomuk server.";
    }
}


void Nepomuk::FileWatch::slotFilesDeleted( const QStringList& paths )
{
    foreach( const QString& path, paths ) {
        slotFileDeleted( path );
    }
}


void Nepomuk::FileWatch::slotFileDeleted( const QString& urlString )
{
    KUrl url( urlString );

    kDebug() << url;

    if ( mainModel() ) {
        removeMetaData( url );

        // remove child annotations in case it is a local folder
        foreach( Soprano::Node node, queryChildren( mainModel(), url.path() ).iterateBindings( 0 ).allNodes() ) {
            mainModel()->removeAllStatements( Statement( node, Node(), Node() ) );
        }
    }
    else {
        kDebug() << "Could not contact Nepomuk server.";
    }
}


void Nepomuk::FileWatch::removeMetaData( const KUrl& url )
{
    kDebug() << url;

    if ( url.isEmpty() ) {
        kDebug() << "empty path. Looks like a bug somewhere...";
        return;
    }

    mainModel()->removeAllStatements( Statement( url, Node(), Node() ) );

    // FIXME: what about the triples that have our uri as object?
}


void Nepomuk::FileWatch::updateMetaData( const KUrl& from, const KUrl& to )
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
    if ( mainModel()->containsAnyStatement( Soprano::Statement( oldResource, Soprano::Node(), Soprano::Node() ) ) ) {

        QList<Soprano::Statement> sl = mainModel()->listStatements( Soprano::Statement( oldResource,
                                                                                        Soprano::Node(),
                                                                                        Soprano::Node() ) ).allStatements();
        Q_FOREACH( Soprano::Statement s, sl ) {
            if ( s.predicate() == Soprano::Vocabulary::Xesam::url() ) {
                mainModel()->addStatement( Soprano::Statement( newResource,
                                                               s.predicate(),
                                                               Soprano::LiteralValue( to.path() ),
                                                               s.context() ) );

            }
            else {
                mainModel()->addStatement( Soprano::Statement( newResource,
                                                               s.predicate(),
                                                               s.object(),
                                                               s.context() ) );
            }
        }

        mainModel()->removeStatements( sl );
        // -----------------------------------------------


        // update resources relating to it
        // -----------------------------------------------
        sl = mainModel()->listStatements( Statement( Node(),
                                                     Node(),
                                                     oldResource ) ).allStatements();
        Q_FOREACH( Soprano::Statement s, sl ) {
            mainModel()->addStatement( Soprano::Statement( s.subject(),
                                                           s.predicate(),
                                                           newResource,
                                                           s.context() ) );
        }
        mainModel()->removeStatements( sl );
        // -----------------------------------------------
    }
}

#include "nepomukfilewatch.moc"
