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


#ifndef CHANGELOG_H
#define CHANGELOG_H

#include <QtCore/QUrl>
#include <QtCore/QTextStream>

namespace Soprano {
    class Model;
    class Statement;
}

namespace Nepomuk {

        class ChangeLogRecord;

        /**
         * \class ChangeLog changelog.h
         *
         * Consists of a set of ChangeLogRecords.
         *
         * In order to use the Sync API, a list of changes or a ChangeLog has
         * to be provided. It is advisable to keep the internal records sorted on
         * the basis of their time stamp, but that IS NOT enforced.
         *
         * \sa ChangeLogRecord
         *
         * \author Vishesh Handa <handa.vish@gmail.com>
         */
        class ChangeLog
        {
        public :
            ChangeLog();
            ChangeLog( const ChangeLog & rhs );
            virtual ~ChangeLog();
            
            /**
             * Converts \p st into a list of ChangeLogRecords. The dateTime is set
             * to the current time, and all statements are marked as added.
             */
            static ChangeLog fromList( const QList<Soprano::Statement> & st );

            static ChangeLog fromList( const QList<ChangeLogRecord> & records );

            /**
             * Load all statements from graph \p graphUri in the \p model into a list of
             * ChangeLogRecord. The dateTime is set to the current time, and all
             * statements are marked as added.
             *
             * By default the main model is used.
             */
            static ChangeLog fromGraphUri( const QUrl& graphUri, Soprano::Model * model = 0 );

            /**
             * Load all statements from all contexts in \p graphUrlList into a list of
             * ChangeLogRecord. The dateTime is set to the current time, and all
             * statements are marked as added.
             */
            static ChangeLog fromGraphUriList( const QList< QUrl >& graphUriList, Soprano::Model* model = 0 );

            
            static ChangeLog fromUrl( const QUrl & url );
            static ChangeLog fromUrl( const QUrl & url, const QDateTime & min );

            /**
            * Saves all the internal records in url. The records are saved in plain text form
            */
            bool save( const QUrl & url ) const;

            int size() const;
            bool empty() const;
            void clear();

            virtual void add( const ChangeLogRecord & record );

            void sort();

            QList<ChangeLogRecord> toList() const;

            /**
            * Changes all the added statements to removed and vice versa
            */
            void invert();

            /**
            * Removes all the records whose subject is not present in \p nepomukUris
            */
            void filter( const QSet<QUrl> & nepomukUris );

            /**
             * Concatenates the records held by the two ChangeLogs
             */
            ChangeLog & operator +=( const ChangeLog & log );
            ChangeLog& operator +=( const ChangeLogRecord & record );

            ChangeLog& operator=( const ChangeLog & rhs );
            
            void removeRecordsAfter( const QDateTime& dt );
            void removeRecordsBefore( const QDateTime & dt );

            /**
             * Return uri of all objects and subjects in changelog
             */
            QSet<QUrl> resources() const;
            /**
             * Return uri of all subjects in changelog
             */
            QSet<QUrl> subjects() const;

            // trueg: do we really only have resource objects here? No literals at all?
            /**
             * Return uri of all objects in changelog
             */
            QSet<QUrl> objects() const;

            static QString dateTimeFormat();

        private:
            class Private;
            Private * d;
        };

        QTextStream& operator<<( QTextStream & ts, const ChangeLog & log );
        QDebug operator<<( QDebug debug, const ChangeLog & log );
}

#endif // CHANGELOG_H
