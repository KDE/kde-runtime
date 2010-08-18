/*
    Copyright (C) 2009 George Kiagiadakis <kiagiadakis.george@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "backtraceparsernull.h"
#include <QtCore/QSet>

BacktraceParserNull::BacktraceParserNull(QObject *parent) : BacktraceParser(parent) {}
BacktraceParserNull::~BacktraceParserNull() {}

QString BacktraceParserNull::parsedBacktrace() const
{
    return m_lines.join("");
}

QString BacktraceParserNull::simplifiedBacktrace() const
{
    return QString();
}

BacktraceParser::Usefulness BacktraceParserNull::backtraceUsefulness() const
{
    return MayBeUseful;
}

QStringList BacktraceParserNull::firstValidFunctions() const
{
    return QStringList();
}

void BacktraceParserNull::resetState()
{
    m_lines.clear();
}

void BacktraceParserNull::newLine(const QString & lineStr)
{
    m_lines.append(lineStr);
}

QSet<QString> BacktraceParserNull::librariesWithMissingDebugSymbols() const
{
    return QSet<QString>();
}

#include "backtraceparsernull.moc"
