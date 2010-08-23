/* This file is part of the KDE Project
   Copyright (c) 2007-2010 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_SERVER_ONTOLOGY_LOADER_H_
#define _NEPOMUK_SERVER_ONTOLOGY_LOADER_H_

#include <QtCore/QObject>
#include <QtCore/QUrl>

namespace Soprano {
    class Model;
}

class KJob;


// Hint: Using QString instead of QUrl for URIs since this API will be exported via DBus and not used otherwise
namespace Nepomuk {
    class OntologyLoader : public QObject
    {
        Q_OBJECT

    public:
        OntologyLoader( Soprano::Model* model, QObject* parent = 0 );
        ~OntologyLoader();

    public Q_SLOTS:
        /**
         * Tries to find the ontology \p uri in the local Nepomuk store.
         * \return The context (named graph) storing the ontology's statements
         * or an invalid URI if the ontology could not be found.
         */
        QString findOntologyContext( const QString& uri );

        /**
         * Update all installed ontologies that changed since the last update.
         */
        void updateLocalOntologies();

        /**
         * Update all installed ontologies, independently of their status.
         */
        void updateAllLocalOntologies();

        /**
         * Try to retrieve an ontology from the web.
         * On success ontologyUpdated will be emitted. If the
         * retrieval failed, ontologyUpdateFailed will be
         * emitted.
         */
        void importOntology( const QString& url );

        /**
         * Creates nao:userVisible properties for all classes and properties
         * and stores them in the kuvo namespace.
         */
        void updateUserVisibility();

    Q_SIGNALS:
        /**
         * Emitted once the update of the ontologies is done.
         * This signal is emited whenever the ontologies change
         * and needed updating.
         */
        void ontologyLoadingFinished( Nepomuk::OntologyLoader* );

        /**
         * Emitted once an ontology has been updated. This holds for both
         * locally installed ontology files (which are read automaticall)
         * and those retrieved from the web via importOntology
         */
        void ontologyUpdated( const QString& uri );

        /**
         * Emitted if updating an ontology failed. This holds for both
         * locally installed ontology files (parsing may fail) and for
         * those imported via importOntology.
         */
        void ontologyUpdateFailed( const QString& uri, const QString& error );

    private Q_SLOTS:
        // a little async updating
        void updateNextOntology();
        void slotGraphRetrieverResult( KJob* job );

    private:
        class Private;
        Private* const d;
    };
}

#endif
