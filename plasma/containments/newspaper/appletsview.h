/*
 *   Copyright 2010 Marco Martin <notmart@gmail.com>
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

#ifndef APPLETSVIEW_H
#define APPLETSVIEW_H

#include <Plasma/ScrollWidget>

#include "appletscontainer.h"

class QTimer;

class AppletMoveSpacer;
class DragCountdown;

class AppletsView : public Plasma::ScrollWidget
{
    Q_OBJECT

public:
    AppletsView(QGraphicsItem *parent = 0);
    ~AppletsView();

    void setAppletsContainer(AppletsContainer *appletsContainer);
    AppletsContainer *appletsContainer() const;

    void setImmediateDrag(const bool immediate);
    bool immediateDrag() const;

protected:
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);
    void dropEvent(QGraphicsSceneDragDropEvent *event);

    void manageHoverMoveEvent(QGraphicsSceneHoverEvent *event);
    void manageMouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void manageMouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void showSpacer(const QPointF &pos);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

protected Q_SLOTS:
    void appletDragRequested();
    void scrollTimeout();
    void spacerRequestedDrop(QGraphicsSceneDragDropEvent *event);

Q_SIGNALS:
    void dropRequested(QGraphicsSceneDragDropEvent *event);

private:
    AppletsContainer *m_appletsContainer;
    DragCountdown *m_dragCountdown;
    QWeakPointer<Plasma::Applet>m_appletMoved;
    AppletMoveSpacer *m_spacer;
    QGraphicsLinearLayout *m_spacerLayout;
    int m_spacerIndex;
    QTimer *m_scrollTimer;
    bool m_scrollDown;
    bool m_clickDrag;
    bool m_movingApplets;
    int m_dragTimeout;
};

#endif
