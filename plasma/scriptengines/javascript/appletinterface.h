/*
 *   Copyright 2008 Chani Armitage <chani@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
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

#ifndef APPLETINTERFACE_H
#define APPLETINTERFACE_H

#include <QObject>
#include <Plasma/DataEngine>

class QAction;
class QGraphicsLayout;
class SimpleJavaScriptApplet;
class QSignalMapper;
class QSizeF;

class KConfigGroup;

namespace Plasma
{
    class Applet;
} // namespace Plasa

class AppletInterface : public QObject
{
    Q_OBJECT
    Q_ENUMS(FormFactor)
    Q_ENUMS(Location)
    Q_ENUMS(AspectRatioMode)
public:
    AppletInterface(SimpleJavaScriptApplet *parent);
    ~AppletInterface();

//------------------------------------------------------------------
//enums copy&pasted from plasma.h because qtscript is evil

enum FormFactor {
    Planar = 0,  /**< The applet lives in a plane and has two
                    degrees of freedom to grow. Optimize for
                    desktop, laptop or tablet usage: a high
                    resolution screen 1-3 feet distant from the
                    viewer. */
    MediaCenter, /**< As with Planar, the applet lives in a plane
                    but the interface should be optimized for
                    medium-to-high resolution screens that are
                    5-15 feet distant from the viewer. Sometimes
                    referred to as a "ten foot interface".*/
    Horizontal,  /**< The applet is constrained vertically, but
                    can expand horizontally. */
    Vertical     /**< The applet is constrained horizontally, but
                    can expand vertically. */
};
enum Location {
    Floating = 0, /**< Free floating. Neither geometry or z-ordering
                     is described precisely by this value. */
    Desktop,      /**< On the planar desktop layer, extending across
                     the full screen from edge to edge */
    FullScreen,   /**< Full screen */
    TopEdge,      /**< Along the top of the screen*/
    BottomEdge,   /**< Along the bottom of the screen*/
    LeftEdge,     /**< Along the left side of the screen */
    RightEdge     /**< Along the right side of the screen */
};
enum AspectRatioMode {
    InvalidAspectRatioMode = -1, /**< Unsetted mode used for dev convenience
                                    when there is a need to store the
                                    aspectRatioMode somewhere */
    IgnoreAspectRatio = 0,       /**< The applet can be freely resized */
    KeepAspectRatio = 1,         /**< The applet keeps a fixed aspect ratio */
    Square = 2,                  /**< The applet is always a square */
    ConstrainedSquare = 3,       /**< The applet is no wider (in horizontal
                                    formfactors) or no higher (in vertical
                                    ones) than a square */
    FixedSize = 4                /** The applet cannot be resized */
};


//-------------------------------------------------------------------

    //FIXME kconfiggroup bindings
    Q_INVOKABLE KConfigGroup config();

    //FIXME bindings
    Plasma::DataEngine *dataEngine(const QString &name);

    Q_INVOKABLE FormFactor formFactor();

    Q_INVOKABLE Location location();

    Q_INVOKABLE QString currentActivity();

    Q_INVOKABLE AspectRatioMode aspectRatioMode();

    Q_INVOKABLE void setAspectRatioMode(AspectRatioMode mode);

    Q_INVOKABLE bool shouldConserveResources();

    Q_INVOKABLE void setFailedToLaunch(bool failed, const QString &reason = QString());

    Q_INVOKABLE bool isBusy();

    Q_INVOKABLE void setBusy(bool busy);

    Q_INVOKABLE void setConfigurationRequired(bool needsConfiguring, const QString &reason = QString());

    Q_INVOKABLE QSizeF size() const;

    Q_INVOKABLE void setAction(const QString &name, const QString &text,
                               const QString &icon = QString(), const QString &shortcut = QString());

    Q_INVOKABLE void removeAction(const QString &name);

    Q_INVOKABLE void resize(qreal w, qreal h);

    Q_INVOKABLE void setMinimumSize(qreal w, qreal h);

    Q_INVOKABLE void setPreferredSize(qreal w, qreal h);

    Q_INVOKABLE void update();

    Q_INVOKABLE void setLayout(QGraphicsLayout *layout);

    Q_INVOKABLE QGraphicsLayout *layout() const;

    //TODO setLayout? layout()?

    const Plasma::Package *package() const;
    QList<QAction*> contextualActions() const;
    Plasma::Applet *applet() const;

Q_SIGNALS:
    void releaseVisualFocus();

    void configNeedsSaving();

public Q_SLOTS:
    void dataUpdated(QString source, Plasma::DataEngine::Data data);

private:
    SimpleJavaScriptApplet *m_appletScriptEngine;
    QSet<QString> m_actions;
    QSignalMapper *m_actionSignals;
};

#endif
