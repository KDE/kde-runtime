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

#include "ontologyupdatejob.h"

#include <QtCore/QUrl>
#include <QtCore/QThread>
#include <QtCore/QDateTime>

#include <Soprano/Backend>
#include <Soprano/Version>
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

#include <KDebug>


using namespace Soprano;

namespace {
    QUrl createMetadataGraphUri( const QUrl& uri ) {
        QString s( uri.toString() );
        if ( s.endsWith( '#' ) )
            s[s.length()-1] = '/';
        else if ( !s.endsWith( '/' ) )
            s += '/';
        s += "metadata";
        return QUrl( s );
    }

    bool findGraphUris( Soprano::Model* model, const QUrl& ns, QUrl& dataGraphUri, QUrl& metaDataGraphUri ) {
        // We use a FILTER(STR(?ns)...) to support both Soprano 2.3 (with plain literals) and earlier (with only typed ones)
        QString query = QString( "select ?dg ?mdg where { "
                                 "?dg <%1> ?ns . "
                                 "?mdg <%4> ?dg . "
                                 "FILTER(STR(?ns) = \"%2\") . "
                                 "}" )
                        .arg( Soprano::Vocabulary::NAO::hasDefaultNamespace().toString() )
                        .arg( ns.toString() )
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


class Nepomuk::OntologyUpdateJob::Private : public QThread
{
public:
    Private( Soprano::Model* mainModel, OntologyUpdateJob* job )
        : QThread( job ),
          m_model( mainModel ),
          m_job( job ),
          m_success( false ) {
    }

    void run();
    void _k_slotFinished();

    QUrl baseUri;
    Soprano::Model* m_model;

private:
    bool updateOntology();
    bool ensureDataLayout( Soprano::Model* tmpModel, const QUrl& ns );
    void createMetadata( Soprano::Model* tmpModel, const QUrl& ns );
    bool removeOntology( const QUrl& ns );

    OntologyUpdateJob* m_job;
    bool m_success;
};


void Nepomuk::OntologyUpdateJob::Private::run()
{
    m_success = updateOntology();
}


bool Nepomuk::OntologyUpdateJob::Private::ensureDataLayout( Soprano::Model* tmpModel, const QUrl& ns )
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


void Nepomuk::OntologyUpdateJob::Private::createMetadata( Soprano::Model* tmpModel, const QUrl& ns )
{
    Q_ASSERT( ns.isValid() );
    QUrl dataGraphUri( ns );
    dataGraphUri.setFragment( QString() );
    QUrl metaDataGraphUri = createMetadataGraphUri( dataGraphUri );

    // set proper context on all data statements (This is a bit ugly but we cannot iterate and modify at the same time!)
    QList<Statement> allStatements = tmpModel->listStatements().allStatements();
    tmpModel->removeAllStatements();
    foreach( Statement s, allStatements ) { // krazy:exclude=foreach
        s.setContext( dataGraphUri );
        tmpModel->addStatement( s );
    }

    // add the metadata
    tmpModel->addStatement( Soprano::Statement( metaDataGraphUri, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::GraphMetadata(), metaDataGraphUri ) );
    tmpModel->addStatement( Soprano::Statement( metaDataGraphUri, Soprano::Vocabulary::NRL::coreGraphMetadataFor(), dataGraphUri, metaDataGraphUri ) );
    tmpModel->addStatement( Soprano::Statement( dataGraphUri, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::Ontology(), metaDataGraphUri ) );
    tmpModel->addStatement( Soprano::Statement( dataGraphUri, Soprano::Vocabulary::NAO::hasDefaultNamespace(), LiteralValue( ns.toString() ), metaDataGraphUri ) );
}


bool Nepomuk::OntologyUpdateJob::Private::updateOntology()
{
    // Create temp memory model
    // ------------------------------------
    const Soprano::Backend* backend = Soprano::PluginManager::instance()->discoverBackendByFeatures( Soprano::BackendFeatureStorageMemory );
    if ( !backend ) {
        kDebug() << "No Soprano backend found that can handle memory models!";
        return false;
    }

    Soprano::Model* tmpModel = backend->createModel( BackendSettings() << BackendSetting( Soprano::BackendOptionStorageMemory ) );
    if ( !tmpModel ) {
        kDebug() << "Failed to create temp memory model!";
        return false;
    }

    Soprano::StatementIterator it = m_job->data();
    while ( it.next() ) {
        tmpModel->addStatement( *it );
    }

    QUrl ontoUri = baseUri;
    if ( ontoUri.isEmpty() ) {
        it = tmpModel->listStatements();
        if ( it.next() ) {
            ontoUri = it.current().subject().uri();
            if ( !ontoUri.fragment().isEmpty() ) {
                ontoUri.setFragment( QString() );
            }
            else {
                ontoUri = ontoUri.toString().left( ontoUri.toString().lastIndexOf( '/' )+1 );
            }
        }
    }
    if ( ontoUri.isEmpty() ) {
        kDebug() << "Failed to determine ontology URI.";
        return false;
    }

    // all the data has been read into the temp model
    // now we make sure it has a proper layout (one main and one metadata graph)
    // ------------------------------------
    QList<Node> graphs = tmpModel->listContexts().allNodes();
    if ( graphs.count() == 0 ) {
        // simple: we have to create all data manually
        createMetadata( tmpModel, ontoUri );
    }
    else if ( graphs.count() == 2 ) {
        // proper number of graphs. Make sure we have all the necessary information
        if ( !ensureDataLayout( tmpModel, ontoUri ) ) {
            delete tmpModel;
            return false;
        }
    }
    else {
        kDebug() << "Invalid data in ontology" << ontoUri << "We need one data and one metadata graph.";
        delete tmpModel;
        return false;
    }


    // store the modification date of the ontology file in the metadata graph and reuse it to know if we have to update
    // ------------------------------------
    QUrl dataGraphUri, metadataGraphUri;
    if ( findGraphUris( tmpModel, ontoUri, dataGraphUri, metadataGraphUri ) ) {
        tmpModel->addStatement( Statement( dataGraphUri, Soprano::Vocabulary::NAO::lastModified(), LiteralValue( QDateTime::currentDateTime() ), metadataGraphUri ) );

        // now it is time to merge the new data in
        // ------------------------------------
        if ( ontoModificationDate( m_model, ontoUri ).isValid() ) {
            removeOntology( ontoUri );
        }

        it = tmpModel->listStatements();
        while ( it.next() ) {
            m_model->addStatement( *it );
        }
        delete tmpModel;

        kDebug() << "Successfully updated ontology" << ontoUri;
        return true;
    }
    else {
        kDebug() << "BUG! Could not find data and metadata graph URIs! This should not happen!";
        return false;
    }
}


bool Nepomuk::OntologyUpdateJob::Private::removeOntology( const QUrl& ns )
{
    QUrl dataGraphUri, metadataGraphUri;
    if ( findGraphUris( m_model, ns, dataGraphUri, metadataGraphUri ) ) {
        // now removing the ontology is simple
        m_model->removeContext( dataGraphUri );
        m_model->removeContext( metadataGraphUri );
        return true;
    }
    else {
        kDebug() << "Could not find data graph URI for" << ns;
        return false;
    }
}


void Nepomuk::OntologyUpdateJob::Private::_k_slotFinished()
{
    // FIXME: more detailed error code and message
    m_job->setError( m_success ? KJob::NoError : KJob::UserDefinedError );
    emit m_job->emitResult();
}


Nepomuk::OntologyUpdateJob::OntologyUpdateJob( Soprano::Model* mainModel, QObject* parent )
    : KJob( parent ),
      d( new Private( mainModel, this ) )
{
    // FIXME: connect the thread for more information
    connect( d, SIGNAL(finished()), this, SLOT(_k_slotFinished()) );
}


Nepomuk::OntologyUpdateJob::~OntologyUpdateJob()
{
    delete d;
}


void Nepomuk::OntologyUpdateJob::start()
{
    d->start();
}


void Nepomuk::OntologyUpdateJob::setBaseUri( const QUrl& uri )
{
    d->baseUri = uri;
}


Soprano::Model* Nepomuk::OntologyUpdateJob::model() const
{
    return d->m_model;
}


QDateTime Nepomuk::OntologyUpdateJob::ontoModificationDate( Soprano::Model* model, const QUrl& uri )
{
    // We use a FILTER(STR(?ns)...) to support both Soprano 2.3 (with plain literals) and earlier (with only typed ones)
    QString query = QString( "select ?date where { "
                             "?onto <%1> ?ns . "
                             "?onto <%3> ?date . "
                             "FILTER(STR(?ns) = \"%2\") . "
                             "}" )
                    .arg( Soprano::Vocabulary::NAO::hasDefaultNamespace().toString() )
                    .arg( uri.toString() )
                    .arg( Soprano::Vocabulary::NAO::lastModified().toString() );
    QueryResultIterator it = model->executeQuery( query, Soprano::Query::QueryLanguageSparql );
    if ( it.next() ) {
        kDebug() << "Found modification date for" << uri << it.binding( "date" ).literal().toDateTime();
        return it.binding( "date" ).literal().toDateTime();
    }
    else {
        return QDateTime();
    }
}

#include "ontologyupdatejob.moc"
