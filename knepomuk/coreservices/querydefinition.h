/* 
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006 Daniele Galdi <daniele.galdi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef QUERY_DEFINITION_H
#define QUERY_DEFINITION_H

class QString;

class QueryDefinition {
 public:
    static const QString FIND_GRAPHS;
    static const QString TO_UNIQUE_URI;
    static const QString FROM_UNIQUE_URI;
};

#endif // QUERY_DEFINITION_H
