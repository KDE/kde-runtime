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

#ifndef UNIQUE_URI_GENERATOR_H
#define UNIQUE_URI_GENERATOR_H

#include <QMutex>
#include <QUrl>

class UniqueUriGenerator
{
 public:
    UniqueUriGenerator();
    virtual ~UniqueUriGenerator();

    QUrl uuid();

    uint id();

 private:
    QMutex m_mutex;

    static int s_counter;
};

#endif // UNIQUE_URI_GENERATOR_H
