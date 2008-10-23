/* This file is part of the KDE Project
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_ONTOLOGY_MANAGER_MODEL_H_
#define _NEPOMUK_ONTOLOGY_MANAGER_MODEL_H_

#include <Soprano/FilterModel>

namespace Nepomuk {
    /**
     * \class OntologyManagerModel ontologymanagermodel.h ontologymanagermodel.h
     *
     * \brief Filter model to manage NRL ontologies.
     *
     * Can be used to manage ontologies stored in a model. The ontologies
     * are stored in NRL graphs.
     *
     * \author Sebastian Trueg <trueg@kde.org>
     */
    class OntologyManagerModel : public Soprano::FilterModel
    {
        Q_OBJECT

    public:
        /**
         * Create a new model.
         */
        OntologyManagerModel( Soprano::Model* parentModel = 0, QObject* parent = 0 );

        /**
         * Destructor
         */
        ~OntologyManagerModel();

        /**
         * Reimplemented from FilterModel. The API is not affected.
         */
        void setParentModel( Soprano::Model* parentModel );
        
        /**
         * Update an ontology.
         *
         * \param data The actual statements defining the ontology. These statements have to 
         * either already define the proper NRL graphs or not define graphs at all. In the 
         * latter case the graphs will be created.
         * \param ns The namespace of the ontology. If this is left invalid it will be
         * determined from the data.
         *
         * \return \p true if the data was valid and the ontology was successfully updated.
         * \false otherwise.
         */
        bool updateOntology( Soprano::StatementIterator data, const QUrl& ns = QUrl() );

        /**
         * Remove an ontology from the model.
         *
         * \param ns The namespace of the ontology.
         *
         * \return \p true if the ontology was found and successfully removed.
         * \p false in case the ontology was not found or an error occurred.
         */
        bool removeOntology( const QUrl& ns );

        /**
         * Determine the modification time of a stored ontology. The
         * modification time of an ontology is the time it was stored into
         * the model.
         *
         * \param uri The namespace of the ontology.
         *
         * \return The modification time of the ontology identified by \p uri
         * or an invalid QDateTime if the ontology was not found.
         */
        QDateTime ontoModificationDate( const QUrl& uri );

        /**
         * Tries to find the ontology \p uri in the local Nepomuk store.
         * \return The context (named graph) storing the ontology's statements
         * or an invalid URI if the ontology could not be found.
         */
        QUrl findOntologyContext( const QUrl& uri );

    private:
        class Private;
        Private* const d;
    };
}

#endif
