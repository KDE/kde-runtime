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

#include "javascriptdataengine.h"

#include <QScriptEngine>

#include <Plasma/Package>

#include "authorization.h"
#include "scriptenv.h"
#include "simplebindings/dataengine.h"
#include "simplebindings/i18n.h"

JavaScriptDataEngine::JavaScriptDataEngine(QObject *parent, const QVariantList &args)
    : DataEngineScript(parent)
{
    Q_UNUSED(args);

    m_qscriptEngine = new QScriptEngine(this);
    m_env = new ScriptEnv(this, m_qscriptEngine);

    connect(m_qscriptEngine, SIGNAL(reportError(ScriptEnv*,bool)), this, SLOT(reportError(ScriptEnv*,bool)));
}

bool JavaScriptDataEngine::init()
{
    QScriptValue global = m_qscriptEngine->globalObject();

    bindI18N(m_qscriptEngine);

    m_iface = m_qscriptEngine->newQObject(this);
    m_iface.setScope(global);
    global.setProperty("engine", m_iface);

    global.setProperty("setData", m_qscriptEngine->newFunction(JavaScriptDataEngine::jsSetData));
    global.setProperty("removeAllData", m_qscriptEngine->newFunction(JavaScriptDataEngine::jsRemoveAllData));
    global.setProperty("removeData", m_qscriptEngine->newFunction(JavaScriptDataEngine::jsRemoveData));
    global.setProperty("removeAllSources", m_qscriptEngine->newFunction(JavaScriptDataEngine::jsRemoveAllSources));

    registerDataEngineMetaTypes(m_qscriptEngine);

    Authorization auth;
    if (!m_env->importExtensions(description(), m_iface, auth)) {
        return false;
    }

    return m_env->include(mainScript());
}

void JavaScriptDataEngine::jsSetMaxSourceCount(int count)
{
    setMaxSourceCount(count);
}

int JavaScriptDataEngine::maxSourceCount() const
{
    return dataEngine()->maxSourceCount();
}

void JavaScriptDataEngine::jsSetMinimumPollingInterval(int interval)
{
    setMinimumPollingInterval(interval);
}

int JavaScriptDataEngine::jsMinimumPollingInterval() const
{
    return minimumPollingInterval();
}

void JavaScriptDataEngine::jsSetPollingInterval(int interval)
{
    m_pollingInterval = interval;
    setPollingInterval(interval);
}

int JavaScriptDataEngine::pollingInterval() const
{
    return m_pollingInterval;
}

QScriptValue JavaScriptDataEngine::jsSetData(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() < 2) {
        return context->throwError(i18n("setData() takes at least two arguments"));
    }

    QString error;
    JavaScriptDataEngine *iFace = extractIFace(engine, error);

    if (!iFace) {
        return context->throwError(error);
    }

    const QString source = context->argument(0).toString();
    if (context->argument(1).isArray()) {
        QScriptValueIterator it(context->argument(1));
        DataEngine::Data data;
        while (it.hasNext()) {
            it.next();
            data.insert(it.name(), it.value().toVariant());
        }

        iFace->setData(source, data);
    } else {
        QString value = context->argument(1).toString();
        if (context->argumentCount() > 2) {
            const QString key = value;
            value = context->argument(2).toString();
            iFace->setData(source, key, value);
        } else {
            iFace->setData(source, value);
        }
    }

    return engine->newVariant(true);
}

JavaScriptDataEngine *JavaScriptDataEngine::extractIFace(QScriptEngine *engine, QString &error)
{
    JavaScriptDataEngine *interface = 0;
    QScriptValue engineValue = engine->globalObject().property("engine");
    QObject *engineObject = engineValue.toQObject();

    if (!engineObject) {
        error = i18n("Could not extract the DataEngineObject");
    } else {
        interface = qobject_cast<JavaScriptDataEngine *>(engineObject);
        if (!interface) {
            error = i18n("Could not extract the DataEngine");
        }
    }

    return interface;
}

