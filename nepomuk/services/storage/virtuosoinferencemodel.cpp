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

#include "virtuosoinferencemodel.h"

#include <Soprano/QueryResultIterator>
#include <Soprano/Node>
#include <Soprano/Vocabulary/NRL>

Nepomuk::VirtuosoInferenceModel::VirtuosoInferenceModel(Model *model)
    : Soprano::FilterModel(model)
{
    if(model) {
        createOntologyGraphGroup();
    }
}

Nepomuk::VirtuosoInferenceModel::~VirtuosoInferenceModel()
{
}

void Nepomuk::VirtuosoInferenceModel::setParentModel(Soprano::Model *model)
{
    FilterModel::setParentModel(model);
    if(model) {
        createOntologyGraphGroup();
    }
}

Soprano::QueryResultIterator Nepomuk::VirtuosoInferenceModel::executeQuery(const QString &query, Soprano::Query::QueryLanguage language, const QString &userQueryLanguage) const
{
    if(language == Soprano::Query::QueryLanguageSparqlNoInference) {
        return FilterModel::executeQuery(query, Soprano::Query::QueryLanguageSparql);
    }
    else if(language == Soprano::Query::QueryLanguageSparql) {
        return FilterModel::executeQuery(QLatin1String("DEFINE input:inference <nepomuk:/ontographgroup> ") + query, language);
    }
    else {
        return FilterModel::executeQuery(query, language, userQueryLanguage);
    }
}
#include <KDebug>
void Nepomuk::VirtuosoInferenceModel::updateOntologyGraphs()
{
    // fetch all nrl:Ontology graphs and add them to the graph group
    Soprano::QueryResultIterator it
            = executeQuery(QString::fromLatin1("select distinct ?r where { ?r a %1 . }").arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::Ontology())),
                           Soprano::Query::QueryLanguageSparql);
    while(it.next()) {
        kDebug() << it[0];
        executeQuery(QString::fromLatin1("DB.DBA.RDF_GRAPH_GROUP_INS('nepomuk:/ontographgroup', '%1')").arg(it[0].uri().toString()),
                     Soprano::Query::QueryLanguageUser,
                     QLatin1String("sql"));
    }
}

void Nepomuk::VirtuosoInferenceModel::createOntologyGraphGroup()
{
    // Update ontology graph group
    Soprano::QueryResultIterator it
            = executeQuery(QString::fromLatin1("select RGG_IID from DB.DBA.RDF_GRAPH_GROUP where RGG_IRI='nepomuk:/ontographgroup'"),
                           Soprano::Query::QueryLanguageUser,
                           QLatin1String("sql"));
    if(!it.next()) {
        executeQuery(QLatin1String("DB.DBA.RDF_GRAPH_GROUP_CREATE('nepomuk:/ontographgroup', 1, null, 'The Nepomuk graph group which contains all nrl:Ontology graphs.')"),
                     Soprano::Query::QueryLanguageUser,
                     QLatin1String("sql"));
    }

    // create the rdfs rule graph on the graph group
    executeQuery(QLatin1String("rdfs_rule_set('nepomuk:/ontographgroup','nepomuk:/ontographgroup')"),
                 Soprano::Query::QueryLanguageUser,
                 QLatin1String("sql"));
}

#include "virtuosoinferencemodel.moc"
