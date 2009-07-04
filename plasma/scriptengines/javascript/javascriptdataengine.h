/*
 *   Copyright 2009 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef JAVASCRIPTDATAENGINE_H
#define JAVASCRIPTDATAENGINE_H

#include <Plasma/DataEngineScript>

#include <QScriptValue>

class QScriptEngine;
class QScriptContext;

class JavaScriptDataEngine : public Plasma::DataEngineScript
{
    Q_OBJECT
    Q_PROPERTY(int sourceCount WRITE jsSetMaxSourceCount READ maxSourceCount)
    Q_PROPERTY(int minimumPollingInterval WRITE jsSetMinimumPollingInterval READ minimumPollingInterval)
    Q_PROPERTY(int pollingInterval WRITE jsSetPollingInterval READ pollingInterval)

public:
    JavaScriptDataEngine(QObject *parent, const QVariantList &args);
    bool init();

    QStringList sources() const;
    bool sourceRequestEvent(const QString &name);
    bool updateSourceEvent(const QString &source);
    //TODO: Plasma::Service *serviceForSource(const QString &source);

    int maxSourceCount() const;
    void jsSetMaxSourceCount(int count);
    void jsSetMinimumPollingInterval(int interval);
    int minimumPollingInterval() const;
    void jsSetPollingInterval(int interval);
    int pollingInterval() const;

private:
    static JavaScriptDataEngine *extractIFace(QScriptEngine *engine, QString &error);
    static QScriptValue jsSetData(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue jsRemoveAllData(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue jsRemoveData(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue jsRemoveAllSources(QScriptContext *context, QScriptEngine *engine);

    QScriptValue callFunction(const QString &functionName, const QScriptValueList &args) const;
    void reportError() const;

    QScriptEngine *m_qscriptEngine;
    QScriptValue m_iface;
};

#endif
