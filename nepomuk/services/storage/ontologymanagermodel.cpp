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

#include "ontologymanagermodel.h"
#include "crappyinferencer.h"

#include <QtCore/QUrl>
#include <QtCore/QDateTime>
#include <QtCore/QScopedPointer>

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
#include <Soprano/Vocabulary/OWL>

#include <KDebug>


using namespace Soprano;



namespace {
    /**
     * Create a uri for an nrl:MetadataGraph.
     * \param uri The uri of the data graph.
     */
    QUrl createMetadataGraphUri( const QUrl& uri ) {
        QString s( uri.toString() );
        if ( s.endsWith( '#' ) )
            s[s.length()-1] = '/';
        else if ( !s.endsWith( '/' ) )
            s += '/';
        s += "metadata";
        return QUrl( s );
    }

    /**
     * Find the graphs an ontology is stored in.
     * \param model The model to search in.
     * \param ns The namespace of the ontology in question.
     * \param dataGraphUri The graph which stores the ontology data (output variable)
     * \param metaDataGraphUri The graph which stores the ontology metadata (output variable)
     *
     * \return \p true if the ontology was found and both dataGraphUri and metaDataGraphUri are filled
     * with proper values.
     */
    bool findGraphUris( Soprano::Model* model, const QUrl& ns, QUrl& dataGraphUri, QUrl& metaDataGraphUri ) {
        // We use a FILTER(STR(?ns)...) to support both Soprano 2.3 (with plain literals) and earlier (with only typed ones)
        QString query = QString( "select ?dg ?mdg where { "
                                 "?dg <%1> ?ns . "
                                 "?mdg <%3> ?dg . "
                                 "FILTER(REGEX(STR(?ns), \"^%2\")) . "
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

    /**
     * Check if the ontology with namespace \p ns has a proper NRL layout in model
     * \p model.
     *
     * An ontology that passes this test can be imported into a production model without
     * any modifications.
     *
     * \return \p true if the necessary NRL graphs are defined, \p false otherwise
     */
    bool ensureDataLayout( Soprano::Model* tmpModel, const QUrl& ns )
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


    /**
     * Try to guess the ontoloy type from the contents of the model:
     * a nrl:Ontology or a nrl:KnowledgeBase or a pure nrl:InstanceBase
     */
    QUrl guessOntologyType( Soprano::Model* tmpModel )
    {
        static QList<QUrl> propertyClasses;
        if ( propertyClasses.isEmpty() )
            propertyClasses << Soprano::Vocabulary::RDFS::Class()
                            << Soprano::Vocabulary::OWL::Class()
                            << Soprano::Vocabulary::RDF::Property()
                            << Soprano::Vocabulary::RDFS::ContainerMembershipProperty()
                            << Soprano::Vocabulary::OWL::ObjectProperty()
                            << Soprano::Vocabulary::OWL::DatatypeProperty()
                            << Soprano::Vocabulary::OWL::AnnotationProperty()
                            << Soprano::Vocabulary::OWL::FunctionalProperty()
                            << Soprano::Vocabulary::OWL::DeprecatedProperty()
                            << Soprano::Vocabulary::OWL::OntologyProperty()
                            << Soprano::Vocabulary::OWL::TransitiveProperty()
                            << Soprano::Vocabulary::OWL::SymmetricProperty()
                            << Soprano::Vocabulary::OWL::InverseFunctionalProperty()
                            << Soprano::Vocabulary::NRL::TransitiveProperty()
                            << Soprano::Vocabulary::NRL::SymmetricProperty()
                            << Soprano::Vocabulary::NRL::AsymmetricProperty()
                            << Soprano::Vocabulary::NRL::InverseFunctionalProperty()
                            << Soprano::Vocabulary::NRL::FunctionalProperty()
                            << Soprano::Vocabulary::NRL::ReflexiveProperty();

        // check for classes and properties
        QStringList classesOrPropertiesSubQueries;
        foreach( const QUrl& uri, propertyClasses ) {
            classesOrPropertiesSubQueries << QString( "?type = <%1>" ).arg( uri.toString() );
        }

        // we cannot use UNION here because redland does not support it!
        bool haveClassesOrProperties = tmpModel->executeQuery( QString( "ask where { "
                                                                        "?r a ?type . "
                                                                        "FILTER(%1) . }" )
                                                               .arg( classesOrPropertiesSubQueries.join( " || " ) ),
                                                               Soprano::Query::QueryLanguageSparql ).boolValue();

        // check for anything that is not a class or property
        classesOrPropertiesSubQueries.clear();
        foreach( const QUrl& uri, propertyClasses ) {
            classesOrPropertiesSubQueries << QString( "?type != <%1>" ).arg( uri.toString() );
        }
        // owl:Ontologys do not have any influce on our descision
        classesOrPropertiesSubQueries << QString( "?type != <%1>" ).arg( Soprano::Vocabulary::OWL::Ontology().toString() );

        bool haveInstances = tmpModel->executeQuery( QString( "ask where { "
                                                              "?r a ?type . "
                                                              "FILTER(%1) . }" )
                                                     .arg( classesOrPropertiesSubQueries.join( " && " ) ),
                                                     Soprano::Query::QueryLanguageSparql ).boolValue();

        if ( haveClassesOrProperties && !haveInstances )
            return Soprano::Vocabulary::NRL::Ontology();
        else if ( !haveClassesOrProperties && haveInstances )
            return Soprano::Vocabulary::NRL::InstanceBase();
        else
            return Soprano::Vocabulary::NRL::KnowledgeBase();
    }


    /**
     * Create the necessary NRL graphs and metadata for an ontology to pass ensureDataLayout.
     *
     * \param tmpModel The model to store everything in
     * \param ns The namespace of the ontology to modify in \p tmpModel
     */
    void createMetadata( Soprano::Model* tmpModel, const QUrl& ns )
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

        QUrl graphType = guessOntologyType( tmpModel );

        kDebug() << "guessed onto type:" << graphType;

        // add the metadata
        tmpModel->addStatement( Soprano::Statement( metaDataGraphUri, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::GraphMetadata(), metaDataGraphUri ) );
        tmpModel->addStatement( Soprano::Statement( metaDataGraphUri, Soprano::Vocabulary::NRL::coreGraphMetadataFor(), dataGraphUri, metaDataGraphUri ) );
        tmpModel->addStatement( Soprano::Statement( dataGraphUri, Soprano::Vocabulary::RDF::type(), graphType, metaDataGraphUri ) );
        if ( graphType == Soprano::Vocabulary::NRL::KnowledgeBase() ) {
            // we do not have inference in Nepomuk yet and this way libnepomuk does not get confused when reading types
            tmpModel->addStatement( Soprano::Statement( dataGraphUri, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::Ontology(), metaDataGraphUri ) );
            tmpModel->addStatement( Soprano::Statement( dataGraphUri, Soprano::Vocabulary::RDF::type(), Soprano::Vocabulary::NRL::InstanceBase(), metaDataGraphUri ) );
        }
#if SOPRANO_IS_VERSION( 2, 2, 67 )
        tmpModel->addStatement( Soprano::Statement( dataGraphUri, Soprano::Vocabulary::NAO::hasDefaultNamespace(), LiteralValue::createPlainLiteral( ns.toString() ), metaDataGraphUri ) );
#else
        tmpModel->addStatement( Soprano::Statement( dataGraphUri, Soprano::Vocabulary::NAO::hasDefaultNamespace(), LiteralValue( ns.toString() ), metaDataGraphUri ) );
#endif
    }
}


class Nepomuk::OntologyManagerModel::Private
{
public:
    Private( OntologyManagerModel* p )
        : q( p ) {
    }

