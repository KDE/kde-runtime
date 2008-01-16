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

#include "ontologyloader.h"

#include <Soprano/Parser>
#include <Soprano/Backend>
#include <Soprano/StorageModel>
#include <Soprano/PluginManager>
#include <Soprano/Global>
#include <Soprano/NodeIterator>
#include <Soprano/StatementIterator>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/XMLSchema>

#include <KDesktopFile>
#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KStandardDirs>

#include <QtCore/QFileInfo>


#define ONTO_FILE_MODIFICATION_DATE "http://nepomuk.kde.org/OntologyLoader/lastModified"


using namespace Soprano;

namespace {
    QUrl createMetadataGraphUri( const QUrl& uri ) {
        QString s( uri.toString() );
        if ( s[s.length()-1] == '#' )
            s[s.length()-1] = '/';
        else if ( s[s.length()-1] != '/' )
            s += '/';
        s += "metadata";
        return QUrl( s );
    }

    bool findGraphUris( Soprano::Model* model, const QUrl& ns, QUrl& dataGraphUri, QUrl& metaDataGraphUri ) {
        QString query = QString( "select ?dg ?mdg where { "
                                 "?dg <%1> \"%2\"^^<%3> . "
                                 "?mdg <%4> ?dg . "
                                 "}" )
                        .arg( Soprano::Vocabulary::NAO::hasDefaultNamespace().toString() )
                        .arg( ns.toString() )
                        .arg( Soprano::Vocabulary::XMLSchema::string().toString() )
                        .arg( Soprano::Vocabulary::NRL::coreGraphMetadataFor().toString() );

        QueryResultIterator it = model->executeQuery( query, Soprano::Query::QueryLanguageSparql );
        if ( it.next() ) {
            metaDataGraphUri = it.binding("mdg").uri();
            dataGraphUri = it.binding("dg").uri();
            return true;
        }
        else {
            return false;
        }
    }
}


class Nepomuk::OntologyLoader::Private
{
public:
    Soprano::Model* model;

    QDateTime ontoModificationDate( const QUrl& ns );
};


QDateTime Nepomuk::OntologyLoader::Private::ontoModificationDate( const QUrl& ns )
{
    QueryResultIterator it = model->executeQuery( QString( "select ?date where { ?onto <%1> \"%2\"^^<%3> . ?onto <%4> ?date . }" )
                                                  .arg( Soprano::Vocabulary::NAO::hasDefaultNamespace().toString() )
                                                  .arg( ns.toString() )
                                                  .arg( Soprano::Vocabulary::XMLSchema::string().toString() )
                                                  .arg( ONTO_FILE_MODIFICATION_DATE ),
                                                  Soprano::Query::QueryLanguageSparql );
    if ( it.next() ) {
        kDebug() << "Found modification date for" << ns << it.binding( "date" ).literal().toDateTime();
        return it.binding( "date" ).literal().toDateTime();
    }
    else {
        return QDateTime();
    }
}


Nepomuk::OntologyLoader::OntologyLoader( Soprano::Model* model, QObject* parent )
    : QObject( parent ),
      d( new Private() )
{
    d->model = model;
}


Nepomuk::OntologyLoader::~OntologyLoader()
{
    delete d;
}


void Nepomuk::OntologyLoader::update()
{
    // update all installed ontologies
    QStringList ontoFiles = KGlobal::dirs()->findAllResources( "data", "nepomuk/ontologies/*.desktop" );
    foreach( QString file, ontoFiles ) {
        updateOntology( file );
    }

    // FIXME: install watches for changed or newly installed ontologies
//    QStringList dirs = KGlobal::dirs()->findDirs( "data", "nepomuk/ontologies" );
}


bool Nepomuk::OntologyLoader::ensureDataLayout( Soprano::Model* tmpModel, const QUrl& ns )
{
    // 1. all statements need to have a proper context set
    StatementIterator it = tmpModel->listStatements();
    while ( it.next() ) {
        if ( !it.current().context().isValid() ) {
            kDebug() << "Invalid data in ontology" << ns << *it;
            return false;
        }
    }

    // 2. make sure we have a proper relation between the data and metadata graphs
    QUrl dataGraphUri, metaDataGraphUri;
    if ( !findGraphUris( tmpModel, ns, dataGraphUri, metaDataGraphUri ) ) {
        kDebug() << "Invalid data in ontology" << ns << "Could not find datagraph and metadatagraph relation.";
        return false;
    }

    return true;
}


