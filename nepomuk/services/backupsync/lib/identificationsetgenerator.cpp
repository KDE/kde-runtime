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

#include "identificationsetgenerator_p.h"

#include <Soprano/Statement>
#include <Soprano/Node>
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>
#include "backupsync.h"

#include <Nepomuk/ResourceManager>

Nepomuk::Sync::IdentificationSetGenerator::IdentificationSetGenerator(const QSet<KUrl>& uniqueUris, Soprano::Model* m, const QSet<KUrl> & ignoreList)
{
    notDone = uniqueUris - ignoreList;
    model = m;
    done = ignoreList;

}

Soprano::QueryResultIterator Nepomuk::Sync::IdentificationSetGenerator::performQuery(const QStringList& uris)
{
    QString query = QString::fromLatin1("select distinct ?r ?p ?o where { ?r ?p ?o. "
                                        "{ ?p %1 %2 .} "
                                        "UNION { ?p %1 %3. } "
                                        "FILTER( ?r in ( %4 ) ) . } ")
    .arg(Soprano::Node::resourceToN3(Soprano::Vocabulary::RDFS::subPropertyOf()),
            Soprano::Node::resourceToN3(Nepomuk::Vocabulary::backupsync::identifyingProperty()),
            Soprano::Node::resourceToN3(Soprano::Vocabulary::RDF::type()),
            uris.join(", "));


    return model->executeQuery(query, Soprano::Query::QueryLanguageSparql);
}

void Nepomuk::Sync::IdentificationSetGenerator::iterate()
{
    QStringList uris;

    QMutableSetIterator<KUrl> iter( notDone );
    while( iter.hasNext() ) {
        const KUrl & uri = iter.next();
        iter.remove();

        done.insert( uri );
        uris.append( Soprano::Node::resourceToN3( uri ) );

        if( uris.size() == maxIterationSize )
            break;
    }

    Soprano::QueryResultIterator it = performQuery( uris );
    while( it.next() ) {
        Soprano::Node sub = it["r"];
        Soprano::Node pred = it["p"];
        Soprano::Node obj = it["o"];

        statements.push_back( Soprano::Statement( sub, pred, obj ) );

        // If the object is also a nepomuk uri, it too needs to be identified.
        const KUrl & objUri = obj.uri();
        if( objUri.url().startsWith("nepomuk:/res/") ) {
            if( !done.contains( objUri ) ) {
                notDone.insert( objUri );
            }
        }
    }
}

QList<Soprano::Statement> Nepomuk::Sync::IdentificationSetGenerator::generate()
{
    done.clear();

    while( !notDone.isEmpty() ) {
        iterate();
    }
    return statements;
}