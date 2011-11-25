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
         * \class SyncResource syncresource.h
         *
         * A SyncResource is a convenient way of storing a set of properties and objects for
         * a common subject. This class does not represent an actual resource present in the
         * repository. It's just a collection of in-memory statements.
         *
         * Interally it uses a multi-hash to store the properties and objects.
         *
         * \author Vishesh Handa <handa.vish@gmail.com>
         */
        class NEPOMUKSYNC_EXPORT SyncResource : public QMultiHash<KUrl, Soprano::Node>
        {
        public :
            SyncResource();
            SyncResource( const KUrl & uri );
            SyncResource( const SyncResource & rhs );
            virtual ~SyncResource();

            /**
             * It uses the the first element's subject as the uri and ignores all further subjects.
             * Please make sure all the subjects are the same cause no kind of checks are made.
             */
            static SyncResource fromStatementList(const QList<Soprano::Statement> & list);

            QList<Soprano::Statement> toStatementList() const;

            bool isFileDataObject() const;
            bool isFolder() const;
            KUrl nieUrl() const;

            KUrl uri() const;

            /**
             * If \p node is resource node the uri is set to the node's uri
             * Otherwise if \p node is a blank node then the uri
             * is set to its identifier
             */
            void setUri( const Soprano::Node & node );

            QList<Soprano::Node> property( const KUrl & url ) const;

            /**
             * Removes all the statements whose object is \p uri
             */
            void removeObject( const KUrl & uri );

            SyncResource& operator=( const SyncResource & rhs );
            bool operator==( const SyncResource & res ) const;

            bool isValid() const;
        private:
            class Private;
            QSharedDataPointer<Private> d;
        };

        /**
         * \class ResourceHash syncresource.h
         *
         * A SyncResource is a convenient way of representing a list of Soprano::Statements
         * or a Soprano::Graph.
         * It provides easy lookup of resources.
         *
         * \author Vishesh Handa <handa.vish@gmail.com>
         */
        class NEPOMUKSYNC_EXPORT ResourceHash : public QHash<KUrl, SyncResource> {
        public :
            static ResourceHash fromStatementList( const QList<Soprano::Statement> & list );
            static ResourceHash fromGraph( const Soprano::Graph & graph );

            QList<Soprano::Statement> toStatementList() const;
        };

        uint NEPOMUKSYNC_EXPORT qHash(const SyncResource& res);
    }
}
#endif // NEPOMUK_SIMPLERESOURCEH_H
