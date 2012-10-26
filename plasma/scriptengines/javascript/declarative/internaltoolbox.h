/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 by Marco Martin <notmart@gmail.com>
 *   Copyright 2012 by Sebastian Kügler <sebas@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.

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

#ifndef PLASMA_INTERNALTOOLBOX_P_H
#define PLASMA_INTERNALTOOLBOX_P_H

#include <QGraphicsWidget>

#include <Plasma/Plasma>
#include <Plasma/Containment>
#include <Plasma/AbstractToolBox>

class QAction;

class KConfigGroup;



class IconWidget;
class InternalToolBoxPrivate;

class InternalToolBox : public Plasma::AbstractToolBox
{
    Q_OBJECT
    //Q_INTERFACES(QGraphicsItem)
    Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize NOTIFY iconSizeChanged)
    Q_PROPERTY(bool isMovable READ isMovable WRITE setIsMovable NOTIFY isMovableChanged)
    Q_PROPERTY(bool immutable READ immutable WRITE setImmutable NOTIFY immutableChanged)
    Q_PROPERTY(QStringList actionKeys READ actionKeys NOTIFY actionKeysChanged)
    Q_PROPERTY(bool showing READ isShowing WRITE setShowing NOTIFY showingChanged)

public:
    enum Corner {
        Top = 0,
        TopRight,
        TopLeft,
        Left,
        Right,
        Bottom,
        BottomRight,
        BottomLeft
    };

    explicit InternalToolBox(Plasma::Containment *parent);
    explicit InternalToolBox(QObject *parent = 0, const QVariantList &args = QVariantList());
    ~InternalToolBox();


    QStringList actionKeys() const;

    bool immutable() const;
    void setImmutable(bool immutable);
    /**
     * create a toolbox tool from the given action
     * @p action the action to associate hte tool with
     */
    void addTool(QAction *action);
    /**
     * remove the tool associated with this action
     */
    void removeTool(QAction *action);
    bool isEmpty() const;
    int size() const;
    void setSize(const int newSize);
    QSize iconSize() const;
    void  setIconSize(const QSize newSize);
    bool isShowing() const; // satisfy badly named API
    bool showing() const;

    virtual void setCorner(const Corner corner);
    virtual Corner corner() const;

    bool isMovable() const;
    void setIsMovable(bool movable);

    virtual QSize fullWidth() const;
    virtual QSize fullHeight() const;
    virtual QSize cornerSize() const;
    virtual void updateToolBox() {}

    void setIconic(bool iconic);
    bool iconic() const;

    virtual void showToolBox() {} ;
    virtual void hideToolBox() {} ;

    QList<QAction *> actions() const;

public Q_SLOTS:
    void save(KConfigGroup &cg) const;
    void restore(const KConfigGroup &containmentGroup);

    // basic desktop controls
    void startLogout();
    void logout();
    void lockScreen();

    Q_INVOKABLE QAction* toolAction(const QString &key);
    Q_INVOKABLE void setShowing(const bool show);

Q_SIGNALS:
    void iconSizeChanged();
    void isMovableChanged();
    void actionKeysChanged();
    void immutableChanged();
    void showingChanged();

protected:
    Plasma::Containment *containment();

protected Q_SLOTS:
    virtual void toolTriggered(bool);
    void actionDestroyed(QObject *object);
    void immutabilityChanged(Plasma::ImmutabilityType immutability);

private:
    void init();

    Plasma::Containment *m_containment;
    InternalToolBox::Corner m_corner;
    int m_size;
    QSize m_iconSize;
    QPoint m_dragStartRelative;
    QTransform m_viewTransform;
    QList<QAction *> m_actions;
    bool m_hidden : 1;
    bool m_showing : 1;
    bool m_movable : 1;
    bool m_dragging : 1;
    bool m_userMoved : 1;
    bool m_iconic : 1;

    InternalToolBoxPrivate* d;
};



#endif // PLASMA_INTERNALTOOLBOX_P_H

