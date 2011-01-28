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



#ifndef RESOURCEIDENTIFIER_P_H
#define RESOURCEIDENTIFIER_P_H

#include <QtCore/QObject>
#include <QtCore/QMultiHash>
#include <QtCore/QSet>

#include <KUrl>

#include "resourceidentifier.h"
#include "simpleresource.h"
#include <Nepomuk/ResourceManager>


namespace Soprano {
    class Node;
    class Statement;
    class Model;
}

namespace Nepomuk {
    namespace Sync {
        
        class ResourceIdentifier::Private
        {
        public:
            /**
             * Contstructor.
             * It will initialize all pointers with NULL and all values that
             * has a incorrect value with this value.
             */
            Private( ResourceIdentifier * parent );
            void init( ResourceManager * rm );
            
            ResourceIdentifier * q;
            
            Soprano::Model * m_model;
            ResourceManager * m_resourceManger;
            
            /**
             * The main identification hash which maps external ResourceUris
             * with the internal ones
             */
            QHash<KUrl, KUrl> m_hash;
            
            QSet<KUrl> m_notIdentified;
            
            /// Used to store all the identification statements
            ResourceHash m_resourceHash;

            //
            // Properties
            //
            KUrl::List m_vitalProperties;
            KUrl::List m_optionalProperties;
            
            /**
             * This contains all the urls that are being identified, at any moment.
             * It is used to avoid infinite recursion while generating the sparql
             * query.
             */
            QSet<KUrl> m_beingIdentified;
            
            float m_minScore;
            
            //
            // Identification Procedures
            //
            

            bool identify( const KUrl & uri );
            
            /**
             * Checks if the @p oldUri is already in the process of being identified.
             * The function returns false if it is being identified otherwise it
             * returns the value of identify( const KUrl & )
             *
             * \sa identify
             */
            bool queryIdentify( const KUrl & oldUri );
            
            /**
             * Finds the best possible match for \p rs from the internal model. It uses
             * constructIdentificationQuery to create the query
             *
             * \param minScore dictates the min number of properties that should match
             *
             * \sa queryIdentify
             */
            KUrl findMatch( const Nepomuk::Sync::SimpleResource& simpleRes );
        };
    }
}


#endif // RESOURCEIDENTIFIER_P_H
