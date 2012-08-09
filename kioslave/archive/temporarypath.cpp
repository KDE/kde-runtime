/*
 * temporarypath.cpp
 *
 * Copyright (C) 2012 basysKom GmbH <info@basyskom.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include "temporarypath.h"

#include <KDebug>

TemporaryPath::TemporaryPath(const QString & path): m_path(path)
{
}

TemporaryPath::~TemporaryPath()
{
     kDebug(7109) << "removing" << m_path;
     QFile::remove(m_path);
     QDir().rmpath(m_path);
     int i = m_path.lastIndexOf(QLatin1Char('/'));
     if (i != -1) {
         QDir().rmpath(m_path.left(i));
     }
}

QString TemporaryPath::path()
{
    return m_path;
}
