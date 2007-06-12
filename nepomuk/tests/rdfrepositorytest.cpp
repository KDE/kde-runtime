/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "rdfrepositorytest.h"

#include <nepomuk/rdfrepository.h>
#include <soprano/statement.h>
#include <krandom.h>

#define KNEP_VERIFY \
if ( !repository()->success() ) { \
    qDebug() << repository()->lastErrorName() << " - " << repository()->lastErrorMessage(); \
} \
QVERIFY( repository()->success() );

Q_DECLARE_METATYPE( Soprano::Statement )
Q_DECLARE_METATYPE( QList<Soprano::Statement> )

using namespace Soprano;
using namespace Nepomuk::Services;
using namespace Nepomuk::RDF;


QList<Statement> RdfRepositoryTest::createTestData( const Statement& s,
						    int num )
{
  QList<Statement> sl;
  for( int i = 0; i < num; ++i ) {
    sl.append( Statement( s.subject().isEmpty() ? Node( randomUri() ) : s.subject(),
			  s.predicate().isEmpty() ? Node( randomUri() ) : s.predicate(),
			  s.object().isEmpty() ? Node( randomUri() ) : s.object(),
			  s.context() ) );
  }
  return sl;
}


void RdfRepositoryTest::testCreateRepository()
{
    repository()->createRepository( "Testi" );
    KNEP_VERIFY
    QVERIFY( repository()->listRepositoryIds().contains( "Testi" ) );
    KNEP_VERIFY
    repository()->removeRepository( "Testi" );
    KNEP_VERIFY
}


void RdfRepositoryTest::testListRepositoriyIds()
{
    QStringList oldIds = repository()->listRepositoryIds();
    qDebug() << "Old rep ids: " << oldIds;
    repository()->createRepository( "Testi1" );
    KNEP_VERIFY
    repository()->createRepository( "Testi2" );
    KNEP_VERIFY
    repository()->createRepository( "Testi3" );
    KNEP_VERIFY
    repository()->createRepository( "Testi4" );
    KNEP_VERIFY

    QStringList ids = repository()->listRepositoryIds();
    qDebug() << "New rep ids: " << oldIds;
    KNEP_VERIFY
    QCOMPARE( ids.count(), 4 + oldIds.count() );
    QVERIFY( ids.contains( "Testi1" ) );
    QVERIFY( ids.contains( "Testi2" ) );
    QVERIFY( ids.contains( "Testi3" ) );
    QVERIFY( ids.contains( "Testi4" ) );

    repository()->removeRepository( "Testi1" );
    KNEP_VERIFY
    repository()->removeRepository( "Testi2" );
    KNEP_VERIFY
    repository()->removeRepository( "Testi3" );
    KNEP_VERIFY
    repository()->removeRepository( "Testi4" );
    KNEP_VERIFY
}


void RdfRepositoryTest::testRemoveRepository()
{
    repository()->createRepository( "Testi" );
    KNEP_VERIFY
    repository()->removeRepository( "Testi" );
    KNEP_VERIFY
    QVERIFY( !repository()->listRepositoryIds().contains( "Testi" ) );
}


void RdfRepositoryTest::testGetRepositorySize()
{
}


void RdfRepositoryTest::testContains()
{
    QList<Statement> sl = createTestData( Statement(), 10 );
    repository()->addStatements( testRepository(), sl );
    KNEP_VERIFY

    // test contains with full statement
    foreach( Statement s, sl ) {
        QVERIFY( repository()->contains( testRepository(), s ) );
    }

    // test contains with partial statement
    foreach( Statement s, sl ) {
        QVERIFY( repository()->contains( testRepository(), Statement( s.subject(), Node(), Node(), Node() ) ) );
        QVERIFY( repository()->contains( testRepository(), Statement( Node(), s.predicate(), Node(), Node() ) ) );
        QVERIFY( repository()->contains( testRepository(), Statement( Node(), Node(), s.object(), Node() ) ) );

        QVERIFY( repository()->contains( testRepository(), Statement( s.subject(), s.predicate(), Node(), Node() ) ) );
        QVERIFY( repository()->contains( testRepository(), Statement( s.subject(), Node(), s.object(), Node() ) ) );
        QVERIFY( repository()->contains( testRepository(), Statement( Node(), s.predicate(), s.object(), Node() ) ) );

        QVERIFY( repository()->contains( testRepository(), Statement( s.subject(), Node(), Node(), s.context() ) ) );
        QVERIFY( repository()->contains( testRepository(), Statement( Node(), s.predicate(), Node(), s.context() ) ) );
        QVERIFY( repository()->contains( testRepository(), Statement( Node(), Node(), s.object(), s.context() ) ) );

        QVERIFY( repository()->contains( testRepository(), Statement( s.subject(), s.predicate(), Node(), s.context() ) ) );
        QVERIFY( repository()->contains( testRepository(), Statement( s.subject(), Node(), s.object(), s.context() ) ) );
        QVERIFY( repository()->contains( testRepository(), Statement( Node(), s.predicate(), s.object(), s.context() ) ) );
    }

    // test non-exisisting statements
    Statement s;
    do {
        s = createTestData( Statement(), 1 ).first();
    } while ( sl.contains( s ) );

    QVERIFY( !repository()->contains( testRepository(), s ) );

    repository()->removeStatements( testRepository(), sl );
    KNEP_VERIFY
}


