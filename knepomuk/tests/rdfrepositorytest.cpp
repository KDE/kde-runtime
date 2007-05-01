/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "rdfrepositorytest.h"

#include <knepomuk/knepomuk.h>
#include <knepomuk/services/rdfrepository.h>

#include <krandom.h>

Q_DECLARE_METATYPE( Soprano::Statement )
Q_DECLARE_METATYPE( QList<Soprano::Statement> )

using namespace Soprano;
using namespace Nepomuk::Services;


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


void RdfRepositoryTest::testListStatements_data()
{
  QTest::addColumn<Statement>("query");
  QTest::addColumn<QList<Statement> >("statements");

  Statement s1( randomUri(), Node(), Node() );
  QTest::newRow("subject only, no context") << s1 << createTestData( s1 );
  Statement s2( Node(), randomUri(), Node() );
  QTest::newRow("predicate only, no context") << s2 << createTestData( s2 );
  Statement s3( Node(), Node(), randomUri() );
  QTest::newRow("object only, no context") << s3 << createTestData( s3 );

  QUrl context = randomUri();
  Statement s4( randomUri(), Node(), Node(), context );
  QTest::newRow("subject only, with context") << s4 << createTestData( s4 );
  Statement s5( Node(), randomUri(), Node(), context );
  QTest::newRow("predicate only, with context") << s5 << createTestData( s5 );
  Statement s6( Node(), Node(), randomUri(), context );
  QTest::newRow("object only, with context") << s6 << createTestData( s6 );
}


void RdfRepositoryTest::testListStatements()
{
  QFETCH(Statement, query);
  QFETCH(QList<Statement>, statements);

  repository()->addStatements( testRepository(), statements );
  QVERIFY( repository()->success() );

  QList<Statement> listedSl = repository()->listStatements( testRepository(), query );
  QVERIFY( repository()->success() );
  QCOMPARE( listedSl.count(), statements.count() );
  QVERIFY( compareLists( statements, listedSl ) );

  repository()->removeStatements( testRepository(), statements );
  QVERIFY( repository()->success() );
}


void RdfRepositoryTest::testAddStatements()
{
  QList<Statement> sl;
  for( int i = 0; i < 100; ++i ) {
    sl.append( Statement( randomUri(), randomUri(), randomUri() ) );
  }

  repository()->addStatements( testRepository(), sl );
  QVERIFY( repository()->success() );

  QListIterator<Statement> it( sl );
  while( it.hasNext() ) {
    QVERIFY( repository()->contains( testRepository(), it.next() ) );
  }

  repository()->removeStatements( testRepository(), sl );
  QVERIFY( repository()->success() );
}


QTEST_MAIN(RdfRepositoryTest)

#include "rdfrepositorytest.moc"
