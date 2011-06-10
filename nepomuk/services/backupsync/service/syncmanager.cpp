/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "syncmanager.h"
#include "identifier.h"
#include "syncfile.h"
#include "changelog.h"
#include "diffgenerator.h"
#include "syncmanageradaptor.h"
#include "tools.h"
#include "logstorage.h"

#include <QtDBus/QDBusConnection>

#include <Soprano/Vocabulary/NAO>

#include <Nepomuk/Resource>
#include <Nepomuk/Variant>

Nepomuk::SyncManager::SyncManager(Nepomuk::Identifier* ident, QObject* parent)
    : QObject( parent ),
      m_identifier( ident )
{
    new SyncManagerAdaptor( this );
    // Register DBus Object
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject( QLatin1String("/syncmanager"), this );
}


Nepomuk::SyncManager::~SyncManager()
{
}


int Nepomuk::SyncManager::sync(const QString& url)
{
    return sync( QUrl( url ) );
}


int Nepomuk::SyncManager::sync(const QUrl& url)
{
    SyncFile syncFile( url );
    return m_identifier->process( syncFile );
}


void Nepomuk::SyncManager::createSyncFile( const QString& url, const QString& startTime )
{
    ChangeLog logFile = LogStorage::instance()->getChangeLog( startTime );

    SyncFile sf( logFile, ResourceManager::instance()->mainModel() );
    sf.save( url );
}


void Nepomuk::SyncManager::createSyncFile( const QUrl& outputUrl, const QList<QString>& urls)
{
    QSet<QUrl> nepomukUris;
    QDateTime min = QDateTime::currentDateTime();

    foreach( const QString & urlString, urls ) {
        QUrl url( urlString );

        Resource res( url );
        QDateTime dt = res.property( Soprano::Vocabulary::NAO::created() ).toDateTime();
        if( dt < min )
            min = dt;

        nepomukUris.insert( res.resourceUri() );
    }

    createSyncFile( outputUrl, nepomukUris, min );
}


void Nepomuk::SyncManager::createSyncFile(const QUrl& outputUrl, QSet<QUrl> & nepomukUris, const QDateTime& min)
{
    ChangeLog logFile = LogStorage::instance()->getChangeLog( min );

    QList<ChangeLogRecord> records = logFile.toList();
    foreach( const ChangeLogRecord & r, records ) {
        const QUrl & objectUri = r.st().object().uri();
        if( nepomukUris.contains( objectUri ) ) {
            if( objectUri.scheme() == QLatin1String("nepomuk") ) {
                nepomukUris.insert( objectUri );
            }
        }
    }

    logFile.filter( nepomukUris );

    SyncFile sf( logFile, ResourceManager::instance()->mainModel() );
    sf.save( outputUrl );
}


void Nepomuk::SyncManager::createFirstSyncFile(const QUrl& outputUrl) const
{
    Q_UNUSED(outputUrl);
    //SyncFile sf = Nepomuk::createFirstSyncFile();
    //sf.save( outputUrl );
}


#include "syncmanager.moc"
