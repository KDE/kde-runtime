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


#ifndef CHANGELOGRECORD_H
#define CHANGELOGRECORD_H

#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtCore/QSharedDataPointer>

#include <Soprano/Statement>
#include <Soprano/Model>

namespace Soprano {
    class Parser;
    class Serializer;
}

namespace Nepomuk {

        /**
        * \class ChangeLogRecord changelogrecord.h
        *
        * This class represents one record of the ChangeLog. Records are in the form -
        * [+/-] [TimeStamp] [Statement]
        *
        * The TimeStamp represents the time the statement was added or removed.
        * The format for the timestamp is "yyyy-MM-ddThh:mm:ss.zzz"
        * The ISODateTime format is NOT used because that format does not contain msecs.
        *
        * \sa ChangeLog
        *
        * \author Vishesh Handa <handa.vish@gmail.com>
        */
        class ChangeLogRecord
        {
        public :
            ChangeLogRecord();
            ~ChangeLogRecord();
            ChangeLogRecord( const ChangeLogRecord & rhs );
            ChangeLogRecord( const Soprano::Statement& statement );
            ChangeLogRecord( const QDateTime & dt, bool added, const Soprano::Statement & st );
            ChangeLogRecord( QString & string );

            ChangeLogRecord& operator=( const ChangeLogRecord & rhs );

            QString toString() const;

            bool operator < ( const ChangeLogRecord & rhs ) const;
            bool operator > ( const ChangeLogRecord & rhs ) const;
            bool operator ==( const ChangeLogRecord & rhs ) const;

            QDateTime dateTime() const;
            bool added() const;
            const Soprano::Statement& st() const;
            Soprano::Node subject() const;
            Soprano::Node predicate() const;
            Soprano::Node object() const;

            void setDateTime( const QDateTime & dt );
            void setAdded( bool add = true );
            void setRemoved();

            void setSubject( const Soprano::Node& subject );
            void setPredicate( const Soprano::Node& predicate );
            void setObject( const Soprano::Node & obj );
            void setContext( const Soprano::Node& context );

            // trueg: toXXX methods are typically non-static methods that convert the object in question
            //        while fromXXX methods are the static ones.
            /**
             * Converts the statement list \p sts into a list of ChangeLogRecords. Each record
             * is given a timestamp of the currentTime and is marked as added.
             */
            static QList<ChangeLogRecord> toRecordList( const QList<Soprano::Statement>& stList );

            static QList< ChangeLogRecord> toRecordList( const QUrl & contextUrl, Soprano::Model * model );
            static QList< ChangeLogRecord> toRecordList( const QList< QUrl >& contextUrlList, Soprano::Model* model );

            /**
             * Saves the records in a text file at \p url. The \p records are stored in this format -
             * [+/-] [TimeStamp] [Statement]
             * The Statement is serialized as NQuad.
             */
            static bool saveRecords( const QList<Nepomuk::ChangeLogRecord>& records, const QUrl& url );

            /**
             * Loads all the records from \p url whose dateTime >= \p min
             */
            static QList<ChangeLogRecord> loadRecords( const QUrl& url, const QDateTime& min );
            static QList<ChangeLogRecord> loadRecords( const QUrl& url );

            /**
             * This is the dateTimeFormat used to convert strings into QDateTime and vice-versa. It
             * should be used everywhere when dealing with ChangeLogRecords.
             */
            static QString dateTimeFormat();

        private:
            class Private;
            QSharedDataPointer<Private> d;
        };

        QTextStream & operator<<( QTextStream & ts, const ChangeLogRecord & record );
}
#endif // CHANGELOGRECORD_H
