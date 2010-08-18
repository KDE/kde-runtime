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
#include "backtraceparser.h"
#include "backtraceparsergdb.h"
#include "backtraceparsernull.h"
#include <QtGui/QTextDocument>
#include <QtGui/QSyntaxHighlighter>

//factory
BacktraceParser *BacktraceParser::newParser(const QString & debuggerName, QObject *parent)
{
    if (debuggerName == "gdb") {
        return new BacktraceParserGdb(parent);
    } else {
        return new BacktraceParserNull(parent);
    }
}

BacktraceParser::BacktraceParser(QObject *parent) : QObject(parent) {}
BacktraceParser::~BacktraceParser() {}

void BacktraceParser::connectToGenerator(QObject *generator)
{
    connect(generator, SIGNAL(starting()), this, SLOT(resetState()));
    connect(generator, SIGNAL(newLine(QString)), this, SLOT(newLine(QString)));
}

QSyntaxHighlighter* BacktraceParser::highlightBacktrace(QTextDocument* /*document*/) const
{
    return 0;
}

#include "backtraceparser.moc"