    CrappyInferencer m_inferenceModel;

private:
    OntologyManagerModel* q;
};





Nepomuk::OntologyManagerModel::OntologyManagerModel( Soprano::Model* parentModel, QObject* parent )
    : FilterModel(),
      d( new Private( this ) )
{
    setParent( parent );
    FilterModel::setParentModel( &d->m_inferenceModel );
    setParentModel( parentModel );
}


Nepomuk::OntologyManagerModel::~OntologyManagerModel()
{
    delete d;
}


void Nepomuk::OntologyManagerModel::setParentModel( Soprano::Model* parentModel )
{
    d->m_inferenceModel.setParentModel( parentModel );
}


bool Nepomuk::OntologyManagerModel::updateOntology( Soprano::StatementIterator data, const QUrl& ns )
{
    clearError();

    QTime timer;
    timer.start();

    // Create temp memory model
    // ------------------------------------
    const Soprano::Backend* backend = Soprano::PluginManager::instance()->discoverBackendByFeatures( Soprano::BackendFeatureStorageMemory );
    if ( !backend ) {
        kDebug() << "No Soprano backend found that can handle memory models!";
        setError( "No Soprano backend found that can handle memory models." );
        return false;
    }

    Soprano::Model* tmpModel = backend->createModel( BackendSettings() << BackendSetting( Soprano::BackendOptionStorageMemory ) );
    if ( !tmpModel ) {
        kDebug() << "Failed to create temp memory model!";
        setError( backend->lastError() );
        return false;
    }

    // so we do not have to care about deleting out tmpModel anymore.
    QScopedPointer<Soprano::Model> modelGarbageCollector( tmpModel );

    // import the data into our tmp model
    while ( data.next() ) {
        tmpModel->addStatement( *data );
    }

    QUrl ontoUri = ns;
    if ( ontoUri.isEmpty() ) {
        StatementIterator it = tmpModel->listStatements();
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
        setError( "Failed to determine ontology URI from data." );
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
            setError( "The ontology data contains invalid statements.", Soprano::Error::ErrorInvalidArgument );
            return false;
        }
    }
    else {
        kDebug() << "Invalid data in ontology" << ontoUri << "We need one data and one metadata graph.";
        setError( "The ontology data contains invalid statements.", Soprano::Error::ErrorInvalidArgument );
        return false;
    }


