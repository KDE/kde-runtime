/*
 *   Copyright 2007-2008 Richard J. Moore <rich@kde.org>
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

#include "simplejavascriptapplet.h"

#include <QScriptEngine>
#include <QFile>
#include <QUiLoader>
#include <QGraphicsLayout>
#include <QWidget>

#include <KDebug>
#include <KLocale>
#include <KStandardDirs>
#include <KConfigGroup>

#include <Plasma/Applet>
#include <Plasma/Svg>
#include <Plasma/FrameSvg>
#include <Plasma/Package>

#include "appletinterface.h"
#include "uiloader.h"

using namespace Plasma;

#include "bind_dataengine.h"

Q_DECLARE_METATYPE(QPainter*)
Q_DECLARE_METATYPE(QStyleOptionGraphicsItem*)
Q_DECLARE_METATYPE(SimpleJavaScriptApplet*)
Q_DECLARE_METATYPE(AppletInterface*)
Q_DECLARE_METATYPE(Applet*)
Q_DECLARE_METATYPE(QGraphicsWidget*)
Q_DECLARE_METATYPE(QGraphicsLayout*)
Q_DECLARE_METATYPE(KConfigGroup)

Q_SCRIPT_DECLARE_QMETAOBJECT(AppletInterface, SimpleJavaScriptApplet*)

QScriptValue constructPainterClass(QScriptEngine *engine);
QScriptValue constructGraphicsItemClass(QScriptEngine *engine);
QScriptValue constructLinearLayoutClass(QScriptEngine *engine);
QScriptValue constructTimerClass(QScriptEngine *engine);
QScriptValue constructFontClass(QScriptEngine *engine);
QScriptValue constructQRectFClass(QScriptEngine *engine);
QScriptValue constructQPointClass(QScriptEngine *engine);
QScriptValue constructQSizeFClass(QScriptEngine *engine);

class DummyService : public Service
{
public:
    ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters)
    {
        Q_UNUSED(operation)
        Q_UNUSED(parameters)
        return 0;
    }
};

/*
 * Workaround the fact that QtScripts handling of variants seems a bit broken.
 */
QScriptValue variant2ScriptValue(QScriptEngine *engine, QVariant var)
{
    if (var.isNull()) {
        return engine->nullValue();
    }

    switch(var.type())
    {
        case QVariant::Invalid:
            return engine->nullValue();
        case QVariant::Bool:
            return QScriptValue(engine, var.toBool());
        case QVariant::Date:
            return engine->newDate(var.toDateTime());
        case QVariant::DateTime:
            return engine->newDate(var.toDateTime());
        case QVariant::Double:
            return QScriptValue(engine, var.toDouble());
        case QVariant::Int:
        case QVariant::LongLong:
            return QScriptValue(engine, var.toInt());
        case QVariant::String:
            return QScriptValue(engine, var.toString());
        case QVariant::Time: {
            QDateTime t(QDate::currentDate(), var.toTime());
            return engine->newDate(t);
        }
        case QVariant::UInt:
            return QScriptValue(engine, var.toUInt());
        default:
            break;
    }

    return qScriptValueFromValue(engine, var);
}

QScriptValue qScriptValueFromData(QScriptEngine *engine, const DataEngine::Data &data)
{
    DataEngine::Data::const_iterator begin = data.begin();
    DataEngine::Data::const_iterator end = data.end();
    DataEngine::Data::const_iterator it;

    QScriptValue obj = engine->newObject();

    for (it = begin; it != end; ++it) {
        //kDebug() << "setting" << it.key() << "to" << it.value();
        QString prop = it.key();
        prop.replace(' ', '_');
        obj.setProperty(prop, variant2ScriptValue(engine, it.value()));
    }

    return obj;
}

