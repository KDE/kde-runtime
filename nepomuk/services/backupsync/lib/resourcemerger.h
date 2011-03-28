/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010-11  Vishesh Handa <handa.vish@gmail.com>

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
#include <Soprano/Error/ErrorCode>

namespace Soprano {
    class Statement;
    class Model;
    class Graph;
    class Node;
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
        class NEPOMUKSYNC_EXPORT ResourceMerger : public Soprano::Error::ErrorCache
        {
        public:
            ResourceMerger( Soprano::Model * model =0, const QHash<KUrl, KUrl> & mappings = (QHash<KUrl, KUrl>()) );
            virtual ~ResourceMerger();

            void setModel( Soprano::Model * model );
            Soprano::Model * model() const;

            void setMappings( const QHash<KUrl, KUrl> & mappings );
            QHash<KUrl, KUrl> mappings() const;
            
            /**
             * Merges all the statements in \p graph into the model, by calling
             * mergeStatement
             *
             * It stops merging if any of the statements in \p graph fail to merge
             * 
             * \sa mergeStatement
             * \return \c true if merging was successful
             */
            virtual bool merge( const Soprano::Graph & graph );

            /**
             * Merges the statement \p st into the model. If the statement
             * already exists then resolveDuplicate() is called.
             *
             * If any of the statements contains a graph, then that graph is used. Otherwise
             * createGraph() is called which returns a new graph.
             *
             * lastError() is set, if merging fails.
             * 
             * \sa createGraph
             * \return \c true if the merging was sucessful. 
             *         \c false if merging failed
             */
            virtual bool mergeStatement( const Soprano::Statement & st );
            
            /**
             * Sets the graph metadata which will be used to create a graph.
             * 
             * \sa createGraph
             */
            void setAdditionalGraphMetadata( const QMultiHash<QUrl, Soprano::Node>& additionalMetadata );
            
            QMultiHash<QUrl, Soprano::Node> additionalMetadata() const;
            
        protected:
            /**
             * Called when trying to merge a statement which contains a Resource that
             * has not been identified.
             * 
             * The default implementation of this creates the resource in the model. 
             * The resourceUri is generated using createResourceUri.
             * 
             * If the resolution is supposed to fail, this function returns KUrl().
             * The reason why resolution failed should also be set with setError()
             * 
             * \sa createResourceUri
             */
            virtual KUrl resolveUnidentifiedResource( const KUrl & uri );

            /**
             * Creates a new graph with the additional metadata. 
             * All graphs that are created should be a subtype of nrl:Graph
             * 
             * \sa additionalMetadata
             */
            virtual KUrl createGraph();

            /**
             * Push the statement into the Nepomuk repository.
             * If a statement with the same subject, predicate and object already
             * exists in the model, then resolveDuplicate is called.
             *
             * \sa resolveDuplicate
             * \return \c true if pushing the statement was successful
             */
            bool push( const Soprano::Statement & st );

            /**
             * If the statement being pushed already exists this method is called.
             * By default it does nothing which means keeping the old statement
             * 
             * \return \c true if resolution was successful
             *         \c false if resolution failed, and merging and should fail
             */ 
            virtual bool resolveDuplicate( const Soprano::Statement & newSt );

            /**
             * Creates a new resource uri. By default this creates it using the
             * ResourceManager::instace()->generateUniqueUri("res")
             */
            virtual QUrl createResourceUri();

            /**
             * Creates a new graph uri. By default this creates it using the
             * ResourceManager::instace()->generateUniqueUri("ctx")
             */
            virtual QUrl createGraphUri();

            /**
             * Returns the graph that is being used to add new statements.
             * If this graph does not exist it is created using createGraph
             *
             * \sa createGraph
             */
            KUrl graph();

            /**
             * Add the statement in the model. By default it just calls
             * Soprano::Model::addStatement()
             * 
             * \return \c Soprano::Error::ErrorNone if added to model
             */
            virtual Soprano::Error::ErrorCode addStatement( const Soprano::Statement & st );
            Soprano::Error::ErrorCode addStatement( const Soprano::Node& subject, const Soprano::Node& property,
                                                    const Soprano::Node& object, const Soprano::Node& graph );
            
        private:
            class Private;
            Private * d;
        };
    }
}
#endif // NEPOMUK_RESOURCEMERGER_H
