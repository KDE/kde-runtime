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

#ifndef _STATEMENTITERATOR_TEST_H_
#define _STATEMENTITERATOR_TEST_H_

#include "testbase.h"

#include <knepomuk/registry.h>


class StatementIteratorTest : public TestBase
{
  Q_OBJECT

 private Q_SLOTS:
   void testIterator();
};

#endif