void Nepomuk::OntologyLoader::createMetadata( Soprano::Model* tmpModel, const QUrl& ns )
{
    QUrl dataGraphUri( ns );
    dataGraphUri.setFragment( QString() );
    QUrl metaDataGraphUri = createMetadataGraphUri( dataGraphUri );

    // set proper context on all data statements (This is a bit ugly but we cannot iterate and modify at the same time!)
    QList<Statement> allStatements = tmpModel->listStatements().allStatements();
    tmpModel->removeAllStatements();
    foreach( Statement s, allStatements ) {
        s.setContext( dataGraphUri );
        tmpModel->addStatement( s );
    }

    // add the metadata
    tmpModel->addStatement( Soprano::Statement( metaDataGraphUri, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::GraphMetadata(), metaDataGraphUri ) );
    tmpModel->addStatement( Soprano::Statement( metaDataGraphUri, Soprano::Vocabulary::NRL::coreGraphMetadataFor(), dataGraphUri, metaDataGraphUri ) );
    tmpModel->addStatement( Soprano::Statement( dataGraphUri, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::Ontology(), metaDataGraphUri ) );
    tmpModel->addStatement( Soprano::Statement( dataGraphUri, Soprano::Vocabulary::NAO::hasDefaultNamespace(), LiteralValue( ns.toString() ), metaDataGraphUri ) );
}


bool Nepomuk::OntologyLoader::updateOntology( const QString& filename )
{
    KDesktopFile df( filename );


    // only update if the modification date of the ontology file changed (not the desktop file).
    // ------------------------------------
    QFileInfo ontoFileInf( df.readPath() );
    QDateTime ontoLastModified = d->ontoModificationDate( df.readUrl() );
    if ( ontoLastModified >= ontoFileInf.lastModified() ) {
        kDebug() << "Ontology" << df.readUrl() << "up to date.";
        return true;
    }


    // Create temp memory model
    // ------------------------------------
    const Soprano::Backend* backend = Soprano::usedBackend();//Soprano::PluginManager::instance()->discoverBackendByFeatures( Soprano::BackendFeatureStorageMemory );
    if ( !backend ) {
        kDebug() << "No Soprano backend found that can handle memory models!";
        return false;
    }

    Soprano::Model* tmpModel = backend->createModel( BackendSettings() << BackendSetting( Soprano::BackendOptionStorageMemory ) );
    if ( !tmpModel ) {
        kDebug() << "Failed to create temp memory model!";
        return false;
    }


    // Parse the ontology data
    // ------------------------------------
    QUrl baseUri = df.readUrl();
    QString mimeType = df.desktopGroup().readEntry( "MimeType", QString() );
    const Soprano::Parser* parser
        = Soprano::PluginManager::instance()->discoverParserForSerialization( Soprano::mimeTypeToSerialization( mimeType ),
                                                                              mimeType );
    if ( !parser ) {
        kDebug() << "No parser to handle" << df.readName() << "(" << mimeType << ")";
        delete tmpModel;
        return false;
    }

    Soprano::StatementIterator it = parser->parseFile( df.readPath(),
                                                       baseUri,
                                                       Soprano::mimeTypeToSerialization( mimeType ),
                                                       mimeType );
    if ( parser->lastError() ) {
        delete tmpModel;
        return false;
    }

    while ( it.next() ) {
        tmpModel->addStatement( *it );
    }


    // all the data has been read into the temp model
    // now we make sure it has a proper layout (one main and one metadata graph)
    // ------------------------------------
    QList<Node> graphs = tmpModel->listContexts().allNodes();
    if ( graphs.count() == 0 ) {
        // simple: we have to create all data manually
        createMetadata( tmpModel, baseUri );
    }
    else if ( graphs.count() == 2 ) {
        // proper number of graphs. Make sure we have all the necessary information
        if ( !ensureDataLayout( tmpModel, baseUri ) ) {
            delete tmpModel;
            return false;
        }
    }
    else {
        kDebug() << "Invalid data in ontology" << df.readName() << "We need one data and one metadata graph.";
        delete tmpModel;
        return false;
    }


    // store the modification date of the ontology file in the metadata graph and reuse it to know if we have to update
    // ------------------------------------
    QUrl dataGraphUri, metadataGraphUri;
    if ( findGraphUris( tmpModel, baseUri, dataGraphUri, metadataGraphUri ) ) {
        tmpModel->addStatement( Statement( dataGraphUri, QUrl( ONTO_FILE_MODIFICATION_DATE ), LiteralValue( ontoFileInf.lastModified() ), metadataGraphUri ) );

        // now it is time to merge the new data in
        // ------------------------------------
        if ( ontoLastModified.isValid() ) {
            removeOntology( baseUri );
        }

        it = tmpModel->listStatements();
        while ( it.next() ) {
            d->model->addStatement( *it );
        }
        delete tmpModel;

        return true;
    }
    else {
        kDebug() << "BUG! Could not find data and metadata graph URIs! This should not happen!";
        return false;
    }
}


bool Nepomuk::OntologyLoader::removeOntology( const QUrl& ns )
{
    QUrl dataGraphUri, metadataGraphUri;
    if ( findGraphUris( d->model, ns, dataGraphUri, metadataGraphUri ) ) {
        // now removing the ontology is simple
        d->model->removeContext( dataGraphUri );
        d->model->removeContext( metadataGraphUri );
        return true;
    }
    else {
        kDebug() << "Could not find data graph URI for" << ns;
        return false;
    }
}

#include "ontologyloader.moc"
