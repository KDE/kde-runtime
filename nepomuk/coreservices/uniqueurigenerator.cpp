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

#include "uniqueurigenerator.h"

#include <QtCore/QString>
#include <QtCore/QDateTime>

int UniqueUriGenerator::s_counter = 0;

UniqueUriGenerator::UniqueUriGenerator()
{
}

UniqueUriGenerator::~UniqueUriGenerator()
{
}

QUrl UniqueUriGenerator::uuid()
{
    m_mutex.lock();
    QDateTime current = QDateTime::currentDateTime();

    QString uri("uri:nepomuk:");
    uri.append( QString::number( current.toTime_t() ) );
    uri.append(":");
    uri.append( QString::number(++s_counter) );
    m_mutex.unlock();

    return QUrl( uri );
}

uint UniqueUriGenerator::id()
{
    m_mutex.lock();
    QDateTime current = QDateTime::currentDateTime();
    uint value = current.toTime_t() + ++s_counter;
    m_mutex.unlock();

    return value;
}
