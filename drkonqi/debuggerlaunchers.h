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
#ifndef DEBUGGERLAUNCHERS_H
#define DEBUGGERLAUNCHERS_H

#include "debugger.h"
#include "debuggermanager.h"
#include <QtDBus/QDBusAbstractAdaptor>
#include <KProcess>

class AbstractDebuggerLauncher : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name)
public:
    explicit AbstractDebuggerLauncher(DebuggerManager *parent = 0) : QObject(parent) {}
    virtual QString name() const = 0;

public slots:
    virtual void start() = 0;

signals:
    void starting();
    void finished();
    void invalidated();
};

class DefaultDebuggerLauncher : public AbstractDebuggerLauncher
{
    Q_OBJECT
public:
    explicit DefaultDebuggerLauncher(const Debugger & debugger, DebuggerManager *parent = 0);
    virtual QString name() const;

public slots:
    virtual void start();

private slots:
    void processStateChanged(QProcess::ProcessState newState);

private:
    const Debugger m_debugger;
};

class TerminalDebuggerLauncher : public DefaultDebuggerLauncher
{
    Q_OBJECT
public:
    explicit TerminalDebuggerLauncher(const Debugger & debugger, DebuggerManager *parent = 0);

public slots:
    virtual void start();
};

class DBusOldInterfaceAdaptor;

/** This class handles the old drkonqi dbus interface used by kdevelop */
class DBusOldInterfaceLauncher : public AbstractDebuggerLauncher
{
    Q_OBJECT
    friend class DBusOldInterfaceAdaptor;
public:
    explicit DBusOldInterfaceLauncher(DebuggerManager *parent = 0);
    virtual QString name() const;

public slots:
    virtual void start();

signals:
    void available();

private:
    QString m_name;
    DBusOldInterfaceAdaptor *m_adaptor;
};

class DBusOldInterfaceAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Krash")
    friend class DBusOldInterfaceLauncher;
public:
    DBusOldInterfaceAdaptor(DBusOldInterfaceLauncher *parent);

public slots:
    int pid();
    Q_NOREPLY void registerDebuggingApplication(const QString & name);

signals:
    void acceptDebuggingApplication();
};

#endif // DEBUGGERLAUNCHERS_H
