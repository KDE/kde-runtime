/*
 *   Copyright 2008 Richard J. Moore <rich@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

#include "javascriptrunner.h"

#include <QScriptEngine>
#include <QFile>

#include <Plasma/AbstractRunner>
#include <Plasma/QueryMatch>


typedef const Plasma::RunnerContext* ConstRunnerContextStar;
typedef const Plasma::QueryMatch* ConstSearchMatchStar;

Q_DECLARE_METATYPE(Plasma::QueryMatch*)
Q_DECLARE_METATYPE(Plasma::RunnerContext*)
Q_DECLARE_METATYPE(ConstRunnerContextStar)
Q_DECLARE_METATYPE(ConstSearchMatchStar)

JavaScriptRunner::JavaScriptRunner(QObject *parent, const QVariantList &args)
    : RunnerScript(parent)
{
    m_engine = new QScriptEngine( this );
    importExtensions();
}

JavaScriptRunner::~JavaScriptRunner()
{
}

Plasma::AbstractRunner* JavaScriptRunner::runner() const
{
    return RunnerScript::runner();
}

bool JavaScriptRunner::init()
{
    setupObjects();

    QFile file(mainScript());
    if ( !file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        kWarning() << "Unable to load script file";
        return false;
    }

    QString script = file.readAll();
    kDebug() << "Script says" << script;

    m_engine->evaluate( script );
    if ( m_engine->hasUncaughtException() ) {
        reportError();
        return false;
    }

    return true;
}

void JavaScriptRunner::match(Plasma::RunnerContext *search)
{
    QScriptValue fun = m_self.property( "match" );
    if ( !fun.isFunction() ) {
	kDebug() << "Script: match is not a function, " << fun.toString();
	return;
    }

    QScriptValueList args;
    args << m_engine->toScriptValue(search);

    QScriptContext *ctx = m_engine->pushContext();
    ctx->setActivationObject( m_self );
    fun.call( m_self, args );
    m_engine->popContext();

    if ( m_engine->hasUncaughtException() ) {
	reportError();
    }
}

void JavaScriptRunner::exec(const Plasma::RunnerContext *search, const Plasma::QueryMatch *action)
{
    QScriptValue fun = m_self.property( "exec" );
    if ( !fun.isFunction() ) {
	kDebug() << "Script: exec is not a function, " << fun.toString();
	return;
    }

    QScriptValueList args;
    args << m_engine->toScriptValue(search);
    args << m_engine->toScriptValue(action);

    QScriptContext *ctx = m_engine->pushContext();
    ctx->setActivationObject( m_self );
    fun.call( m_self, args );
    m_engine->popContext();

    if ( m_engine->hasUncaughtException() ) {
	reportError();
    }
}

void JavaScriptRunner::setupObjects()
{
    QScriptValue global = m_engine->globalObject();

    // Expose an applet
    m_self = m_engine->newQObject( this );
    m_self.setScope( global );    

    global.setProperty("runner", m_self);
}

void JavaScriptRunner::importExtensions()
{
    QStringList extensions;
    //extensions << "qt.core" << "qt.gui" << "qt.svg" << "qt.xml" << "org.kde.plasma";
    extensions << "qt.core" << "qt.gui" << "qt.xml";
    foreach (const QString &ext, extensions) {
        kDebug() << "importing " << ext << "...";
        QScriptValue ret = m_engine->importExtension(ext);
        if (ret.isError())
            kDebug() << "failed to import extension" << ext << ":" << ret.toString();
    }
    kDebug() << "done importing extensions.";
}

void JavaScriptRunner::reportError()
{
    kDebug() << "Error: " << m_engine->uncaughtException().toString()
	     << " at line " << m_engine->uncaughtExceptionLineNumber() << endl;
    kDebug() << m_engine->uncaughtExceptionBacktrace();
}

#include "javascriptrunner.moc"