void RdfRepositoryTest::testAddStatement()
{
    Statement s = createTestData( Statement(), 1 ).first();
    repository()->addStatement( testRepository(), s );
    KNEP_VERIFY

    QVERIFY( repository()->contains( testRepository(), s ) );

    repository()->removeStatement( testRepository(), s );
    KNEP_VERIFY
}


void RdfRepositoryTest::testAddStatements()
{
    QList<Statement> sl = createTestData( Statement(), 10 );
    repository()->addStatements( testRepository(), sl );
    KNEP_VERIFY

    foreach( Statement s, sl ) {
        QVERIFY( repository()->contains( testRepository(), s ) );
    }

    repository()->removeStatements( testRepository(), sl );
    KNEP_VERIFY
}


void RdfRepositoryTest::testRemoveContext()
{
    QUrl context1 = randomUri();
    QUrl context2 = randomUri();
    QList<Statement> sl1 = createTestData( Statement( Node(), Node(), Node(), context1 ), 10 );
    QList<Statement> sl2 = createTestData( Statement( Node(), Node(), Node(), context2 ), 10 );

    repository()->addStatements( testRepository(), sl1 );
    KNEP_VERIFY
    repository()->addStatements( testRepository(), sl2 );
    KNEP_VERIFY

    repository()->removeContext( testRepository(), context1 );
    KNEP_VERIFY

    foreach( Statement s, sl1 ) {
        QVERIFY( !repository()->contains( testRepository(), s ) );
    }
    foreach( Statement s, sl2 ) {
        QVERIFY( repository()->contains( testRepository(), s ) );
    }

    repository()->removeContext( testRepository(), context2 );
    KNEP_VERIFY
}


void RdfRepositoryTest::testRemoveStatement()
{
    QList<Statement> sl = createTestData( Statement(), 2 );
    repository()->addStatements( testRepository(), sl );
    KNEP_VERIFY

    repository()->removeStatement( testRepository(), sl[0] );
    KNEP_VERIFY

    QVERIFY( !repository()->contains( testRepository(), sl[0] ) );
    QVERIFY( repository()->contains( testRepository(), sl[1] ) );

    repository()->removeStatement( testRepository(), sl[1] );
    KNEP_VERIFY
}


void RdfRepositoryTest::testRemoveStatements()
{
    QList<Statement> sl1 = createTestData( Statement(), 10 );
    QList<Statement> sl2 = createTestData( Statement(), 10 );

    repository()->addStatements( testRepository(), sl1 );
    KNEP_VERIFY
    repository()->addStatements( testRepository(), sl2 );
    KNEP_VERIFY

    repository()->removeStatements( testRepository(), sl1 );
    KNEP_VERIFY

    foreach( Statement s, sl1 ) {
        QVERIFY( !repository()->contains( testRepository(), s ) );
    }
    foreach( Statement s, sl2 ) {
        QVERIFY( repository()->contains( testRepository(), s ) );
    }

    repository()->removeStatements( testRepository(), sl2 );
    KNEP_VERIFY
}


void RdfRepositoryTest::testRemoveAllStatements()
{
    QUrl url1 = randomUri();
    QUrl url2 = randomUri();
    QList<Statement> sl1 = createTestData( Statement( url1, Node(), Node() ), 10 );
    QList<Statement> sl2 = createTestData( Statement( url2, Node(), Node() ), 10 );

    repository()->addStatements( testRepository(), sl1 );
    KNEP_VERIFY
    repository()->addStatements( testRepository(), sl2 );
    KNEP_VERIFY

    repository()->removeAllStatements( testRepository(), Statement( url1, Node(), Node() ) );
    KNEP_VERIFY

    foreach( Statement s, sl1 ) {
        QVERIFY( !repository()->contains( testRepository(), s ) );
    }
    foreach( Statement s, sl2 ) {
        QVERIFY( repository()->contains( testRepository(), s ) );
    }

    repository()->removeStatements( testRepository(), sl2 );
    KNEP_VERIFY


    // test context support
    sl1 = createTestData( Statement( Node(), Node(), Node(), url1 ) );
    sl2 = createTestData( Statement( Node(), Node(), Node(), url2 ) );

    repository()->addStatements( testRepository(), sl1 );
    KNEP_VERIFY
    repository()->addStatements( testRepository(), sl2 );
    KNEP_VERIFY

    repository()->removeAllStatements( testRepository(), Statement( Node(), Node(), Node(), url1 ) );
    KNEP_VERIFY

    foreach( Statement s, sl1 ) {
        QVERIFY( !repository()->contains( testRepository(), s ) );
    }
    foreach( Statement s, sl2 ) {
        QVERIFY( repository()->contains( testRepository(), s ) );
    }
}


