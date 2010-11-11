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


#ifndef LOGSTORAGE_H
#define LOGSTORAGE_H

#include <QtCore/QObject>
#include <QtCore/QDateTime>

namespace Nepomuk {

    class ChangeLog;
    class ChangeLogRecord;
    class LogStorageHelper;
    
    class LogStorage : public QObject
    {
        Q_OBJECT
        //Q_CLASSINFO("D-Bus Interface", "org.kde.nepomuk.services.nepomukbackupsync.logstorage")
    public:
        static LogStorage * instance();

        ChangeLog getChangeLog( const QString & minDate );
        ChangeLog getChangeLog( const QDateTime & min );
        
        void addRecord( const ChangeLogRecord & record );
        void removeRecords( const QDateTime & lessThan );

        /**
         * Locks the LogStorage at the currentDateTime. All calls to getChangeLog will
         * not returns any ChangeLogRecords whose time stamp >= currentTime.
         * The will last untill the LogStorage is unlocked.
         * 
         * Locks are not recursive. Only the last called lock is valid.
         *
         * The logs are still saved. The only change is that getChangeLog will not return
         * them to you.
         * 
         * \sa unlock getChangeLog
         */
        void lock();

        /**
         * Unlocks the LogStorage. Now all the records will be returned.
         */
        void unlock();

    private:
        friend class Nepomuk::LogStorageHelper;
        
        LogStorage();
        ~LogStorage();
        
        bool saveRecords();
        QList<ChangeLogRecord> m_records;
        
        static const int m_maxFileRecords = 250;
        QString m_dirUrl;

        bool m_locked;
        QDateTime m_lockTime;
    };

}

#endif // LOGSTORAGE_H