QScriptValue JavaScriptDataEngine::jsRemoveAllData(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() < 1) {
        return context->throwError(i18n("removeAllData() takes at least one argument (the source name)"));
    }

    QString source = context->argument(0).toString();
    QString error;
    JavaScriptDataEngine *iFace = extractIFace(engine, error);

    if (iFace) {
        iFace->removeAllData(source);
        return engine->newVariant(true);
    }

    return context->throwError(error);
}

QScriptValue JavaScriptDataEngine::jsRemoveData(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() < 2) {
        return context->throwError(i18n("removeData() takes at least two arguments (the source and key names)"));
    }

    QString source = context->argument(0).toString();
    QString key = context->argument(1).toString();
    QString error;
    JavaScriptDataEngine *iFace = extractIFace(engine, error);

    if (iFace) {
        iFace->removeData(source, key);
        return engine->newVariant(true);
    }

    return context->throwError(error);
}

QScriptValue JavaScriptDataEngine::jsRemoveAllSources(QScriptContext *context, QScriptEngine *engine)
{
    QString error;
    JavaScriptDataEngine *iFace = extractIFace(engine, error);

    if (iFace) {
        iFace->removeAllSources();
        return engine->newVariant(true);
    }

    return context->throwError(error);
}

QScriptValue JavaScriptDataEngine::callFunction(const QString &functionName, const QScriptValueList &args) const
{
    //kDebug() << "calling" << functionName;
    QScriptValue fun = m_iface.property(functionName);
    if (fun.isFunction()) {
        QScriptContext *ctx = m_qscriptEngine->pushContext();
        ctx->setActivationObject(m_iface);
        QScriptValue rv = fun.call(m_iface, args);
        m_qscriptEngine->popContext();

        if (m_qscriptEngine->hasUncaughtException()) {
            reportError(m_env, false);
            m_qscriptEngine->clearExceptions();
        } else {
            return rv;
        }
    }

    return QScriptValue();
}

void JavaScriptDataEngine::reportError(ScriptEnv *env, bool fatal) const
{
    Q_UNUSED(fatal)

    kDebug() << "Error: " << env->engine()->uncaughtException().toString()
             << " at line " << env->engine()->uncaughtExceptionLineNumber() << endl;
    kDebug() << env->engine()->uncaughtExceptionBacktrace();
}

QStringList JavaScriptDataEngine::sources() const
{
    QScriptValueList args;
    QScriptValue rv = callFunction("sources", args);
    if (rv.isValid() && (rv.isVariant() || rv.isArray())) {
        return rv.toVariant().toStringList();
    }

    return QStringList();
}

bool JavaScriptDataEngine::sourceRequestEvent(const QString &name)
{
    QScriptValueList args;
    args << name;
    QScriptValue rv = callFunction("sourceRequestEvent", args);
    if (rv.isValid() && rv.isBool()) {
        return rv.toBool();
    }

    return false;
}

bool JavaScriptDataEngine::updateSourceEvent(const QString &source)
{
    QScriptValueList args;
    args << source;
    QScriptValue rv = callFunction("updateSourceEvent", args);
    if (rv.isValid() && rv.isBool()) {
        return rv.toBool();
    }

    return false;
}

Plasma::Service *JavaScriptDataEngine::serviceForSource(const QString &source)
{
    QScriptValueList args;
    args << source;
    QScriptValue rv = callFunction("updateSourceEvent", args);
    if (rv.isValid() && rv.isQObject()) {
        return qobject_cast<Plasma::Service *>(rv.toQObject());
    }

    return 0;
}

bool JavaScriptDataEngine::include(const QString &script)
{
    const QString path = package()->filePath("scripts", script);
    if (path.isEmpty()) {
        return false;
    }

    return m_env->include(path);
}

K_EXPORT_PLASMA_DATAENGINESCRIPTENGINE(javascriptdataengine, JavaScriptDataEngine)

#include <javascriptdataengine.moc>
