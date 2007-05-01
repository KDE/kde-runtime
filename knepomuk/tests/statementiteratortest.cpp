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

#include "statementiteratortest.h"

#include <knepomuk/knepomuk.h>
#include <knepomuk/services/rdfrepository.h>

#include <krandom.h>

#include <soprano/statement.h>

#include <QtCore/QList>


using namespace Soprano;
using namespace Nepomuk::Services;
using namespace Nepomuk::RDF;

static bool compareListAndIt( const QList<Statement>& sl, const StatementListIterator& it )
{
  QList<Statement> itL;
  while( it.hasNext() )
    itL.append( it.next() );

  return TestBase::compareLists( sl, itL );
}


void StatementIteratorTest::testIterator()
{
  QUrl theUri = randomUri();

  // create some random data
  repository()->createRepository( testRepository() );
  QList<Statement> sl;
  for( int i = 0; i < 10; ++i ) {
    sl.append( Statement( theUri, randomUri(), randomUri() ) );
  }
  repository()->addStatements( testRepository(), sl );

  // now actually test the iterator

  // 1. no reload -> big max
  StatementListIterator it1( repository()->queryListStatements( testRepository(), Statement( theUri, Node(), Node() ), 100 ), repository() );
  QVERIFY( compareListAndIt( sl, it1 ) );

  // 1. no reload -> exact max
  StatementListIterator it2( repository()->queryListStatements( testRepository(), Statement( theUri, Node(), Node() ), 10 ), repository() );
  QVERIFY( compareListAndIt( sl, it2 ) );

  // 1. with reload
  StatementListIterator it3( repository()->queryListStatements( testRepository(), Statement( theUri, Node(), Node() ), 2 ), repository() );
  QVERIFY( compareListAndIt( sl, it3 ) );

  // remove the data
  repository()->removeStatements( testRepository(), sl );
  repository()->removeRepository( testRepository() );
}

QTEST_MAIN(StatementIteratorTest)

#include "statementiteratortest.moc"
