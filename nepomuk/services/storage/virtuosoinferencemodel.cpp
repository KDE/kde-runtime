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
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/XMLSchema>
#include <Soprano/Vocabulary/RDFS>

#include <KDebug>

using namespace Soprano::Vocabulary;

namespace {
    const char* s_typeVisibilityGraph = "nepomuk:/ctx/typevisibility";
}


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

void Nepomuk::VirtuosoInferenceModel::updateOntologyGraphs(bool forced)
{
    int graphGroupMemberCount = 0;
    Soprano::QueryResultIterator it
            = executeQuery(QString::fromLatin1("select count(RGGM_MEMBER_IID) from RDF_GRAPH_GROUP_MEMBER as x, RDF_GRAPH_GROUP as y "
                                               "where x.RGGM_GROUP_IID=y.RGG_IID and y.RGG_IRI='nepomuk:/ontographgroup'"),
                           Soprano::Query::QueryLanguageUser,
                           QLatin1String("sql"));
    if(it.next()) {
        graphGroupMemberCount = it[0].literal().toInt();
    }

    // update the ontology graph group only if something changed (forced) or if it is empty
    if(forced || graphGroupMemberCount <= 0) {
        kDebug() << "Need to update ontology graph group";
        // fetch all nrl:Ontology graphs and add them to the graph group
        Soprano::QueryResultIterator it
                = executeQuery(QString::fromLatin1("select distinct ?r where { ?r a %1 . }").arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::NRL::Ontology())),
                               Soprano::Query::QueryLanguageSparql);
        while(it.next()) {
            executeQuery(QString::fromLatin1("DB.DBA.RDF_GRAPH_GROUP_INS('nepomuk:/ontographgroup', '%1')").arg(it[0].uri().toString()),
                         Soprano::Query::QueryLanguageUser,
                         QLatin1String("sql"));
        }
    }

    // update graph visibility if something has changed or if we never did it
    const QUrl visibilityGraph = QUrl::fromEncoded(s_typeVisibilityGraph);
    if(forced ||
       !executeQuery(QString::fromLatin1("ask where { graph %1 { ?s ?p ?o . } }")
                     .arg(Soprano::Node::resourceToN3(visibilityGraph)),
                     Soprano::Query::QueryLanguageSparql).boolValue()) {
        kDebug() << "Need to update type visibility.";
        updateTypeVisibility();
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

void Nepomuk::VirtuosoInferenceModel::updateTypeVisibility()
{
    const QUrl visibilityGraph = QUrl::fromEncoded(s_typeVisibilityGraph);

    // 1. remove all visibility values we added ourselves
    removeContext(visibilityGraph);

    // 2. make rdfs:Resource non-visible (this is required since with KDE 4.9 we introduced a new
    //    way of visibility handling which relies on types alone rather than visibility values on
    //    resources. Any visible type will make all sub-types visible, too. If rdfs:Resource were
    //    visible everything would be.
    removeAllStatements(RDFS::Resource(), NAO::userVisible(), Soprano::Node());

    // 3. Set each type visible which is not rdfs:Resource and does not have a non-visible parent
    executeQuery(QString::fromLatin1("insert into %1 { "
                                     "?t %2 'true'^^%3 . "
                                     "} where { "
                                     "?t a rdfs:Class . "
                                     "filter not exists { ?tt %2 'false'^^%3 .  ?t rdfs:subClassOf ?tt . } }")
                 .arg(Soprano::Node::resourceToN3(visibilityGraph),
                      Soprano::Node::resourceToN3(NAO::userVisible()),
                      Soprano::Node::resourceToN3(XMLSchema::boolean())),
                 Soprano::Query::QueryLanguageSparql);
}

#include "virtuosoinferencemodel.moc"