QScriptValue qScriptValueFromKConfigGroup(QScriptEngine *engine, const KConfigGroup &config)
{
    QScriptValue obj = engine->newObject();

    if (!config.isValid()) {
        return obj;
    }

    QMap<QString, QString> entryMap = config.entryMap();
    QMap<QString, QString>::const_iterator it = entryMap.constBegin();
    QMap<QString, QString>::const_iterator begin = it;
    QMap<QString, QString>::const_iterator end = entryMap.constEnd();

    //setting the group name
    obj.setProperty("__name", QScriptValue(engine, config.name()));

    //setting the key/value pairs
    for (it = begin; it != end; ++it) {
        //kDebug() << "setting" << it.key() << "to" << it.value();
        QString prop = it.key();
        prop.replace(' ', '_');
        obj.setProperty(prop, variant2ScriptValue(engine, it.value()));
    }

    return obj;
}

void kConfigGroupFromScriptValue(const QScriptValue& obj, KConfigGroup &config)
{
    KConfigSkeleton *skel = new KConfigSkeleton();
    config = KConfigGroup(skel->config(), obj.property("__name").toString());

    QScriptValueIterator it(obj);

    while (it.hasNext()) {
        it.next();
        //kDebug() << it.name() << "is" << it.value().toString();
        if (it.name() != "__name") {
            config.writeEntry(it.name(), it.value().toString());
        }
    }
}

SimpleJavaScriptApplet::SimpleJavaScriptApplet(QObject *parent, const QVariantList &args)
    : Plasma::AppletScript(parent)
{
    kDebug() << "Script applet launched, args" << args;

    m_engine = new QScriptEngine(this);
    importExtensions();
}

SimpleJavaScriptApplet::~SimpleJavaScriptApplet()
{
}

void SimpleJavaScriptApplet::reportError()
{
    kDebug() << "Error: " << m_engine->uncaughtException().toString()
             << " at line " << m_engine->uncaughtExceptionLineNumber() << endl;
    kDebug() << m_engine->uncaughtExceptionBacktrace();
}

void SimpleJavaScriptApplet::showConfigurationInterface()
{
    kDebug() << "Script: showConfigurationInterface";

    // Here we'll load a ui file...
    QScriptValue global = m_engine->globalObject();

    QScriptValue fun = m_self.property("showConfigurationInterface");
    if (!fun.isFunction()) {
        kDebug() << "Script: ShowConfiguratioInterface is not a function, " << fun.toString();
        return;
    }

    QScriptContext *ctx = m_engine->pushContext();
    ctx->setActivationObject(m_self);
    fun.call(m_self);
    m_engine->popContext();

    if (m_engine->hasUncaughtException()) {
        reportError();
    }
}

void SimpleJavaScriptApplet::configAccepted()
{
    QScriptValue fun = m_self.property("configAccepted");
    if (!fun.isFunction()) {
        kDebug() << "Script: configAccepted is not a function, " << fun.toString();
        return;
    }

    QScriptContext *ctx = m_engine->pushContext();
    ctx->setActivationObject(m_self);
    fun.call(m_self);
    m_engine->popContext();

    if (m_engine->hasUncaughtException()) {
        reportError();
    }
}

void SimpleJavaScriptApplet::dataUpdated(const QString &name, const DataEngine::Data &data)
{
    QScriptValue fun = m_self.property("dataUpdate");
    if (!fun.isFunction()) {
        kDebug() << "Script: dataUpdate is not a function, " << fun.toString();
        return;
    }

    QScriptValueList args;
    args << m_engine->toScriptValue(name) << m_engine->toScriptValue(data);

    QScriptContext *ctx = m_engine->pushContext();
    ctx->setActivationObject(m_self);
    fun.call(m_self, args);
    m_engine->popContext();

    if (m_engine->hasUncaughtException()) {
        reportError();
    }
}

void SimpleJavaScriptApplet::executeAction(const QString &name)
{
    callFunction("action_" + name);
    /*
    QScriptValue fun = m_self.property("action_" + name);
    if (fun.isFunction()) {
        QScriptContext *ctx = m_engine->pushContext();
        ctx->setActivationObject(m_self);
        fun.call(m_self);
        m_engine->popContext();

        if (m_engine->hasUncaughtException()) {
            reportError();
        }
    }*/
}

