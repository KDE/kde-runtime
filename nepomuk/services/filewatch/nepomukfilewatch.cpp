/* This file is part of the KDE Project
   Copyright (c) 2007-2010 Sebastian Trueg <trueg@kde.org>

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
#include "metadatamover.h"
#include "strigiserviceinterface.h"
#include "../strigi/priority.h"
#include "nie.h"

#ifdef BUILD_KINOTIFY
#include "kinotify.h"
#endif

#include <QtCore/QDir>
#include <QtCore/QThread>
#include <QtDBus/QDBusConnection>

#include <KDebug>
#include <KUrl>
#include <KPluginFactory>

#include <Nepomuk/ResourceManager>

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Node>


using namespace Soprano;


NEPOMUK_EXPORT_SERVICE( Nepomuk::FileWatch, "nepomukfilewatch")


namespace {

    class RemoveInvalidThread : public QThread {
    public :
        RemoveInvalidThread(QObject* parent = 0);
        void run();
    };

    RemoveInvalidThread::RemoveInvalidThread(QObject* parent): QThread(parent)
    {
        connect( this, SIGNAL(finished()), this, SLOT(deleteLater()) );
    }

    void RemoveInvalidThread::run()
    {
        QString query = QString::fromLatin1( "select distinct ?g ?url where { ?r %1 ?url. FILTER( regex(str(?url), 'file://') ). graph ?g { ?r ?p ?o. } }" )
                                        .arg( Soprano::Node::resourceToN3( Nepomuk::Vocabulary::NIE::url() ) );
        Soprano::QueryResultIterator it = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );

        while( it.next() ) {
            QUrl url( it["url"].uri() );
            QString file = url.toLocalFile();

            if( !file.isEmpty() && !QFile::exists(file) ) {
                kDebug() << "REMOVING " << file;
                Nepomuk::ResourceManager::instance()->mainModel()->removeContext( it["g"] );
            }
        }
    }

}


Nepomuk::FileWatch::FileWatch( QObject* parent, const QList<QVariant>& )
    : Service( parent )
{
    // start the mover thread
    m_metadataMover = new MetadataMover( mainModel(), this );
    connect( m_metadataMover, SIGNAL(movedWithoutData(QString)),
             this, SLOT(slotMovedWithoutData(QString)),
             Qt::QueuedConnection );
    m_metadataMover->start();

#ifdef BUILD_KINOTIFY
    // listing all folders in watchFolder below will be IO-intensive. Do not grab it all
    if ( !lowerIOPriority() )
        kDebug() << "Failed to lower io priority.";

    // monitor the file system for changes (restricted by the inotify limit)
    m_dirWatch = new KInotify( this );

    // FIXME: force to only use maxUserWatches-500 or something or always leave 500 free watches

    connect( m_dirWatch, SIGNAL( moved( const QString&, const QString& ) ),
             this, SLOT( slotFileMoved( const QString&, const QString& ) ) );
    connect( m_dirWatch, SIGNAL( deleted( const QString& ) ),
             this, SLOT( slotFileDeleted( const QString& ) ) );
    connect( m_dirWatch, SIGNAL( created( const QString& ) ),
             this, SLOT( slotFileCreated( const QString& ) ) );
    connect( m_dirWatch, SIGNAL( modified( const QString& ) ),
             this, SLOT( slotFileModified( const QString& ) ) );
    connect( m_dirWatch, SIGNAL( watchUserLimitReached() ),
             this, SLOT( slotInotifyWatchUserLimitReached() ) );

    // recursively watch the whole home dir

    // FIXME: we cannot simply watch the folders that contain annotated files since moving
    // one of these files out of the watched "area" would mean we "lose" it, i.e. we have no
    // information about where it is moved.
    // On the other hand only using the homedir means a lot of restrictions.
    // One dummy solution would be a hybrid: watch the whole home dir plus all folders that
    // contain annotated files outside of the home dir and hope for the best

    watchFolder( QDir::homePath() );
#else
    connectToKDirWatch();
#endif

    (new RemoveInvalidThread())->start();
}


Nepomuk::FileWatch::~FileWatch()
{
    m_metadataMover->stop();
    m_metadataMover->wait();
}


void Nepomuk::FileWatch::watchFolder( const QString& path )
{
    kDebug() << path;
#ifdef BUILD_KINOTIFY
    if ( m_dirWatch && !m_dirWatch->watchingPath( path ) )
        m_dirWatch->addWatch( path,
                              KInotify::WatchEvents( KInotify::EventMove|KInotify::EventDelete|KInotify::EventDeleteSelf|KInotify::EventCreate ),
                              KInotify::WatchFlags() );
#endif
}


void Nepomuk::FileWatch::slotFileMoved( const QString& urlFrom, const QString& urlTo )
{
    KUrl from( urlFrom );
    KUrl to( urlTo );

    kDebug() << from << to;

    m_metadataMover->moveFileMetadata( from, to );
}


void Nepomuk::FileWatch::slotFilesDeleted( const QStringList& paths )
{
    KUrl::List urls;
    foreach( const QString& path, paths ) {
        urls << KUrl(path);
    }

    kDebug() << urls;

    m_metadataMover->removeFileMetadata( urls );
}


void Nepomuk::FileWatch::slotFileDeleted( const QString& urlString )
{
    slotFilesDeleted( QStringList( urlString ) );
}


void Nepomuk::FileWatch::slotFileCreated( const QString& path )
{
    updateFolderViaStrigi( path );
}


void Nepomuk::FileWatch::slotFileModified( const QString& path )
{
    updateFolderViaStrigi( path );
}


void Nepomuk::FileWatch::slotMovedWithoutData( const QString& path )
{
    updateFolderViaStrigi( path );
}


// static
void Nepomuk::FileWatch::updateFolderViaStrigi( const QString& path )
{
    //
    // Tell Strigi service (if running) to update the newly created
    // folder or the folder containing the newly created file
    //
    org::kde::nepomuk::Strigi strigi( "org.kde.nepomuk.services.nepomukstrigiservice", "/nepomukstrigiservice", QDBusConnection::sessionBus() );
    if ( strigi.isValid() ) {
        QString dirPath;
        QFileInfo info( path );
        if ( !info.exists() )
            return;
        if ( info.isDir() )
            dirPath = info.absoluteFilePath();
        else
            dirPath = info.absolutePath();

        strigi.updateFolder( dirPath, false /* no forced update */ );
    }
}


void Nepomuk::FileWatch::connectToKDirWatch()
{
    // monitor KIO for changes
    QDBusConnection::sessionBus().connect( QString(), QString(), "org.kde.KDirNotify", "FileMoved",
                                           this, SIGNAL( slotFileMoved( const QString&, const QString& ) ) );
    QDBusConnection::sessionBus().connect( QString(), QString(), "org.kde.KDirNotify", "FilesRemoved",
                                           this, SIGNAL( slotFilesDeleted( const QStringList& ) ) );
}


#ifdef BUILD_KINOTIFY
void Nepomuk::FileWatch::slotInotifyWatchUserLimitReached()
{
    // we do it the brutal way for now hoping with new kernels and defaults this will never happen
    delete m_dirWatch;
    m_dirWatch = 0;
    connectToKDirWatch();
}
#endif

#include "nepomukfilewatch.moc"
