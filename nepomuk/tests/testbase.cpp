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

#include "testbase.h"

#include <krandom.h>

#include <soprano/statement.h>

using namespace Soprano;
using namespace Nepomuk::Services;


static const QString s_testRep = "testRep";

QString TestBase::testRepository()
{
  return s_testRep;
}


Nepomuk::Middleware::Registry* TestBase::registry() const
{
  return m_registry;
}


Nepomuk::Services::RDFRepository* TestBase::repository() const
{
  return m_repository;
}


void TestBase::init()
{
}


void TestBase::cleanup()
{
    // remove everything
    repository()->removeAllStatements( testRepository(), Statement() );
}


void TestBase::initTestCase()
{
  m_registry = new Nepomuk::Middleware::Registry( this );
  m_repository = new RDFRepository( m_registry->discoverRDFRepository() );

  m_repositoryIds = m_repository->listRepositoryIds();

  m_repository->createRepository( s_testRep );
  QVERIFY( m_repository->success() );
}


void TestBase::cleanupTestCase()
{
    QStringList repos = m_repository->listRepositoryIds();
    foreach( QString repo, repos ) {
        if ( !m_repositoryIds.contains( repo ) ) {
            m_repository->removeRepository( repo );
            QVERIFY( m_repository->success() );
        }
    }

    delete m_repository;
    delete m_registry;
}


QUrl TestBase::randomUri()
{
  return "http://nepomuk-kde.semanticdesktop.org/testdata#" + KRandom::randomString( 20 );
}


bool TestBase::compareLists( const QList<Statement>& l1, const QList<Statement>& l2 )
{
  foreach( const Statement& s, l1 )
    if( !l2.contains( s ) )
      return false;

  foreach( const Statement& s, l2 )
    if( !l1.contains( s ) )
      return false;

  return true;
}

#include "testbase.moc"
