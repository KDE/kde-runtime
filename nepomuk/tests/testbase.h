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

#ifndef _TEST_BASE_H_
#define _TEST_BASE_H_

#include <QtTest/QtTest>

#include <knepomuk/registry.h>
#include <knepomuk/services/rdfrepository.h>

class TestBase : public QObject
{
  Q_OBJECT

 public:
   Nepomuk::Backbone::Registry* registry() const;
   Nepomuk::Services::RDFRepository* repository() const;

   static QString testRepository();

   static QUrl randomUri();
   static bool compareLists( const QList<Soprano::Statement>&, const QList<Soprano::Statement>& );

 private Q_SLOTS:
   /**
    * Creates the Registry, the RDFRepository, and a default test repository.
    */
   virtual void initTestCase();
   virtual void cleanupTestCase();
   virtual void init();
   virtual void cleanup();

 private:
   Nepomuk::Backbone::Registry* m_registry;
   Nepomuk::Services::RDFRepository* m_repository;

   QStringList m_repositoryIds;
};

#endif
