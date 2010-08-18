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
#ifndef BACKTRACEPARSER_H
#define BACKTRACEPARSER_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QMetaType>
class QTextDocument;
class QSyntaxHighlighter;

class BacktraceParser : public QObject
{
    Q_OBJECT
    Q_ENUMS(Usefulness)
public:
    enum Usefulness { InvalidUsefulness, Useless, ProbablyUseless, MayBeUseful, ReallyUseful };

    static BacktraceParser *newParser(const QString & debuggerName, QObject *parent = 0);

    explicit BacktraceParser(QObject *parent = 0);
    virtual ~BacktraceParser();

    //any QObject that defines the starting() and newLine(QString) signals will do
    void connectToGenerator(QObject *generator);

    virtual QString parsedBacktrace() const = 0;
    virtual QString simplifiedBacktrace() const = 0;
    virtual Usefulness backtraceUsefulness() const = 0;
    virtual QStringList firstValidFunctions() const = 0;
    virtual QSet<QString> librariesWithMissingDebugSymbols() const = 0;

    /// default implementation does nothing, overload to provide syntax highlighting
    virtual QSyntaxHighlighter* highlightBacktrace(QTextDocument* document) const;

protected Q_SLOTS:
    virtual void resetState() = 0;
    virtual void newLine(const QString & lineStr) = 0;
};

Q_DECLARE_METATYPE(BacktraceParser::Usefulness)

#endif // BACKTRACEPARSER_H
