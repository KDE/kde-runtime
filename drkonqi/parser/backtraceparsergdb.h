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
#ifndef BACKTRACEPARSERGDB_H
#define BACKTRACEPARSERGDB_H

#include "backtraceparser.h"

class BacktraceParserGdb : public BacktraceParser
{
    Q_OBJECT
public:
    explicit BacktraceParserGdb(QObject *parent = 0);
    virtual ~BacktraceParserGdb();

    virtual QString parsedBacktrace() const;
    virtual QString simplifiedBacktrace() const;
    virtual Usefulness backtraceUsefulness() const;
    virtual QStringList firstValidFunctions() const;
    virtual QSet<QString> librariesWithMissingDebugSymbols() const;

    virtual QSyntaxHighlighter* highlightBacktrace(QTextDocument* document) const;

protected Q_SLOTS:
    virtual void resetState();
    virtual void newLine(const QString & lineStr);

private:
    void parseLine(const QString & lineStr);

    class Private;
    Private *d;
};

#endif // BACKTRACEPARSERGDB_H
