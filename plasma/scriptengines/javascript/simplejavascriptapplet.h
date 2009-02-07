/*
 *   Copyright 2007 Richard J. Moore <rich@kde.org>
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

#ifndef SCRIPT_H
#define SCRIPT_H

#include <QScriptValue>

#include <Plasma/AppletScript>
#include <Plasma/DataEngine>

#include "uiloader.h"

class QScriptEngine;
class QScriptContext;

class AppletInterface;

class SimpleJavaScriptApplet : public Plasma::AppletScript
{
    Q_OBJECT

public:
    SimpleJavaScriptApplet( QObject *parent, const QVariantList &args );
    ~SimpleJavaScriptApplet();
    bool init();

    void reportError();

    void paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect);
    QList<QAction*> contextualActions();
    void constraintsEvent(Plasma::Constraints constraints);

    Q_INVOKABLE QString findDataResource( const QString &filename );
    Q_INVOKABLE void debug( const QString &msg );

public slots:
    void dataUpdated( const QString &name, const Plasma::DataEngine::Data &data );
    void configChanged();
    void executeAction(const QString &name);

private:
    void importExtensions();
    void setupObjects();
    void callFunction(const QString &functionName, const QScriptValueList &args = QScriptValueList());

    static QString findSvg(QScriptEngine *engine, const QString &file);
    static QScriptValue dataEngine(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue service(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue loadui(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue newPlasmaSvg(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue newPlasmaFrameSvg(QScriptContext *context, QScriptEngine *engine);

    void installWidgets( QScriptEngine *engine );
    static QScriptValue createWidget(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue print(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue createPrototype( QScriptEngine *engine, const QString &name );

private:
    static KSharedPtr<UiLoader> s_widgetLoader;
    QScriptEngine *m_engine;
    QScriptValue m_self;
    AppletInterface *m_interface;
    friend class AppletInterface;
};


#endif // SCRIPT_H

