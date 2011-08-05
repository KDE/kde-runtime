/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

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

#include "graphmaintainer.h"

#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/NodeIterator>
#include <Soprano/BindingSet>
#include <Soprano/Node>
#include <Soprano/Statement>
#include <Soprano/Vocabulary/NRL>

#include <KDebug>

#include <QtCore/QTime>


using namespace Soprano::Vocabulary;

Nepomuk::GraphMaintainer::GraphMaintainer(Soprano::Model* model)
    : QThread(model),
      m_model(model),
      m_canceled(false)
{
}

Nepomuk::GraphMaintainer::~GraphMaintainer()
{
    m_canceled = true;
    wait();
}

void Nepomuk::GraphMaintainer::run()
{
    kDebug() << "Running graph maintenance";

#ifndef NDEBUG
    QTime timer;
    timer.start();
#endif

    m_canceled = false;

    //
    // query all empty graphs in batches
    //
    const QString query = QString::fromLatin1("select ?g ?mg where { "
                                              "?g a %1 . "
                                              "OPTIONAL { ?mg %2 ?g . } . "
                                              "FILTER(!bif:exists((select (1) where { graph ?g { ?s ?p ?o . } . }))) . "
                                              "} LIMIT 100")
            .arg(Soprano::Node::resourceToN3(NRL::InstanceBase()),
                 Soprano::Node::resourceToN3(NRL::coreGraphMetadataFor()));

    while(!m_canceled) {
        QList<Soprano::BindingSet> graphs = m_model->executeQuery(query, Soprano::Query::QueryLanguageSparql).allBindings();
        if(graphs.isEmpty()) {
            break;
        }
        foreach(const Soprano::BindingSet& graph, graphs) {
            const Soprano::Node g = graph["g"];
            const Soprano::Node mg = graph["mg"];

            if(m_canceled) {
                break;
            }

            // clear the metadata graph
            if(mg.isResource()) {
                m_model->removeContext(mg);
            }

            // clear out anything that is left in crappy inference graphs
            m_model->removeAllStatements(g, Soprano::Node(), Soprano::Node());

            // wait a bit so we do not hog all CPU.
            msleep(200);
        }
    }

#ifndef NDEBUG
    kDebug() << "Finished graph maintenance Elapsed:" << double(timer.elapsed())/1000.0 << "sec";
#else
    kDebug() << "Finished graph maintenance";
#endif

}

#include "graphmaintainer.moc"
