/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2007-2010 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_SEARCH_THREAD_H_
#define _NEPOMUK_SEARCH_THREAD_H_

#include <QtCore/QThread>
#include <QtCore/QHash>
#include <QtCore/QPair>

#include <Nepomuk/Query/Result>
#include <Nepomuk/Query/Query>

#include <KUrl>


namespace Soprano {
    class QueryResultIterator;
}

namespace Nepomuk {
    namespace Query {
        class SearchThread : public QThread
        {
            Q_OBJECT

        public:
            SearchThread( QObject* parent = 0 );
            ~SearchThread();

            /**
             * Use instead of QThread::start()
             */
            void query( const QString& query, const RequestPropertyMap& requestProps, double cutOffScore = 0.0 );
            void cancel();

            double cutOffScore() const { return m_cutOffScore; }

        signals:
            void newResult( const Nepomuk::Query::Result& result );

        protected:
            void run();

        private:
            void sparqlQuery( const QString& query, double baseScore );
            Nepomuk::Query::Result extractResult( const Soprano::QueryResultIterator& it ) const;

            QString m_sparqlQuery;
            RequestPropertyMap m_requestProperties;

            double m_cutOffScore;

            // status
            bool m_canceled;
        };
    }
}

#endif
