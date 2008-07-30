/* This file is part of the KDE Project
   Copyright (c) 2007 Sebastian Trueg <trueg@kde.org>

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

#include <Nepomuk/Service>
#include <QtCore/QUrl>

namespace Soprano {
    class Model;
}

class KJob;


// Hint: Using QString instead of QUrl for URIs since this API will be exported via DBus and not used otherwise
namespace Nepomuk {
    class OntologyLoader : public Nepomuk::Service
    {
        Q_OBJECT

    public:
        OntologyLoader( QObject* parent = 0, const QList<QVariant>& args = QList<QVariant>() );
        ~OntologyLoader();

    public Q_SLOTS:
        /**
         * Tries to find the ontology \p uri in the local Nepomuk store.
         * \return The context (named graph) storing the ontology's statements
         * or an invalid URI if the ontology could not be found.
         */
        QString findOntologyContext( const QString& uri );

        /**
         * Update all installed ontologies and install dir watches
         * to monitor newly installed and changed ontologies.
         *
         * This should also be called for initialization
         */
        void updateLocalOntologies();

        /**
         * Try to retrieve an ontology from the web.
         * On success ontologyUpdated will be emitted. If the
         * retrieval failed, ontologyUpdateFailed will be
         * emitted.
         */
        void importOntology( const QString& url );

    Q_SIGNALS:
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
