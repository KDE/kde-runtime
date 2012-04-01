/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2012 Sebastian Trueg <trueg@kde.org>
   
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

#ifndef NEPOMUK_VIRTUOSOINFERENCEMODEL_H
#define NEPOMUK_VIRTUOSOINFERENCEMODEL_H

#include <Soprano/FilterModel>

namespace Nepomuk {
    
    class VirtuosoInferenceModel : public Soprano::FilterModel
    {
        Q_OBJECT

    public:
        VirtuosoInferenceModel(Soprano::Model* model);
        ~VirtuosoInferenceModel();
        
        /**
         * Will already create the graph group to ensure that no errors will be thrown if
         * it is used in executeQuery before a call to updateOntololgyGraphs
         */
        void setParentModel(Model *model);

        /**
         * Reimplemented to add Virtuoso SPARQL extensions for inference to each query and suppeor for Soprano::Query::QueryLanguageSparqlNoInference.
         */
        Soprano::QueryResultIterator executeQuery(const QString &query, Soprano::Query::QueryLanguage language, const QString &userQueryLanguage = QString()) const;

    public slots:
        /**
         * Needs to be called if any ontology changes or a new ontology is imported.
         */
        void updateOntologyGraphs();

    private:
        void createOntologyGraphGroup();
    };
}

#endif // NEPOMUK_VIRTUOSOINFERENCEMODEL_H
