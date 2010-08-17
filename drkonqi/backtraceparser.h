/*
    Copyright (C) 2009  George Kiagiadakis <gkiagia@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BACKTRACEPARSER_H
#define BACKTRACEPARSER_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QMetaType>

class QTextDocument;
class QSyntaxHighlighter;

class BacktraceGenerator;

class BacktraceParser : public QObject
{
    Q_OBJECT
    Q_ENUMS(Usefulness)
public:
    enum Usefulness { InvalidUsefulness, Useless, ProbablyUseless, MayBeUseful, ReallyUseful };

    static BacktraceParser *newParser(const QString & debuggerName, QObject *parent = 0);

    explicit BacktraceParser(QObject *parent = 0);
    virtual ~BacktraceParser();

    void connectToGenerator(BacktraceGenerator *generator);

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

    QSyntaxHighlighter* highlightBacktrace(QTextDocument* document) const;

protected Q_SLOTS:
    virtual void resetState();
    virtual void newLine(const QString & lineStr);

private:
    void parseLine(const QString & lineStr);

    class Private;
    Private *d;
};

class BacktraceParserNull : public BacktraceParser
{
    Q_OBJECT
public:
    explicit BacktraceParserNull(QObject *parent = 0);
    virtual ~BacktraceParserNull();

    virtual QString parsedBacktrace() const;
    virtual QString simplifiedBacktrace() const;
    virtual Usefulness backtraceUsefulness() const;
    virtual QStringList firstValidFunctions() const;

    virtual QSet<QString> librariesWithMissingDebugSymbols() const;
    
protected Q_SLOTS:
    virtual void resetState();
    virtual void newLine(const QString & lineStr);

private:
    QStringList m_lines;
};

Q_DECLARE_METATYPE(BacktraceParser::Usefulness)

#endif
