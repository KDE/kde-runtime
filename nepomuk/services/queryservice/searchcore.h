/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2007-2009 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_SEARCH_CORE_H_
#define _NEPOMUK_SEARCH_CORE_H_

#include <QtCore/QObject>
#include <QtCore/QHash>

#include <KUrl>

#include <Nepomuk/Query/Result>
#include <Nepomuk/Query/Query>

namespace Nepomuk {
    namespace Query {
        class SearchCore : public QObject
        {
            Q_OBJECT

        public:
            SearchCore( QObject* parent = 0 );
            ~SearchCore();

            double cutOffScore() const;

            QList<Result> lastResults() const;

            bool isActive() const;

        public Q_SLOTS:
            void query( const QString& query, const Nepomuk::Query::RequestPropertyMap& requestProps );

            void cancel();

            /**
             * Default: 0.0
             */
            void setCutOffScore( double score );

        Q_SIGNALS:
            void newResult( const Nepomuk::Query::Result& );
            void scoreChanged( const Nepomuk::Query::Result& );
            void finished();

        private Q_SLOTS:
            void slotNewResult( const Nepomuk::Query::Result& );
            void slotFinished();

        private:
            class Private;
            Private* const d;
        };
    }
}

#endif