void RdfRepositoryTest::testListStatements()
{
    QUrl url1 = randomUri();
    QUrl url2 = randomUri();
    QList<Statement> sl1 = createTestData( Statement( url1, Node(), Node() ), 10 );
    QList<Statement> sl2 = createTestData( Statement( url2, Node(), Node() ), 10 );

    repository()->addStatements( testRepository(), sl1 );
    KNEP_VERIFY
    repository()->addStatements( testRepository(), sl2 );
    KNEP_VERIFY

    QList<Statement> sl1_1 = repository()->listStatements( testRepository(), Statement( url1, Node(), Node() ) );
    QVERIFY( compareLists( sl1, sl1_1 ) );

    repository()->removeStatements( testRepository(), sl1 );
    repository()->removeStatements( testRepository(), sl2 );
}


void RdfRepositoryTest::testConstructSparql()
{
    QList<Statement> sl = createTestData( Statement(), 10 );
    repository()->addStatements( testRepository(), sl );
    KNEP_VERIFY

    QString query = "construct { ?s ?p ?o } where { ?s ?p ?o . }";
    QList<Statement> sl2 = repository()->constructSparql( testRepository(), query );
    KNEP_VERIFY

    QCOMPARE( 10, sl2.count() );
    QVERIFY( compareLists( sl, sl2 ) );
}


void RdfRepositoryTest::testSelectSparql()
{
    QList<Statement> sl = createTestData( Statement(), 10 );
    repository()->addStatements( testRepository(), sl );
    KNEP_VERIFY

    QString query = "select ?s where { ?s ?p ?o . }";
    QueryResultTable result = repository()->selectSparql( testRepository(), query );
    KNEP_VERIFY

    QCOMPARE( 1, result.columns.count() );
    QCOMPARE( QString( "s" ), result.columns[0] );

    QCOMPARE( 10, result.rows.count() );
    foreach( Statement s, sl ) {
        bool found = false;
        foreach( QList<Node> row, result.rows ) {
            if ( s.subject() == row.first() ) {
                found = true;
                break;
            }
        }
        QVERIFY( found );
    }
}


void RdfRepositoryTest::testDescribeSparql()
{
    // FIXME: implement the testDescribeSparql test
}


void RdfRepositoryTest::testConstruct()
{
    // the sparql test should be sufficient
}


void RdfRepositoryTest::testSelect()
{
    // the sparql test should be sufficient
}


void RdfRepositoryTest::testQueryListStatements()
{
    QUrl url1 = randomUri();
    QUrl url2 = randomUri();
    QList<Statement> sl1 = createTestData( Statement( url1, Node(), Node() ), 10 );
    QList<Statement> sl2 = createTestData( Statement( url2, Node(), Node() ), 10 );

    repository()->addStatements( testRepository(), sl1 );
    KNEP_VERIFY
    repository()->addStatements( testRepository(), sl2 );
    KNEP_VERIFY

    int id = repository()->queryListStatements( testRepository(), Statement( url1, Node(), Node() ), 0 );

    QList<Statement> sl1_1;
    QList<Statement> sl1_1_1 = repository()->fetchListStatementsResults( id, 5 );
    KNEP_VERIFY
    sl1_1 += sl1_1_1;

    QCOMPARE( 5, sl1_1_1.count() );

    sl1_1_1 = repository()->fetchListStatementsResults( id, 10 );
    KNEP_VERIFY
    sl1_1 += sl1_1_1;

    QCOMPARE( 5, sl1_1_1.count() );

    QVERIFY( compareLists( sl1, sl1_1 ) );
}


void RdfRepositoryTest::testQueryConstruct()
{
    // the sparql test should be enough for now
}


void RdfRepositoryTest::testQuerySelect()
{
    // the sparql test should be enough for now
}


