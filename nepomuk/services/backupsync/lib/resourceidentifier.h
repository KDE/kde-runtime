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


#ifndef NEPOMUK_RESOURCEIDENTIFIER_H
#define NEPOMUK_RESOURCEIDENTIFIER_H

#include <QtCore/QObject>
#include <KUrl>

#include "nepomuksync_export.h"

namespace Soprano {
    class Statement;
    class Graph;
    class Model;
}

namespace Nepomuk {

    class Resource;

    namespace Types {
        class Property;
    }
    
    namespace Sync {

        class SimpleResource;

        /**
         * \class ResourceIdentifier resourceidentifier.h
         *
         * This class is used to identify already existing resources from a set of
         * properties and objects. It identifies the resources on the basis of the
         * identifying statements provided.
         * 
         * \author Vishesh Handa <handa.vish@gmail.com>
         */
        class NEPOMUKSYNC_EXPORT ResourceIdentifier
        {
        public:
            ResourceIdentifier( Soprano::Model * model = 0 );
            virtual ~ResourceIdentifier();

            Soprano::Model * model();
            void setModel( Soprano::Model * model );

            //
            // Processing
            //
            virtual void identifyAll();

            virtual bool identify( const KUrl & uri );

            /**
             * Identifies all the resources present in the \p uriList.
             */
            virtual void identify( const KUrl::List & uriList );
            
            /**
             * This returns true if ALL the external ResourceUris have been identified.
             * If this is false, you should manually identify some of the resources by
             * providing the resource.
             *
             * \sa forceResource
             */
            bool allIdentified() const;

            virtual void addStatement( const Soprano::Statement & st );
            virtual void addStatements( const Soprano::Graph& graph );
            virtual void addStatements( const QList<Soprano::Statement> & stList );
            virtual void addSimpleResource( const SimpleResource & res );
            
            //
            // Getting the info
            //
            /**
             * Returns the detected uri for the given resourceUri.
             * This method usefull only after identifyAll() method was called
             */
            KUrl mappedUri( const KUrl & resourceUri ) const;

            KUrl::List mappedUris() const;
            
            /**
             * Returns mappings of the identified uri
             */
            QHash<KUrl, KUrl> mappings() const;

            /**
             * Returns urls that were not successfully identified
             */
            QSet<KUrl> unidentified() const;

            /**
             * Returns all the statements that are being used to identify \p uri
             */
            Soprano::Graph statements( const KUrl & uri );

            QList<Soprano::Statement> identifyingStatements() const;

            SimpleResource simpleResource( const KUrl & uri );
            //
            // Score
            //
            /**
             * Returns the min % of the number of statements that should match during identification
             * in order for a resource to be successfully identified.
             *
             * Returns a value between [0,1]
             */
            float minScore() const;

            void setMinScore( float score );

            //
            // Property Settings
            //
            /**
             * The property \p prop will be matched during identification, but it will
             * not contribute to the actual score if it cannot be matched.
             */
            void addOptionalProperty( const Types::Property & property );

            void clearOptionalProperties();
            
            KUrl::List optionalProperties() const;

            /**
            * If the property \p prop cannot be matched during identification then the
            * identification for that resource will fail.
            *
            * By default - rdf:type is the only vital property
            */
            void addVitalProperty( const Types::Property & property );

            void clearVitalProperties();
            
            KUrl::List vitalProperties() const;

            //
            // Manual Identification
            //
            /**
            * Used for manual identification when both the old Resource uri, and
            * the new resource Uri is provided.
            *
            * If the resource \p res provided is of type nfo:FileDataObject and has
            * a nie:url. That url will be used to attempt to identify the unidentified
            * resources having a similar nie:url ( same directory or common base directory )
            * It does not perform the actual identification, but changes the internal identification
            * statements so that the unidentified resources may be identified during the next
            * call to identifyAll
            *
            * This method should typically be called after identifyAll has been called at least once.
            */
            void forceResource( const KUrl & oldUri, const Resource & res);

            /**
             * Ignores resourceUri (It will no longer play a part in identification )
             * if @p ignoreSub is true and resourceUri is a Folder, all the sub folders
             * and files are also ignored.
             *
             * This is an expensive method. It will also remove all the statements which
             * contain \p uri as the object.
             */
            bool ignore( const KUrl& uri, bool ignoreSub = false );
            
            //
            // Identification Statement generator
            //
            static Soprano::Graph createIdentifyingStatements( const KUrl::List & uriList );

        private:
            class Private;
            Private * d;

        protected:
            /**
            * Called during identification if there is more than one match for one resource.
            *
            * The default behavior is to return an empty uri, which depicts identification failure
            */
            virtual KUrl duplicateMatch( const KUrl & uri, const QSet<KUrl> & matchedUris, float score );

            /**
             * In case identification fails for \p uri this method would be called. Derived classes
             * can implement their own identification mechanisms over here.
             */
            virtual Nepomuk::Resource additionalIdentification( const KUrl & uri );
        };
    }
}

#endif // NEPOMUK_RESOURCEIDENTIFIER_H
