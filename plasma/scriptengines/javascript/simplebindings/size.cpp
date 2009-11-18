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

#include <QtScript/QScriptValue>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptContext>
#include <QtCore/QSizeF>
#include "../backportglobal.h"

Q_DECLARE_METATYPE(QSizeF*)
Q_DECLARE_METATYPE(QSizeF)

static QScriptValue ctor(QScriptContext *ctx, QScriptEngine *eng)
{
    if (ctx->argumentCount() == 2)
    {
        qreal width = ctx->argument(1).toNumber();
        qreal height = ctx->argument(1).toNumber();
        return qScriptValueFromValue(eng, QSizeF(width, height));
    }

    return qScriptValueFromValue(eng, QSizeF());
}

static QScriptValue width(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QSizeF, width);
    return QScriptValue(eng, self->width());
}

static QScriptValue height(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QSizeF, height);
    return QScriptValue(eng, self->height());
}

QScriptValue constructQSizeFClass(QScriptEngine *eng)
{
    QScriptValue proto = qScriptValueFromValue(eng, QSizeF());
    QScriptValue::PropertyFlags getter = QScriptValue::PropertyGetter;
    QScriptValue::PropertyFlags setter = QScriptValue::PropertySetter;

    proto.setProperty("width", eng->newFunction(width));
    proto.setProperty("height", eng->newFunction(height));

    return eng->newFunction(ctor, proto);
}

