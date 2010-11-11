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


#ifndef NEPOMUK_RESOURCEMERGER_H
#define NEPOMUK_RESOURCEMERGER_H

#include <QtCore/QList>
#include <QtCore/QHash>

#include <KUrl>

#include "nepomuksync_export.h"

namespace Soprano {
    class Statement;
    class Model;
    class Graph;
}

namespace Nepomuk {

    class Resource;
    class ResourceManager;

    namespace Types {
        class Class;
    }
    
    namespace Sync {

        /**
         * \class ResourceMerger resourcemerger.h
         *
         * This class can be used to push resources into repository after identification.
         * It's default behavior is to create all the resources that have not been
         * identified.
         *
         * By default, it pushes all the statements with a nrl:InstanceBase graph. If a
         * statement already exists in the repository then it is NOT overwritten.
         * 
         * \author Vishesh Handa <handa.vish@gmail.com>
         */
        class NEPOMUKSYNC_EXPORT ResourceMerger
        {
        public:
            ResourceMerger( ResourceManager * rm = 0 );
            virtual ~ResourceMerger();

            ResourceManager * resourceManager() const;
            void setResourceManager( ResourceManager * rm );

            Soprano::Model * model() const;

            /**
             * Pushes all the statements in \p graph into the model. If the statement
             * already exists then resolveDuplicate() is called.
             *
             * If any of the statements contains a graph, then that graph is used. Otherwise
             * a newly generated graph is used.
             *
             * \sa setGraphType graphType
             */
            virtual void merge( const Soprano::Graph & graph, const QHash<KUrl, Resource> & mappings );

            /**
             * The graph type by default is nrl:InstanceBase. If \p type is not a subclass of
             * nrl:Graph then it is ignored.
             */
            bool setGraphType( const Types::Class & type );
            
            Types::Class graphType() const;

        protected:
            
            /**
             * Called when trying to merge a statement which contains a Resource that
             * has not been identified.
             * 
             * The default implementation of this creates the resource in the main model.
             */
            virtual Resource resolveUnidentifiedResource( const KUrl & uri );

            /**
             * Creates a new graph of type graphType()
             */
            virtual KUrl createGraph();

            /**
             * Push the statement into the Nepomuk repository if it doesn't already exist!
             */
            void push( const Soprano::Statement & st );

        private:
            class Private;
            Private * d;
        };
    }
}
#endif // NEPOMUK_RESOURCEMERGER_H