void SimpleJavaScriptApplet::paintInterface(QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect)
{
    Q_UNUSED(option)
    Q_UNUSED(contentsRect)

    //kDebug() << "paintInterface() (c++)";
    QScriptValue fun = m_self.property("paintInterface");
    if (!fun.isFunction()) {
        //kDebug() << "Script: paintInterface is not a function, " << fun.toString();
        AppletScript::paintInterface(p, option, contentsRect);
        return;
    }

    QScriptValueList args;
    args << m_engine->toScriptValue(p);
    args << m_engine->toScriptValue(const_cast<QStyleOptionGraphicsItem*>(option));
    args << m_engine->toScriptValue(contentsRect);

    QScriptContext *ctx = m_engine->pushContext();
    ctx->setActivationObject(m_self);
    fun.call(m_self, args);
    m_engine->popContext();

    if (m_engine->hasUncaughtException()) {
        reportError();
    }
}

QList<QAction*> SimpleJavaScriptApplet::contextualActions()
{
    return m_interface->contextualActions();
}

void SimpleJavaScriptApplet::callFunction(const QString &functionName, const QScriptValueList &args)
{
    QScriptValue fun = m_self.property(functionName);
    if (fun.isFunction()) {
        QScriptContext *ctx = m_engine->pushContext();
        ctx->setActivationObject(m_self);
        fun.call(m_self, args);
        m_engine->popContext();

        if (m_engine->hasUncaughtException()) {
            reportError();
        }
    }
}

void SimpleJavaScriptApplet::constraintsEvent(Plasma::Constraints constraints)
{
    QString functionName;

    if (constraints & Plasma::FormFactorConstraint) {
        callFunction("formFactorChanged");
    }

    if (constraints & Plasma::LocationConstraint) {
        callFunction("locationChanged");
    }

    if (constraints & Plasma::ContextConstraint) {
        callFunction("contextChanged");
    }
}

bool SimpleJavaScriptApplet::init()
{
    setupObjects();

    kDebug() << "ScriptName:" << applet()->name();
    kDebug() << "ScriptCategory:" << applet()->category();

    applet()->resize(200, 200);
    QFile file(mainScript());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        kWarning() << "Unable to load script file";
        return false;
    }

    QString script = file.readAll();
    kDebug() << "Script says" << script;

    m_engine->evaluate(script);
    if (m_engine->hasUncaughtException()) {
        reportError();
        return false;
    }

    return true;
}

void SimpleJavaScriptApplet::importExtensions()
{
    return; // no extension, so do bother wasting cycles
    QStringList extensions;
    //extensions << "qt.core" << "qt.gui" << "qt.svg" << "qt.xml" << "qt.plasma";
    //extensions << "qt.core" << "qt.gui" << "qt.xml";
    foreach (const QString &ext, extensions) {
        kDebug() << "importing " << ext << "...";
        QScriptValue ret = m_engine->importExtension(ext);
        if (ret.isError()) {
            kDebug() << "failed to import extension" << ext << ":" << ret.toString();
        }
    }
    kDebug() << "done importing extensions.";
}

