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


#include "backupmanager.h"
#include "backupmanageradaptor.h"
#include "logstorage.h"
#include "identifier.h"
#include "tools.h"

#include "changelog.h"
#include "syncfile.h"
#include "identificationset.h"

#include <QtDBus/QDBusConnection>
#include <QtCore/QListIterator>
#include <QtCore/QTimer>
#include <QtCore/QDir>

#include <KDebug>
#include <KStandardDirs>
#include <KConfig>
#include <KConfigGroup>
#include <KDirWatch>

Nepomuk::BackupManager::BackupManager(Nepomuk::Identifier* ident, QObject* parent)
    : QObject( parent ),
      m_identifier( ident ),
      m_config( "nepomukbackuprc" )
{
    new BackupManagerAdaptor( this );
    // Register via DBUs
    QDBusConnection con = QDBusConnection::sessionBus();
    con.registerObject( QLatin1String("/backupmanager"), this );

    m_backupLocation = KStandardDirs::locateLocal( "data", "nepomuk/backupsync/backups/" );

    KDirWatch* dirWatch = KDirWatch::self();
    connect( dirWatch, SIGNAL( dirty( const QString& ) ),
             this, SLOT( slotConfigDirty() ) );
    connect( dirWatch, SIGNAL( created( const QString& ) ),
             this, SLOT( slotConfigDirty() ) );

    dirWatch->addFile( KStandardDirs::locateLocal( "config", m_config.name() ) );

    connect( &m_timer, SIGNAL(timeout()), this, SLOT(automatedBackup()) );
    slotConfigDirty();
}


Nepomuk::BackupManager::~BackupManager()
{
}


void Nepomuk::BackupManager::backup(const QString& oldUrl)
{
    QString url = oldUrl;
    if( url.isEmpty() )
        url = KStandardDirs::locateLocal( "data", "nepomuk/backupsync/backup" ); // default location

    kDebug() << url;

    QFile::remove( url );

    saveBackupSyncFile( url );
    emit backupDone();
}


int Nepomuk::BackupManager::restore(const QString& oldUrl)
{
    //TODO: Some kind of error checking!
    QString url = oldUrl;
    if( url.isEmpty() )
        url = KStandardDirs::locateLocal( "data", "nepomuk/backupsync/backup" );
    
    return m_identifier->process( SyncFile(url) );
}

void Nepomuk::BackupManager::automatedBackup()
{
    QDate today = QDate::currentDate();
    backup( m_backupLocation + today.toString(Qt::ISODate) );

    resetTimer();
    removeOldBackups();
}

void Nepomuk::BackupManager::slotConfigDirty()
{
    // Reparse config
    QString timeString = m_config.group("backup").readEntry<QString>("time", QTime().toString( Qt::ISODate ) );
    m_backupTime = QTime::fromString( timeString, Qt::ISODate );

    m_daysBetweenBackups = m_config.group("backup").readEntry<int>("daysbetweenbackups", 0);
    m_maxBackups = m_config.group("backup").readEntry<int>("maxBackups", 1);

    // Remove old timers and start new
    resetTimer();
    removeOldBackups();
}

void Nepomuk::BackupManager::resetTimer()
{
    if( m_backupTime.isNull() && m_daysBetweenBackups == 0 ) {
        // Never perform automated backups
        return;
    }

    QDateTime current = QDateTime::currentDateTime();
    QDateTime dateTime = current.addDays( m_daysBetweenBackups );
    dateTime.setTime( m_backupTime );

    if( dateTime < current ) {
        dateTime = dateTime.addDays( 1 );
    }
    
    int msecs = current.msecsTo( dateTime );

    m_timer.stop();
    m_timer.start( msecs );
    kDebug() << "Setting timer for " << msecs/1000.0/60/60 << " hours";
}

void Nepomuk::BackupManager::removeOldBackups()
{
    QDir dir( m_backupLocation );
    QStringList infoList = dir.entryList( QDir::Files | QDir::NoDotAndDotDot, QDir::Name );

    while( infoList.size() > m_maxBackups ) {
        const QString backupPath = m_backupLocation + infoList.last();
        kDebug() << "Removing : " << backupPath;
        QFile::remove( backupPath );
        infoList.pop_back();
    }
}



#include "backupmanager.moc"