void RdfRepositoryTest::testQueryConstructSparql()
{
    QList<Statement> sl = createTestData( Statement(), 10 );
    repository()->addStatements( testRepository(), sl );
    KNEP_VERIFY

    QString query = "construct { ?s ?p ?o } where { ?s ?p ?o . }";
    int id = repository()->queryConstructSparql( testRepository(), query, 0 );
    KNEP_VERIFY

    QList<Statement> sl1_1;
    QList<Statement> sl1_1_1 = repository()->fetchConstructResults( id, 5 );
    KNEP_VERIFY
    sl1_1 += sl1_1_1;

    QCOMPARE( 5, sl1_1_1.count() );

    sl1_1_1 = repository()->fetchConstructResults( id, 10 );
    KNEP_VERIFY
    sl1_1 += sl1_1_1;

    QCOMPARE( 5, sl1_1_1.count() );

    QVERIFY( compareLists( sl, sl1_1 ) );
}


void RdfRepositoryTest::testQuerySelectSparql()
{
    QList<Statement> sl = createTestData( Statement(), 10 );
    repository()->addStatements( testRepository(), sl );
    KNEP_VERIFY

    QString query = "select ?s where { ?s ?p ?o . }";
    int id = repository()->querySelectSparql( testRepository(), query, 0 );
    KNEP_VERIFY

    QueryResultTable result = repository()->fetchSelectResults( id, 5 );
    KNEP_VERIFY

    QCOMPARE( result.columns.count(), 1 );
    QCOMPARE( QString( "s" ), result.columns[0] );

    QCOMPARE( result.rows.count(), 5 );
    QList<QList<Node> > rows = result.rows;

    result = repository()->fetchSelectResults( id, 10 );
    KNEP_VERIFY

    QCOMPARE( result.rows.count(), 5 );
    rows += result.rows;

    foreach( Statement s, sl ) {
        bool found = false;
        foreach( QList<Node> row, rows ) {
            if ( s.subject() == row.first() ) {
                found = true;
                break;
            }
        }
        QVERIFY( found );
    }
}


void RdfRepositoryTest::testQueryDescribeSparql()
{
    // FIXME: implement the testQueryDescribeSparql test
}


void RdfRepositoryTest::testAskSparql()
{
    QUrl url1 = randomUri();
    QUrl url2 = randomUri();

    repository()->addStatement( testRepository(), Statement( randomUri(), url1, randomUri() ) );

    QString query1 = QString( "ASK {?a <%1> ?c}" ).arg( url1.toString() );
    QString query2 = QString( "ASK {?a <%1> ?c}" ).arg( url2.toString() );

    QVERIFY( repository()->askSparql( testRepository(), query1 ) );
    KNEP_VERIFY

    QVERIFY( !repository()->askSparql( testRepository(), query2 ) );
    KNEP_VERIFY
}


void RdfRepositoryTest::testFetchListStatementsResults()
{
    // tested above
}


void RdfRepositoryTest::testFetchConstructResults()
{
    // tested above
}


void RdfRepositoryTest::testFetchDescribeResults()
{
    // tested above
}


void RdfRepositoryTest::testFetchSelectResults()
{
    // tested above
}


void RdfRepositoryTest::testCloseQuery()
{
    QUrl url1 = randomUri();
    QList<Statement> sl1 = createTestData( Statement( url1, Node(), Node() ), 10 );

    repository()->addStatements( testRepository(), sl1 );
    KNEP_VERIFY

    int id = repository()->queryListStatements( testRepository(), Statement( url1, Node(), Node() ), 0 );

    repository()->closeQuery( id );
    KNEP_VERIFY

    QVERIFY( repository()->fetchListStatementsResults( id, 5 ).isEmpty() );
    QVERIFY( !repository()->success() );
}


void RdfRepositoryTest::testSupportedQueryLanguages()
{
    // I see no way to test this...
}


void RdfRepositoryTest::testSupportsQueryLanguage()
{
    QStringList langs = repository()->supportedQueryLanguages();
    foreach( QString lang, langs ) {
        QCOMPARE( 1, repository()->supportsQueryLanguage( lang ) );
    }

    QString otherLang = "someweirdlanguagename";
    if ( !langs.contains( otherLang ) ) {
        QCOMPARE( 0, repository()->supportsQueryLanguage( otherLang ) );
    }
}


void RdfRepositoryTest::testSupportedSerializations()
{
    // not implemented yet in the repository
}


void RdfRepositoryTest::testSupportsSerialization()
{
    // not implemented yet in the repository
}


void RdfRepositoryTest::testAddGraph()
{
    // not implemented yet in the repository
}


void RdfRepositoryTest::testRemoveGraph()
{
    // not implemented yet in the repository
}

QTEST_MAIN(RdfRepositoryTest)

#include "rdfrepositorytest.moc"
