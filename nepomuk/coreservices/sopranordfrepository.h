/* 
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006 Daniele Galdi <daniele.galdi@gmail.com>
 * Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef SOPRANO_RDFREPOSITORY_SERVICE_H
#define SOPRANO_RDFREPOSITORY_SERVICE_H

#include <QtCore/QString>
#include <QtCore/QList>

#include <knepomuk/services/rdfrepositorypublisher.h>

#include <soprano/soprano.h>

using namespace Nepomuk::Backbone;
using namespace Nepomuk::Services;

namespace Nepomuk {
    namespace CoreServices {
	class SopranoRDFRepository : public Services::RDFRepositoryPublisher
	    {
		Q_OBJECT

	    public:
		SopranoRDFRepository( Soprano::Model *system,
				      QMap<QString, Soprano::Model *> *resolver );

		~SopranoRDFRepository();
 
	    public Q_SLOTS:

		void createRepository( const QString& repositoryId );

		QStringList listRepositoryIds();

		void removeRepository( const QString& repositoryId );

		int getRepositorySize( const QString& graphId );

		int contains( const QString& repositoryId, const Soprano::Statement& statement );

		void addStatement( const QString& repositoryId, const Soprano::Statement& statement );

		void addStatements( const QString& repositoryId, const QList<Soprano::Statement>& statements );

		void removeContext( const QString& repositoryId, const Soprano::Node& context );

		int removeStatement( const QString& repositoryId, const Soprano::Statement& statement );

		int removeStatements( const QString& repositoryId, const QList<Soprano::Statement>& statements );

		int removeAllStatements( const QString& repositoryId, const Soprano::Statement& statement );

		QList<Soprano::Statement> listStatements( const QString& repositoryId, const Soprano::Statement& statement );

		QList<Soprano::Statement> constructSparql( const QString& repositoryId, const QString& query );

		Nepomuk::RDF::QueryResultTable selectSparql( const QString& repositoryId, const QString& query );
	  
		QList<Soprano::Statement> describeSparql( const QString& repositoryId, const QString& query );

		QList<Soprano::Statement> construct( const QString& repositoryId, const QString& query,
							  const QString& querylanguage );
	  
		Nepomuk::RDF::QueryResultTable select( const QString& repositoryId, const QString& query,
						       const QString& querylangauge );

		int queryListStatements( const QString& repositoryId, const Soprano::Statement& statement,
					 int timeoutMSec );

		int queryConstruct( const QString& repositoryId, const QString& query,
				    const QString& querylanguage, int timeoutMSec );

		int querySelect( const QString& repositoryId, const QString& query,
				 const QString& querylanguage, int timeoutMSec );

		int queryConstructSparql( const QString& repositoryId, const QString& query,
					  int timeoutMSec );

		int querySelectSparql( const QString& repositoryId, const QString& query,
				       int timeoutMSec );

		int queryDescribeSparql( const QString& repositoryId, const QString& query,
					 int timeoutMSec );

		int askSparql( const QString& repositoryId, const QString& query );

		QList<Soprano::Statement> fetchListStatementsResults( int queryId, int max );

		QList<Soprano::Statement> fetchConstructResults( int queryId, int max );

		QList<Soprano::Statement> fetchDescribeResults( int queryId, int max );

		RDF::QueryResultTable fetchSelectResults( int queryId, int size );

		void closeQuery( int queryId );

		QStringList supportedQueryLanguages();

		int supportsQueryLanguage( const QString& lang );

		QStringList supportedSerializations();

		int supportsSerialization( const QString& serializationMimeType );

		void addGraph( const QString& repositoryId, const QString& graph,
			       const QString& formatMimetype, const Soprano::Node& context );
	  

		void removeGraph( const QString& repositoryId, const QString& graph,
				  const QString& formatMimetype, const Soprano::Node& context );

		void dumpGraph( const QString& graph );

	    private:
		Soprano::ResultSet executeQuery( const QString& graphId,
						 const QString& query,
						 const QString& queryType );

		/**
		 * Creates a pseudo-index graph for the given statement if the object
		 * is a literal. The literal will be split into single words and for
		 * each word a new index statement is created.
		 */
		QList<Soprano::Statement> buildIndexGraph( const Soprano::Statement& ) const;

		QString createStoragePath( const QString& repositoryId ) const;

		class Private;
		Private *d;
	    };
    }
}

#endif