    // store the modification date of the ontology file in the metadata graph and reuse it to know if we have to update
    // ------------------------------------
    QUrl dataGraphUri, metadataGraphUri;
    if ( findGraphUris( tmpModel, ontoUri, dataGraphUri, metadataGraphUri ) ) {
        // remove any modification date data there is
        tmpModel->removeAllStatements( dataGraphUri, Soprano::Vocabulary::NAO::lastModified(), Node() );

        // set the new modification date
        tmpModel->addStatement( dataGraphUri, Soprano::Vocabulary::NAO::lastModified(), LiteralValue( QDateTime::currentDateTime() ), metadataGraphUri );

        // now it is time to merge the new data in
        // ------------------------------------
        if ( ontoModificationDate( ontoUri ).isValid() ) {
            if ( !removeOntology( ontoUri ) ) {
                return false;
            }
        }

        StatementIterator it = tmpModel->listStatements();
        while ( it.next() ) {
            if ( addStatement( *it ) != Error::ErrorNone ) {
                // FIXME: here we should cleanup, but then again, if adding the statement
                // fails, removing will probably also fail. So the only real solution
                // would be a transaction.
                return false;
            }
        }

        kDebug() << "Successfully updated ontology" << ontoUri << QString("(%1ms)").arg(timer.elapsed());
        return true;
    }
    else {
        kDebug() << "BUG! BUG! BUG! BUG! BUG! BUG! Could not find data and metadata graph URIs! This should not happen!";
        return false;
    }
}


bool Nepomuk::OntologyManagerModel::removeOntology( const QUrl& ns )
{
    clearError();

    QUrl dataGraphUri, metadataGraphUri;
    if ( findGraphUris( this, ns, dataGraphUri, metadataGraphUri ) ) {
        // now removing the ontology is simple
        removeContext( dataGraphUri );
        removeContext( metadataGraphUri );
        return true;
    }
    else {
        kDebug() << "Could not find data graph URI for" << ns;
        setError( "Could not find ontology " + ns.toString(), Error::ErrorInvalidArgument );
        return false;
    }
}


QDateTime Nepomuk::OntologyManagerModel::ontoModificationDate( const QUrl& uri )
{
    // We use a FILTER(STR(?ns)...) to support both Soprano 2.3 (with plain literals) and earlier (with only typed ones)
    QString query = QString( "select ?date where { "
                             "?onto <%1> ?ns . "
                             "?onto <%3> ?date . "
                             "FILTER(STR(?ns) = \"%2\") . "
                             "FILTER(DATATYPE(?date) = <%4>) . }" )
                    .arg( Soprano::Vocabulary::NAO::hasDefaultNamespace().toString() )
                    .arg( uri.toString() )
                    .arg( Soprano::Vocabulary::NAO::lastModified().toString() )
                    .arg( Soprano::Vocabulary::XMLSchema::dateTime().toString() );
    QueryResultIterator it = executeQuery( query, Soprano::Query::QueryLanguageSparql );
    if ( it.next() ) {
        kDebug() << "Found modification date for" << uri << it.binding( "date" ).literal().toDateTime();
        return it.binding( "date" ).literal().toDateTime();
    }
    else {
        return QDateTime();
    }
}


QUrl Nepomuk::OntologyManagerModel::findOntologyContext( const QUrl& uri )
{
    QUrl dataGraphUri, metaDataGraphUri;
    if ( findGraphUris( parentModel(), uri, dataGraphUri, metaDataGraphUri ) ) {
        return dataGraphUri;
    }
    else {
        return QUrl();
    }
}

#include "ontologymanagermodel.moc"
