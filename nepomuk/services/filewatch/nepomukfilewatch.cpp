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
#include "metadatamover.h"

#include <QtDBus/QDBusConnection>

#include <KDebug>
#include <KUrl>
#include <KPluginFactory>


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


Nepomuk::FileWatch::FileWatch( QObject* parent, const QList<QVariant>& )
    : Service( parent )
{
    // start the mover thread
    m_metadataMover = new MetadataMover( mainModel(), this );
    m_metadataMover->start();

    // monitor KIO for changes
    QDBusConnection::sessionBus().connect( QString(), QString(), "org.kde.KDirNotify", "FileMoved",
                                           this, SIGNAL( slotFileMoved( const QString&, const QString& ) ) );
    QDBusConnection::sessionBus().connect( QString(), QString(), "org.kde.KDirNotify", "FilesRemoved",
                                           this, SIGNAL( slotFilesDeleted( const QStringList& ) ) );
}


Nepomuk::FileWatch::~FileWatch()
{
    m_metadataMover->stop();
    m_metadataMover->wait();
}


void Nepomuk::FileWatch::moveFileMetadata( const QString& from, const QString& to )
{
    slotFileMoved( from, to );
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

#include "nepomukfilewatch.moc"
