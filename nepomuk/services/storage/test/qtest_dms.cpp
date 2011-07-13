/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>
   Copyright (C) 2011 Vishesh Handa <handa.vish@gmail.com>

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

#include "qtest_dms.h"

#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/XMLSchema>
#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NMM>
#include <Nepomuk/Vocabulary/NCO>
#include <Nepomuk/Vocabulary/NIE>

#include <Soprano/LiteralValue>

using namespace Soprano::Vocabulary;
using namespace Soprano;
using namespace Nepomuk::Vocabulary;
using namespace Nepomuk;

void Nepomuk::insertOntologies(Soprano::Model* m_model, const QUrl& graph)
{
    m_model->addStatement( graph, RDF::type(), NRL::Ontology(), graph );
    // removeResources depends on type inference
    m_model->addStatement( graph, RDF::type(), NRL::Graph(), graph );

    m_model->addStatement( QUrl("prop:/int"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/int"), RDFS::range(), XMLSchema::xsdInt(), graph );

    m_model->addStatement( QUrl("prop:/int2"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/int2"), RDFS::range(), XMLSchema::xsdInt(), graph );

    m_model->addStatement( QUrl("prop:/int3"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/int3"), RDFS::range(), XMLSchema::xsdInt(), graph );

    m_model->addStatement( QUrl("prop:/int_c1"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/int_c1"), RDFS::range(), XMLSchema::xsdInt(), graph );
    m_model->addStatement( QUrl("prop:/int_c1"), NRL::maxCardinality(), LiteralValue(1), graph );

    m_model->addStatement( QUrl("prop:/string"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/string"), RDFS::range(), XMLSchema::string(), graph );

    m_model->addStatement( QUrl("prop:/res"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/res"), RDFS::range(), RDFS::Resource(), graph );

    m_model->addStatement( QUrl("prop:/res2"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/res2"), RDFS::range(), RDFS::Resource(), graph );

    m_model->addStatement( QUrl("prop:/res3"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/res3"), RDFS::range(), RDFS::Resource(), graph );

    m_model->addStatement( QUrl("prop:/res_c1"), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( QUrl("prop:/res_c1"), RDFS::range(), RDFS::Resource(), graph );
    m_model->addStatement( QUrl("prop:/res_c1"), NRL::maxCardinality(), LiteralValue(1), graph );

    m_model->addStatement( QUrl("class:/typeA"), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( QUrl("class:/typeB"), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( QUrl("class:/typeB"), RDFS::subClassOf(), QUrl("class:/typeA"), graph );

    // properties used all the time
    m_model->addStatement( NAO::identifier(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( RDF::type(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( RDF::type(), RDFS::range(), RDFS::Class(), graph );
    m_model->addStatement( NIE::url(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NIE::url(), RDFS::range(), RDFS::Resource(), graph );


    // some ontology things the ResourceMerger depends on
    m_model->addStatement( RDFS::Class(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( RDFS::Class(), RDFS::subClassOf(), RDFS::Resource(), graph );
    m_model->addStatement( NRL::Graph(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NRL::InstanceBase(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NRL::InstanceBase(), RDFS::subClassOf(), NRL::Graph(), graph );
    m_model->addStatement( NAO::prefLabel(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NAO::prefLabel(), RDFS::range(), RDFS::Literal(), graph );
    m_model->addStatement( NFO::fileName(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NFO::fileName(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NCO::fullname(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NCO::fullname(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NIE::title(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NIE::title(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NAO::created(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NAO::created(), RDFS::range(), XMLSchema::dateTime(), graph );
    m_model->addStatement( NAO::created(), NRL::maxCardinality(), LiteralValue(1), graph );
    m_model->addStatement( NAO::lastModified(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NAO::lastModified(), RDFS::range(), XMLSchema::dateTime(), graph );
    m_model->addStatement( NAO::lastModified(), NRL::maxCardinality(), LiteralValue(1), graph );

    // used in testStoreResources_sameNieUrl
    m_model->addStatement( NAO::numericRating(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NAO::numericRating(), RDFS::range(), XMLSchema::xsdInt(), graph );
    m_model->addStatement( NAO::numericRating(), NRL::maxCardinality(), LiteralValue(1), graph );

    // some ontology things we need in testStoreResources_realLife
    m_model->addStatement( NMM::season(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::season(), RDFS::range(), XMLSchema::xsdInt(), graph );
    m_model->addStatement( NMM::episodeNumber(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::episodeNumber(), RDFS::range(), XMLSchema::xsdInt(), graph );
    m_model->addStatement( NMM::hasEpisode(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::hasEpisode(), RDFS::range(), NMM::TVShow(), graph );
    m_model->addStatement( NIE::description(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NIE::description(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NMM::synopsis(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::synopsis(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NMM::series(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::series(), RDFS::range(), NMM::TVSeries(), graph );
    m_model->addStatement( NIE::title(), RDFS::subClassOf(), QUrl("http://www.semanticdesktop.org/ontologies/2007/08/15/nao#identifyingProperty"), graph );

    // some ontology things we need in testStoreResources_strigiCase
    m_model->addStatement( NMM::performer(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::performer(), RDFS::domain(), NMM::MusicPiece(), graph );
    m_model->addStatement( NMM::performer(), RDFS::range(), NCO::Contact(), graph );
    m_model->addStatement( NMM::musicAlbum(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NMM::musicAlbum(), RDFS::range(), NMM::MusicAlbum(), graph );
    m_model->addStatement( NMM::MusicAlbum(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NMM::TVShow(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NMM::TVSeries(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NMM::MusicPiece(), RDF::type(), RDFS::Class(), graph );

    // used by testStoreResources_duplicates
    m_model->addStatement( NFO::hashAlgorithm(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NFO::hashAlgorithm(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NFO::hashValue(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NFO::hashValue(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NFO::hashValue(), NRL::maxCardinality(), LiteralValue(1), graph );
    m_model->addStatement( NFO::hasHash(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NFO::hasHash(), RDFS::range(), NFO::FileHash(), graph );
    m_model->addStatement( NFO::hasHash(), RDFS::domain(), NFO::FileDataObject(), graph );
    m_model->addStatement( NFO::FileHash(), RDF::type(), RDFS::Resource(), graph );
    m_model->addStatement( NFO::FileHash(), RDF::type(), RDFS::Class(), graph );


    m_model->addStatement( NIE::isPartOf(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NIE::isPartOf(), RDFS::range(), NFO::FileDataObject(), graph );
    m_model->addStatement( NIE::lastModified(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NIE::lastModified(), RDFS::range(), XMLSchema::dateTime(), graph );

    m_model->addStatement( NCO::fullname(), RDF::type(), RDF::Property(), graph );
    m_model->addStatement( NCO::fullname(), RDFS::range(), XMLSchema::string(), graph );
    m_model->addStatement( NCO::fullname(), RDFS::domain(), NCO::Contact(), graph );
    m_model->addStatement( NCO::fullname(), NRL::maxCardinality(), LiteralValue(1), graph );
    m_model->addStatement( NCO::Contact(), RDF::type(), RDFS::Resource(), graph );
    m_model->addStatement( NCO::Contact(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NCO::Contact(), RDFS::subClassOf(), NCO::Role(), graph );
    m_model->addStatement( NCO::Contact(), RDFS::subClassOf(), NAO::Party(), graph );

    m_model->addStatement( NAO::Tag(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NFO::FileDataObject(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NFO::Folder(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NFO::Video(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( NIE::InformationElement(), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( QUrl("class:/typeA"), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( QUrl("class:/typeB"), RDF::type(), RDFS::Class(), graph );
    m_model->addStatement( QUrl("class:/typeC"), RDF::type(), RDFS::Class(), graph );
}