void SimpleJavaScriptApplet::setupObjects()
{
    QScriptValue global = m_engine->globalObject();

    // Bindings for data engine
    m_engine->setDefaultPrototype(qMetaTypeId<DataEngine*>(), m_engine->newQObject(new DataEngine()));
    m_engine->setDefaultPrototype(qMetaTypeId<Service*>(), m_engine->newQObject(new DummyService()));
    m_engine->setDefaultPrototype(qMetaTypeId<ServiceJob*>(), m_engine->newQObject(new ServiceJob(QString(), QString(), QMap<QString, QVariant>())));

    global.setProperty("dataEngine", m_engine->newFunction(SimpleJavaScriptApplet::dataEngine));
    global.setProperty("service", m_engine->newFunction(SimpleJavaScriptApplet::service));
    qScriptRegisterMetaType<DataEngine::Data>(m_engine, qScriptValueFromData, 0, QScriptValue());
    qScriptRegisterMetaType<KConfigGroup>(m_engine, qScriptValueFromKConfigGroup, kConfigGroupFromScriptValue, QScriptValue());

    // Expose applet interface
    m_interface = new AppletInterface(this);
    m_self = m_engine->newQObject(m_interface);
    m_self.setScope(global);
    global.setProperty("plasmoid", m_self);

    //manually create enum values. ugh
    QMetaObject meta = AppletInterface::staticMetaObject;
    for (int i=0; i < meta.enumeratorCount(); ++i) {
        QMetaEnum e = meta.enumerator(i);
        kDebug() << e.name();
        for (int i=0; i < e.keyCount(); ++i) {
            kDebug() << e.key(i) << e.value(i);
            global.setProperty(e.key(i), QScriptValue(m_engine, e.value(i)));
        }
    }

    // Add a global loadui method for ui files
    QScriptValue fun = m_engine->newFunction(SimpleJavaScriptApplet::loadui);
    global.setProperty("loadui", fun);

    fun = m_engine->newFunction(SimpleJavaScriptApplet::print);
    global.setProperty("print", fun);


    // Work around bug in 4.3.0
    qMetaTypeId<QVariant>();

    // Add constructors
    global.setProperty("PlasmaSvg", m_engine->newFunction(SimpleJavaScriptApplet::newPlasmaSvg));
    global.setProperty("PlasmaFrameSvg", m_engine->newFunction(SimpleJavaScriptApplet::newPlasmaFrameSvg));

    // Add stuff from 4.4
    global.setProperty("QPainter", constructPainterClass(m_engine));
    global.setProperty("QGraphicsItem", constructGraphicsItemClass(m_engine));
    global.setProperty("QTimer", constructTimerClass(m_engine));
    global.setProperty("QFont", constructFontClass(m_engine));
    global.setProperty("QRectF", constructQRectFClass(m_engine));
    global.setProperty("QSizeF", constructQSizeFClass(m_engine));
    global.setProperty("QPoint", constructQPointClass(m_engine));
    global.setProperty("LinearLayout", constructLinearLayoutClass(m_engine));

    installWidgets(m_engine);
}

QString SimpleJavaScriptApplet::findDataResource(const QString &filename)
{
    QString path("plasma-script/%1");
    return KGlobal::dirs()->findResource("data", path.arg(filename));
}

void SimpleJavaScriptApplet::debug(const QString &msg)
{
    kDebug() << msg;
}

#if 0
QScriptValue SimpleJavaScriptApplet::dataEngine(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1)
        return context->throwError("dataEngine takes one argument");

    QString dataEngine = context->argument(0).toString();

    Script *self = engine->fromScriptValue<Script*>(context->thisObject());

    DataEngine *data = self->dataEngine(dataEngine);
    return engine->newQObject(data);
}
#endif

QScriptValue SimpleJavaScriptApplet::dataEngine(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1) {
        return context->throwError("dataEngine takes one argument");
    }

    QString dataEngine = context->argument(0).toString();

    QScriptValue appletValue = engine->globalObject().property("plasmoid");
    //kDebug() << "appletValue is " << appletValue.toString();

    QObject *appletObject = appletValue.toQObject();
    if (!appletObject) {
        return context->throwError(i18n("Could not extract the AppletObject"));
    }

    AppletInterface *interface = qobject_cast<AppletInterface*>(appletObject);
    if (!interface) {
        return context->throwError(i18n("Could not extract the Applet"));
    }

    DataEngine *data = interface->dataEngine(dataEngine);
    return engine->newQObject(data);
}

QScriptValue SimpleJavaScriptApplet::service(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 2) {
        return context->throwError("service takes two arguments");
    }

    QString dataEngine = context->argument(0).toString();

    QScriptValue appletValue = engine->globalObject().property("plasmoid");
    //kDebug() << "appletValue is " << appletValue.toString();

    QObject *appletObject = appletValue.toQObject();
    if (!appletObject) {
        return context->throwError(i18n("Could not extract the AppletObject"));
    }

    AppletInterface *interface = qobject_cast<AppletInterface*>(appletObject);
    if (!interface) {
        return context->throwError(i18n("Could not extract the Applet"));
    }

    DataEngine *data = interface->dataEngine(dataEngine);
    QString source = context->argument(1).toString();
    Service *service = data->serviceForSource(source);
    kDebug( )<< "lets try to get" << source << "from" << dataEngine;
    return engine->newQObject(service);
}

