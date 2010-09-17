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

#include <QtCore/QRunnable>
#include <QtCore/QPointer>

#include <Nepomuk/Query/Result>

#include "folder.h"


namespace Soprano {
    class QueryResultIterator;
}

namespace Nepomuk {
    namespace Query {
        class Folder;

        class SearchRunnable : public QRunnable
        {
        public:
            SearchRunnable( Folder* folder );
            ~SearchRunnable();

            void cancel();

        Q_SIGNALS:
            void newResult( const Nepomuk::Query::Result& result );

        protected:
            void run();

        private:
            Nepomuk::Query::Result extractResult( const Soprano::QueryResultIterator& it ) const;

            QPointer<Folder> m_folder;

            // status
            bool m_canceled;
            int m_resultCnt;
        };
    }
}

#endif
