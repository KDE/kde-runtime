/*
 *   Copyright 2007 Richard J. Moore <rich@kde.org>
 *   Copyright 2009 Artur Duque de Souza <asouza@kde.org>
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
#include <QtGui/QGraphicsWidget>
#include <QtGui/QGraphicsAnchorLayout>

#include <Plasma/Applet>

#include "../backportglobal.h"
#include "../appletinterface.h"

Q_DECLARE_METATYPE(QScript::Pointer<QGraphicsItem>::wrapped_pointer_type)
Q_DECLARE_METATYPE(QGraphicsWidget*)
Q_DECLARE_METATYPE(QGraphicsAnchor*)
Q_DECLARE_METATYPE(QGraphicsLayoutItem*)
DECLARE_POINTER_METATYPE(QGraphicsAnchorLayout)

// QGraphicsAnchorLayout
DECLARE_VOID_NUMBER_METHOD(QGraphicsAnchorLayout, setSpacing)
DECLARE_NUMBER_GET_SET_METHODS(QGraphicsAnchorLayout, horizontalSpacing, setHorizontalSpacing)
DECLARE_NUMBER_GET_SET_METHODS(QGraphicsAnchorLayout, verticalSpacing, setVerticalSpacing)
DECLARE_VOID_NUMBER_METHOD(QGraphicsAnchorLayout, removeAt)


/////////////////////////////////////////////////////////////

QGraphicsLayoutItem *convertToLayoutItem (QScriptContext *ctx, int index = 0)
{
    QObject *object = ctx->argument(index).toQObject();
    QGraphicsLayoutItem *item = qobject_cast<QGraphicsWidget*>(object);

    if (!item) {
        item = qscriptvalue_cast<QGraphicsAnchorLayout*>(ctx->argument(index));
    }

    if (!item) {
        AppletInterface *interface = qobject_cast<AppletInterface*>(object);

        if (!interface) {
            interface = qobject_cast<AppletInterface*>(ctx->engine()->globalObject().property("plasmoid").toQObject());
        }

        if (interface) {
            item = interface->applet();
        }
    }

    return item;
}

static QScriptValue ctor(QScriptContext *ctx, QScriptEngine *eng)
{
    QGraphicsLayoutItem *parent = convertToLayoutItem(ctx);

    if (!parent) {
        return ctx->throwError(i18n("The parent must be a QGraphicsLayoutItem"));
    }

    return qScriptValueFromValue(eng, new QGraphicsAnchorLayout(parent));
}

BEGIN_DECLARE_METHOD(QGraphicsAnchorLayout, addAnchor) {
    QGraphicsLayoutItem *item1 = convertToLayoutItem(ctx, 0);
    QGraphicsLayoutItem *item2 = convertToLayoutItem(ctx, 2);

    if (!item1 || !item2) {
        return eng->undefinedValue();
    }

    QGraphicsAnchor *anchor = self->addAnchor(item1, static_cast<Qt::AnchorPoint>(ctx->argument(1).toInt32()),
                                              item2, static_cast<Qt::AnchorPoint>(ctx->argument(3).toInt32()));

    return eng->newQObject(anchor, QScriptEngine::QtOwnership);
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsAnchorLayout, anchor) {
    QGraphicsLayoutItem *item1 = convertToLayoutItem(ctx, 0);
    QGraphicsLayoutItem *item2 = convertToLayoutItem(ctx, 2);

    if (!item1 || !item2) {
        return eng->undefinedValue();
    }

    QGraphicsAnchor *anchor = self->anchor(item1, static_cast<Qt::AnchorPoint>(ctx->argument(1).toInt32()),
                                           item2, static_cast<Qt::AnchorPoint>(ctx->argument(3).toInt32()));

    return eng->newQObject(anchor, QScriptEngine::QtOwnership);
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsAnchorLayout, addAnchors) {
    QGraphicsLayoutItem *item1 = convertToLayoutItem(ctx, 0);
    QGraphicsLayoutItem *item2 = convertToLayoutItem(ctx, 1);

    if (!item1 || !item2) {
        return eng->undefinedValue();
    }

    self->addAnchors(item1, item2, static_cast<Qt::Orientation>(ctx->argument(2).toInt32()));
    return eng->undefinedValue();
} END_DECLARE_METHOD

BEGIN_DECLARE_METHOD(QGraphicsItem, toString) {
    return QScriptValue(eng, "QGraphicsAnchorLayout");
} END_DECLARE_METHOD

/////////////////////////////////////////////////////////////

class PrototypeAnchorLayout : public QGraphicsAnchorLayout
{
public:
    PrototypeAnchorLayout()
    { }
};

QScriptValue constructAnchorLayoutClass(QScriptEngine *eng)
{
    QScriptValue proto =
        QScript::wrapPointer<QGraphicsAnchorLayout>(eng,
                                                    new QGraphicsAnchorLayout(),
                                                    QScript::UserOwnership);
    ADD_METHOD(proto, setSpacing);
    ADD_GET_SET_METHODS(proto, horizontalSpacing, setHorizontalSpacing);
    ADD_GET_SET_METHODS(proto, verticalSpacing, setVerticalSpacing);
    ADD_METHOD(proto, removeAt);
    ADD_METHOD(proto, addAnchor);
    ADD_METHOD(proto, anchor);
    ADD_METHOD(proto, addAnchors);
    ADD_METHOD(proto, toString);

    QScript::registerPointerMetaType<QGraphicsAnchorLayout>(eng, proto);

    QScriptValue ctorFun = eng->newFunction(ctor, proto);
    return ctorFun;
}