QScriptValue SimpleJavaScriptApplet::loadui(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1) {
        return context->throwError("loadui takes one argument");
    }

    QString filename = context->argument(0).toString();
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        return context->throwError(i18n("Unable to open '%1'",filename));
    }

    QUiLoader loader;
    QWidget *w = loader.load(&f);
    f.close();

    return engine->newQObject(w);
}

QString SimpleJavaScriptApplet::findSvg(QScriptEngine *engine, const QString &file)
{
    QScriptValue appletValue = engine->globalObject().property("plasmoid");
    //kDebug() << "appletValue is " << appletValue.toString();

    QObject *appletObject = appletValue.toQObject();
    if (!appletObject) {
        return file;
    }

    AppletInterface *interface = qobject_cast<AppletInterface*>(appletObject);
    if (!interface) {
        return file;
    }

    QString path = interface->package()->filePath("images", file + ".svg");
    if (path.isEmpty()) {
        path = interface->package()->filePath("images", file + ".svgz");

        if (path.isEmpty()) {
            return file;
        }
    }

    return path;
}

QScriptValue SimpleJavaScriptApplet::newPlasmaSvg(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 0) {
        return context->throwError(i18n("Constructor takes at least 1 argument"));
    }

    QString filename = context->argument(0).toString();
    QObject *parent = 0;

    if (context->argumentCount() == 2) {
        parent = qscriptvalue_cast<QObject *>(context->argument(1));
    }

    Svg *svg = new Svg(parent);
    svg->setImagePath(findSvg(engine, filename));
    return engine->newQObject(svg);
}

QScriptValue SimpleJavaScriptApplet::newPlasmaFrameSvg(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 0) {
        return context->throwError(i18n("Constructor takes at least 1 argument"));
    }

    QString filename = context->argument(0).toString();
    QObject *parent = 0;

    if (context->argumentCount() == 2) {
        parent = qscriptvalue_cast<QObject *>(context->argument(1));
    }

    FrameSvg *frameSvg = new FrameSvg(parent);
    frameSvg->setImagePath(findSvg(engine, filename));
    return engine->newQObject(frameSvg);
}

void SimpleJavaScriptApplet::installWidgets(QScriptEngine *engine)
{
    QScriptValue globalObject = engine->globalObject();
    UiLoader loader;

    foreach (const QString &widget, loader.availableWidgets()) {
        QScriptValue fun = engine->newFunction(createWidget);
        QScriptValue name = engine->toScriptValue(widget);
        fun.setProperty(QString("functionName"), name,
                         QScriptValue::ReadOnly | QScriptValue::Undeletable | QScriptValue::SkipInEnumeration);
        fun.setProperty(QString("prototype"), createPrototype(engine, name.toString()));

        globalObject.setProperty(widget, fun);
    }
}

QScriptValue SimpleJavaScriptApplet::createWidget(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() > 1) {
        //FIXME: 4.3: i18nc
        return context->throwError("CreateWidget takes one argument");
    }

    QGraphicsWidget *parent = 0;
    if (context->argumentCount()) {
        parent = qscriptvalue_cast<QGraphicsWidget*>(context->argument(0));

        if (!parent) {
            return context->throwError(i18n("The parent must be a QGraphicsWidget"));
        }
    }

    QString self = context->callee().property("functionName").toString();
    UiLoader loader;
    QGraphicsWidget *w = loader.createWidget(self, parent);

    if (!w) {
        return QScriptValue();
    }

    QScriptValue fun = engine->newQObject(w);
    fun.setPrototype(context->callee().property("prototype"));

    return fun;
}

QScriptValue SimpleJavaScriptApplet::print(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1) {
        return context->throwError(i18n("print takes one argument"));
    }

    kDebug() << context->argument(0).toString();
    return engine->undefinedValue();
}

QScriptValue SimpleJavaScriptApplet::createPrototype(QScriptEngine *engine, const QString &name)
{
    Q_UNUSED(name)
    QScriptValue proto = engine->newObject();

    // Hook for adding extra properties/methods
    return proto;
}

K_EXPORT_PLASMA_APPLETSCRIPTENGINE(qscriptapplet, SimpleJavaScriptApplet)

#include "simplejavascriptapplet.moc"


