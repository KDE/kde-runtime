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

#ifndef IDENTIFICATIONSET_H
#define IDENTIFICATIONSET_H

#include <QtCore/QUrl>
#include <QtCore/QSet>
#include <QtCore/QMultiHash>
#include <QtCore/QTextStream>
#include <QtCore/QSharedDataPointer>
#include <Nepomuk/ResourceManager>

namespace Soprano {
    class Statement;
    class Model;
}

namespace Nepomuk {
        
        class ChangeLog;
        
        class IdentificationSet
        {
        public :
            IdentificationSet();
            IdentificationSet( const IdentificationSet & rhs );
            virtual ~IdentificationSet();

            /**
             * Loads the IdentificationSet present at the \p url
             *
             * \param url It must be a local file
             */
            static IdentificationSet fromUrl( const QUrl & url );

            /**
             * Parses the contents of the text stream \p ts and returns a IdentificationSet.
             */
            static IdentificationSet fromTextStream( QTextStream& ts );

            /**
             * Creates an IdentificationSet from the ChangeLog \p log. The model \p is queried
             * for all the identification properties for each resource in the \p log.
             *
             * \param log The ChangeLog
             * \param model The model which will be queried for all the identification properties.
             *              By default the Nepomuk mainModel is used.
             * \param ignoreList The resources that shall be ignored ( not considered )
             */
            static IdentificationSet fromChangeLog(const ChangeLog& log, Soprano::Model* model = Nepomuk::ResourceManager::instance()->mainModel(), const QSet<QUrl> & ignoreList = QSet<QUrl>());

            /**
             * Creates an IdentificationSet which only contains the properties for the Resource
             * with resource uri \p resourceUri.
             */
            static IdentificationSet fromResource(const QUrl & resourceUri, Soprano::Model* model = Nepomuk::ResourceManager::instance()->mainModel(), const QSet<QUrl> & ignoreList = QSet<QUrl>());

            /**
             * Creates an IdentificationSet for all the resources present in \p resList
             */
            static IdentificationSet fromResourceList( const QList<QUrl> resList, Soprano::Model* model = Nepomuk::ResourceManager::instance()->mainModel() );
            
            /**
             * Creates an IdentificationSet which contains all the identification properties
             * of all the resources present in the \p log. No model is queried to get these
             * properties. Only the identification properties present in the ChangeLog are
             * used.
             *
             * In order to test which properties are identifyable, the main model is queried.
             */
            static IdentificationSet fromOnlyChangeLog( const ChangeLog & log );

            bool save( const QUrl & url ) const;
            bool save( QTextStream & out ) const;

            QList<Soprano::Statement> toList() const;

            void clear();
            void mergeWith(const IdentificationSet & rhs);
            
            IdentificationSet & operator=( const IdentificationSet & rhs );
            IdentificationSet& operator<<(const IdentificationSet & rhs);

            static void createIdentificationSet( Nepomuk::ChangeLog& log, const QUrl& outputUrl );
        private :
            class Private;
            QSharedDataPointer<Private> d;
        };
}
#endif // IDENTIFICATIONSET_H
