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
#ifndef DRKONQI_H
#define DRKONQI_H

#include <QtCore/QObject>

class BacktraceGenerator;
class SystemInformation;
class CrashedApplication;

class DrKonqi : public QObject
{
    Q_OBJECT
public:
    enum State { ProcessRunning, ProcessStopped, DebuggerRunning };

    static DrKonqi *instance();
    bool init();
    void cleanup();

    State currentState() const;
    BacktraceGenerator *backtraceGenerator() const;
    SystemInformation *systemInformation() const;
    static const CrashedApplication *crashedApplication();

    bool appRestarted() const;
    static void saveReport(const QString & reportText, QWidget *parent = 0);

signals:
    void debuggerRunning(bool running);
    void newDebuggingApplication(const QString&);
    void acceptDebuggingApplication();

public slots:
    void restartCrashedApplication();
    void startDefaultExternalDebugger();
    void startCustomExternalDebugger();
    void registerDebuggingApplication(const QString&);

private slots:
    void stopAttachedProcess();
    void continueAttachedProcess();
    void debuggerStarting();
    void debuggerStopped();

private:
    DrKonqi();
    virtual ~DrKonqi();

    struct Private;
    Private *const d;
};

#endif
