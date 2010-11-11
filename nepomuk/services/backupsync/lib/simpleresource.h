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


#ifndef NEPOMUK_SIMPLERESOURCEH_H
#define NEPOMUK_SIMPLERESOURCEH_H

#include <KUrl>

#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QSharedDataPointer>

#include "nepomuksync_export.h"

namespace Soprano {
    class Node;
    class Statement;
    class Graph;
}

namespace Nepomuk {
    namespace Sync {

        /**
         * \class SimpleResource simpleresource.h
         *
         * A SimpleResource is a convenient way of storing a set of properties and objects for
         * a common subject. This class does not represent an actual resource present in the
         * repository. It's just a collection of in-memory statements.
         *
         * Interally it uses a multi-hash to store the properties and objects.
         * 
         * \author Vishesh Handa <handa.vish@gmail.com>
         */
        class NEPOMUKSYNC_EXPORT SimpleResource : public QMultiHash<KUrl, Soprano::Node>
        {
        public :
            SimpleResource();
            SimpleResource( const SimpleResource & rhs );
            virtual ~SimpleResource();

            /**
             * It uses the the first element's subject as the uri and ignores all further subjects.
             * Please make sure all the subjects are the same cause no kind of checks are made.
             */
            static SimpleResource fromStatementList(const QList<Soprano::Statement> & list);
            
            QList<Soprano::Statement> toStatementList() const;

            bool isFileDataObject() const;
            bool isFolder() const;
            KUrl nieUrl() const;

            KUrl uri() const;
            void setUri( const KUrl & newUri );

            
            QList<Soprano::Node> property( const KUrl & url ) const;

            /**
             * Removes all the statements whose object is \p uri
             */
            void removeObject( const KUrl & uri );

            SimpleResource& operator=( const SimpleResource & rhs );
            bool operator==( const SimpleResource & res );
        private:
            class Private;
            QSharedDataPointer<Private> d;
        };

        /**
         * \class ResourceHash simpleresource.h
         *
         * A SimpleResource is a convenient way of representing a list of Soprano::Statements
         * or a Soprano::Graph.
         * It provides easy lookup of resources.
         *
         * \author Vishesh Handa <handa.vish@gmail.com>
         */
        class NEPOMUKSYNC_EXPORT ResourceHash : public QHash<KUrl, SimpleResource> {
        public :
            static ResourceHash fromStatementList( const QList<Soprano::Statement> & list );
            static ResourceHash fromGraph( const Soprano::Graph & graph );

            QList<Soprano::Statement> toStatementList() const;
        };
        
    }
}
#endif // NEPOMUK_SIMPLERESOURCEH_H
