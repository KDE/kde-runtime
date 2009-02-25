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
class BackTrace; //TODO rename me!

class BacktraceParser : public QObject
{
    Q_OBJECT
public:
    enum Usefulness { ReallyUseful, MayBeUseful, ProbablyUseless, Useless };

    static BacktraceParser *newParser(const QString & debuggerName, QObject *parent = 0);

    explicit BacktraceParser(QObject *parent = 0);
    virtual ~BacktraceParser();

    void connectToDebugger(BackTrace *debugger);

    virtual QString parsedBacktrace() const = 0;
    virtual Usefulness backtraceUsefulness() const = 0;
    virtual QString firstValidFunctions() const { return QString(); } //TODO dummy

Q_SIGNALS:
    void done(const QString & backtrace);

protected Q_SLOTS:
    void debuggerFinished();
    virtual void parseLine(const QString & lineStr) = 0;
};

class BacktraceParserGdb : public BacktraceParser
{
    Q_OBJECT
public:
    explicit BacktraceParserGdb(QObject *parent = 0);
    virtual ~BacktraceParserGdb();

    virtual QString parsedBacktrace() const;
    virtual Usefulness backtraceUsefulness() const;

protected Q_SLOTS:
    virtual void parseLine(const QString & lineStr);

private:
    class Private;
    Private * const d;
};

class BacktraceParserNull : public BacktraceParser
{
    Q_OBJECT
public:
    explicit BacktraceParserNull(QObject *parent = 0);
    virtual ~BacktraceParserNull();
    
    virtual QString parsedBacktrace() const;
    virtual Usefulness backtraceUsefulness() const;

protected Q_SLOTS:
    virtual void parseLine(const QString & lineStr);

private:
    QStringList m_lines;
};

#endif
