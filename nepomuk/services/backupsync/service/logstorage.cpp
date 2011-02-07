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


#include "logstorage.h"

#include <QtCore/QDir>

#include "changelog.h"
#include "changelogrecord.h"

#include <KStandardDirs>
#include <KDebug>

Nepomuk::LogStorage::LogStorage()
{
    m_dirUrl = KStandardDirs::locateLocal( "data", "nepomuk/backupsync/log/" );
    m_locked = false;
}


Nepomuk::LogStorage::~LogStorage()
{
    // Always save all the records on exit
    saveRecords();
}


class Nepomuk::LogStorageHelper
{
public:
    Nepomuk::LogStorage q;
};
K_GLOBAL_STATIC(Nepomuk::LogStorageHelper, instanceHelper)

// static
Nepomuk::LogStorage* Nepomuk::LogStorage::instance()
{
    return &(instanceHelper->q);
}


Nepomuk::ChangeLog Nepomuk::LogStorage::getChangeLog(const QString& minDate)
{
    return getChangeLog( QDateTime::fromString( minDate, ChangeLog::dateTimeFormat() ) );
}

Nepomuk::ChangeLog Nepomuk::LogStorage::getChangeLog(const QDateTime& min)
{
    // Always save all the existing records before accessing them. It's like flushing the cache
    saveRecords();

    ChangeLog log;

    QDir dir( m_dirUrl );
    const QStringList& entries = dir.entryList( QDir::Files, QDir::Name );
    if( entries.empty() ) {
        kDebug() << "No enteries to generate a ChangeLog from";
        return ChangeLog();
    }

    foreach( const QString & fileName, entries ) {
        QDateTime dt = QDateTime::fromString( fileName, ChangeLog::dateTimeFormat() );
        if( dt < min )
            continue;

        //TODO: Optimize : Every record shouldn't be checked. Be smart!
        log += ChangeLog::fromUrl( m_dirUrl + fileName, min );
    }

    //TODO: Optimize this!
    if( m_locked )
        log.removeRecordsAfter( m_lockTime );

    return log;
}


void Nepomuk::LogStorage::addRecord(const Nepomuk::ChangeLogRecord& record)
{
    //kDebug() << record.st();
    m_records.append( record );

    if( m_records.size() >= m_maxFileRecords ) {
        kDebug() << "Saving Records .. " << m_records.size();
        saveRecords();
        m_records.clear();
    }
}


//IMPORTANT: This function doesn't actually remove ALL the records less than min
// This has been done purposely, as otherwise one would need to read the entire contents
// of at least one LogFile.
void Nepomuk::LogStorage::removeRecords(const QDateTime& min)
{
    QDir dir( m_dirUrl );
    const QStringList& entries = dir.entryList( QDir::Files, QDir::Name );

    foreach( const QString fileName, entries ) {
        QDateTime dt = QDateTime::fromString(fileName, ChangeLog::dateTimeFormat());
        if( dt < min ) {
            QFile file( m_dirUrl + fileName );
            file.remove();
        }
    }
}


namespace {

    //TODO: Optimize : Currently O(n)
    int countRecords( const QString & url ) {
        int count = 0;

        QFile file( url );
        file.open( QIODevice::ReadOnly | QIODevice::Text );

        QTextStream in( &file );

        while( !in.atEnd() )
        {
            QString line = in.readLine();
            count++;
        }
        return count;
    }
}


//FIXME: The code is too complicated and POSSIBLY buggy. It's buggy in the way that it makes
// the service usage skyrocket to 50%+ for large amounts of time.
bool Nepomuk::LogStorage::saveRecords()
{
    if( m_records.empty() )
        return false;

    kDebug();

    // Always sort them not mattter what. I don't know why I get weird results otherwise.
    qSort( m_records );

    QDir dir( m_dirUrl );
    const QStringList& entries = dir.entryList( QDir::Files, QDir::Reversed );

    QDateTime max = m_records.last().dateTime();

    // First time
    if( entries.empty() ) {
        QString path = m_dirUrl + max.toString( ChangeLog::dateTimeFormat() );
        ChangeLogRecord::saveRecords( m_records, path );;
        return true;
    }

    QString fileName = entries.first();

    int begin = 0;
    const int end = m_records.size() - 1;

    while( 1 ) {
        QString filePath = m_dirUrl + fileName;

        int alreadyPresent = countRecords( filePath );
        int numRecords = end - begin + 1;
        int availSize = m_maxFileRecords - alreadyPresent;

        int numToSave = availSize > numRecords ? numRecords : availSize;

        if( numToSave > 0 ) {
            ChangeLogRecord::saveRecords( m_records.mid( begin, numToSave ), filePath );
            QString newName = m_records[ begin + numToSave -1 ].dateTime().toString( ChangeLog::dateTimeFormat() );
            QFile file( filePath );
            file.rename( m_dirUrl + newName );

            begin += numToSave;

            if( begin > end )
                break;
        }

        fileName = m_records.last().dateTime().toString( ChangeLog::dateTimeFormat() );
    }

    kDebug() << "Saved!";
    return true;
}


void Nepomuk::LogStorage::lock()
{
    m_locked = true;
    m_lockTime = QDateTime::currentDateTime();
}

void Nepomuk::LogStorage::unlock()
{
    m_locked = false;
}

